#include "Renderer_Internal.h"

#include "Base/Endian.h"
#include "Base/FMath.h"
#include "Base/Tables.h"
#include "Blit.h"
#include "Game/Data.h"
#include "Map/MapData.h"
#include "Textures.h"
#include "Video.h"
#include <algorithm>

//----------------------------------------------------------------------------------------------------------------------
// Code for drawing walls and skies in the game.
//
// Notes:
//  (1) Clip values are the solid pixel bounding the range
//  (2) floorclip starts out ScreenHeight
//  (3) ceilingclip starts out -1
//----------------------------------------------------------------------------------------------------------------------
BEGIN_NAMESPACE(Renderer)

static uint32_t gClipBoundTop[MAXSCREENWIDTH];          // Bounds top y for vertical clipping
static uint32_t gClipBoundBottom[MAXSCREENWIDTH];       // Bounds bottom y for vertical clipping

struct drawtex_t {
    const ImageData*    pData;          // Image data for the texture
    float               topheight;      // Top texture height in global pixels
    float               bottomheight;   // Bottom texture height in global pixels
    float               texturemid;     // Anchor point for texture
};

//----------------------------------------------------------------------------------------------------------------------
// Draw a single column of a wall clipped to the 3D view
//----------------------------------------------------------------------------------------------------------------------
static void drawClippedWallColumn(
    const int32_t viewX,
    const float viewY,
    const uint32_t columnHeight,
    const float invColumnScale,
    const uint32_t texX,
    const float texY,
    const float lightMultiplier,
    const ImageData& texData
) noexcept {
    // Get integer Y coordinate
    const int32_t viewYI = (int32_t) viewY;

    // Clip to bottom of the screen
    if (viewYI >= (int32_t) gScreenHeight)
        return;
    
    // The y texture coordinate and step
    const float texYStep = invColumnScale;
    float texYClipped = texY;

    // Clip to top of the screen
    const uint32_t pixelsOffscreenAtTop = (uint32_t) (viewYI < 0) ? -viewYI : 0;

    if (pixelsOffscreenAtTop >= (int32_t) columnHeight)
        return;
    
    const float pixelsOffscreenAtTopF = (viewY < 0.0f) ? -viewY : 0.0f;

    // Do adjustments to the y texture coordinate:
    //
    // (1) If the column is being clipped then we need to skip past the offscreen portion.
    // (2) For more 'solid', less 'fuzzy' and temporally stable texture mapping, we also need
    //     to adjustments based on the sub pixel y-position of the column. If for example the
    //     true pixel Y position is 0.25 units above it's integer position then count 0.25
    //     pixels as already having been stepped and adjust the texture coordinate accordingly.
    {
        float pixelSkip;

        if (pixelsOffscreenAtTop > 0) {
            pixelSkip = pixelsOffscreenAtTopF + 1.0f;
        } else {            
            pixelSkip = 1.0f - std::fmod(viewY, 1.0f);
        }

        texYClipped += pixelSkip * texYStep;
    }
    
    // Compute clipped column height and texture coordinate
    const int32_t clippedViewY = viewYI + pixelsOffscreenAtTop;
    const uint32_t maxColumnHeight = gScreenHeight - clippedViewY;
    const uint32_t clippedColumnHeight = std::min(columnHeight - pixelsOffscreenAtTop, maxColumnHeight);

    // Do the blit
    Blit::blitColumn<
        Blit::BCF_STEP_Y |
        Blit::BCF_V_WRAP_WRAP |
        Blit::BCF_COLOR_MULT_RGB
    >(
        texData,
        (float) texX,
        texYClipped,
        0.0f,
        Video::gFrameBuffer,
        Video::SCREEN_WIDTH,
        Video::SCREEN_HEIGHT,
        viewX + gScreenXOffset,
        clippedViewY + gScreenYOffset,
        clippedColumnHeight,
        0,
        invColumnScale,
        lightMultiplier,
        lightMultiplier,
        lightMultiplier
    );
}

//----------------------------------------------------------------------------------------------------------------------
// Draws a single column of the sky
//----------------------------------------------------------------------------------------------------------------------
static void drawSkyColumn(const uint32_t viewX) noexcept {
    // Note: sky textures are 256 pixels wide so the mask wraps it around
    const uint32_t texX = (((gXToViewAngle[viewX] + gViewAngleBAM) >> ANGLETOSKYSHIFT) & 0xFF);

    // Figure out the sky column height and texel step (y)
    const Texture* const pTexture = (const Texture*) getWallTexture(getCurrentSkyTexNum());     // FIXME: don't keep doing this for each column
    const uint32_t skyTexH = pTexture->data.height;

    const Fixed skyScale = fixedDiv(intToFixed(gScreenHeight), intToFixed(Renderer::REFERENCE_3D_VIEW_HEIGHT));
    const Fixed scaledColHeight = fixedMul(intToFixed(skyTexH), skyScale);
    const uint32_t roundColHeight = ((scaledColHeight & FRACMASK) != 0) ? 1 : 0;
    const uint32_t colHeight = (uint32_t) fixedToInt(scaledColHeight) + roundColHeight;
    BLIT_ASSERT(colHeight < gScreenHeight);

    const float texYStep = FMath::doomFixed16ToFloat<float>(Blit::calcTexelStep(skyTexH, colHeight));

    // Draw the sky column
    Blit::blitColumn<
        Blit::BCF_STEP_Y
    >(
        pTexture->data,
        (float) texX,
        0.0f,
        0.0f,
        Video::gFrameBuffer,
        Video::SCREEN_WIDTH,
        Video::SCREEN_HEIGHT,
        viewX + gScreenXOffset,
        gScreenYOffset,
        colHeight,
        0,
        texYStep
    );
}

static inline int32_t getWallColumnHeight(const float topY, const float bottomY) noexcept {
    return (int32_t) std::ceil(bottomY - topY);
}

//----------------------------------------------------------------------------------------------------------------------
// Compute the screen location and part of the texture to use for the given draw texture and then draw a single wall
// column based on that information.
//----------------------------------------------------------------------------------------------------------------------
static void drawWallColumn(
    const drawtex_t& tex, 
    const uint32_t viewX,
    const uint32_t texX,
    const float wallTopY,
    const float wallBottomY,
    const float invColumnScale,
    const float lightMultiplier
) noexcept {
    // Compute height of column from source image height and make sure not invalid
    const int32_t columnHeight = getWallColumnHeight(wallTopY, wallBottomY);

    if (columnHeight <= 0)
        return;
    
    // View Y to draw at and y position in texture to use
    const ImageData& texData = *tex.pData;
    const uint32_t texWidth = texData.width;
    const uint32_t texHeight = texData.height;

    const float viewY = wallTopY;
    const float texYNotOffset = tex.texturemid - tex.topheight;
    const float texY = (texYNotOffset < 0.0f) ? texYNotOffset + texHeight : texYNotOffset;  // DC: This is required for correct vertical alignment in some cases

    // Draw the column
    drawClippedWallColumn(
        viewX,
        viewY,
        columnHeight,
        invColumnScale,
        (texX % texWidth),  // Wraparound texture x coord!
        texY,
        lightMultiplier,
        texData
    );
}

//----------------------------------------------------------------------------------------------------------------------
// Draw a single wall texture.
// Also save states for pending ceiling, floor and future clipping
//----------------------------------------------------------------------------------------------------------------------
static void drawSeg(const viswall_t& seg) noexcept {
    // If there is nothing (no upper or lower part) to draw for this seg then just bail immediately...
    const uint32_t actionBits = seg.WallActions;
    
    if ((actionBits & (AC_TOPTEXTURE|AC_BOTTOMTEXTURE)) == 0)
        return;

    // Grab some lighting stuff
    LightParams lightParams = getLightParams(seg.seglightlevel);
    const float segLightMul = seg.SegPtr->lightMul;
    
    // Y center of the screen and scaled half view width
    const float viewCenterY = (float) gCenterY;
    const float stretchWidth = FMath::doomFixed16ToFloat<float>(gStretchWidth);
    
    // How much to step scale for each x pixel and seg center angle
    const float segLeftScale = seg.LeftScale;
    const float segScaleStep = seg.ScaleStep;
    const float segCenterAngle = FMath::doomAngleToRadians<float>(seg.CenterAngle);

    // Store the current y coordinate for the top and bottom of the top and bottom walls here.
    // Also store the step per pixel in y for the top and bottom coords.
    // This helps make the result sub-pixel accurate.
    float topTexViewTYStep;
    float topTexViewBYStep;
    float topTexViewTY;
    float topTexViewBY;
    float bottomTexViewTYStep;
    float bottomTexViewBYStep;
    float bottomTexViewTY;
    float bottomTexViewBY;
    
    // Storing some texture related params here
    drawtex_t topTex;
    drawtex_t bottomTex;

    // Setup parameters for the top and bottom wall (if present).
    // Note that if the seg is solid the 'top' wall is actually the entire wall.        
    if (actionBits & AC_TOPTEXTURE) {   
        const Texture* const pTex = seg.t_texture;

        topTex.topheight = seg.t_topheight;
        topTex.bottomheight = seg.t_bottomheight;
        topTex.texturemid = seg.t_texturemid;         
        topTex.pData = &pTex->data;

        const float topWorldTY = seg.t_topheight;
        const float topWorldBY = seg.t_bottomheight;

        topTexViewTYStep = -segScaleStep * topWorldTY;
        topTexViewBYStep = -segScaleStep * topWorldBY;
        topTexViewTY = viewCenterY - topWorldTY * segLeftScale;
        topTexViewBY = viewCenterY - topWorldBY * segLeftScale;
    }
        
    if (actionBits & AC_BOTTOMTEXTURE) {
        const Texture* const pTex = seg.b_texture;

        bottomTex.topheight = seg.b_topheight;
        bottomTex.bottomheight = seg.b_bottomheight;
        bottomTex.texturemid = seg.b_texturemid;
        bottomTex.pData = &pTex->data;

        const float bottomWorldTY = seg.b_topheight;
        const float bottomWorldBY = seg.b_bottomheight;
        
        bottomTexViewTYStep = -segScaleStep * bottomWorldTY;
        bottomTexViewBYStep = -segScaleStep * bottomWorldBY;
        bottomTexViewTY = viewCenterY - bottomWorldTY * segLeftScale;
        bottomTexViewBY = viewCenterY - bottomWorldBY * segLeftScale;
    }
        
    // Init the scale fraction and step through all the columns in the seg
    float columnScale = segLeftScale;
    
    for (int32_t viewX = seg.leftX; viewX <= seg.rightX; ++viewX) {
        const float invColumnScale = 1.0f / columnScale;
        
        // Calculate texture offset into shape
        const uint32_t texX = (uint32_t) std::round(
            seg.offset - (
                std::tan(segCenterAngle - getViewAngleForX(viewX)) *
                seg.distance
            )
        );

        // Figure out the light multiplier to use
        const float viewStretchWidthF = FMath::doomFixed16ToFloat<float>(gStretchWidth);
        const float columnDist = (1.0f / columnScale) * viewStretchWidthF;
        const float distLightMul = lightParams.getLightMulForDist(columnDist);
        const float lightMul = std::fmax(distLightMul * segLightMul, MIN_LIGHT_MUL);

        // Daw the top and bottom textures (if present) and update increments for the next column
        if (actionBits & AC_TOPTEXTURE) {
            drawWallColumn(topTex, viewX, texX, topTexViewTY, topTexViewBY, invColumnScale, lightMul);
            topTexViewTY += topTexViewTYStep;
            topTexViewBY += topTexViewBYStep;
        }

        if (actionBits & AC_BOTTOMTEXTURE) {
            drawWallColumn(bottomTex, viewX, texX, bottomTexViewTY, bottomTexViewBY, invColumnScale, lightMul);
            bottomTexViewTY += bottomTexViewTYStep;
            bottomTexViewBY += bottomTexViewBYStep;
        }
        
        columnScale += segScaleStep;
    }
}

//----------------------------------------------------------------------------------------------------------------------
// Given a span of pixels, see if it is already defined in a record somewhere.
// If it is, then merge it otherwise make a new plane definition.
//----------------------------------------------------------------------------------------------------------------------
static visplane_t* findPlane(
    visplane_t& plane,
    const float height,
    const uint32_t picHandle,
    const int32_t start,
    const int32_t stop,
    const uint32_t light
) noexcept {
    visplane_t* pPlane = &plane + 1;    // Automatically skip to the next plane

    while (pPlane < gpEndVisPlane) {
        if ((height == pPlane->height) &&           // Same plane as before?
            (picHandle == pPlane->picHandle) &&
            (light == pPlane->planeLight) &&
            (pPlane->cols[start].isUndefined())     // Not defined yet?
        ) {
            if (start < pPlane->minX) {     // In range of the plane?
                pPlane->minX = start;       // Mark the new edge
            }
            
            if (stop > pPlane->maxX) {
                pPlane->maxX = stop;        // Mark the new edge
            }

            return pPlane;                  // Use the same one as before
        }

        ++pPlane;
    }

    // Make a new plane
    ASSERT_LOG(gpEndVisPlane - gVisPlanes < MAXVISPLANES, "No more visplanes!");
    pPlane = gpEndVisPlane;
    ++gpEndVisPlane;

    pPlane->height = height;            // Init all the vars in the visplane
    pPlane->picHandle = picHandle;
    pPlane->minX = start;
    pPlane->maxX = stop;
    pPlane->planeLight = light;         // Set the light level

    // Quickly fill in the visplane table:
    // A brute force method to fill in the visplane record FAST!
    {
        ScreenYPair* pSet = pPlane->cols;  

        for (uint32_t j = gScreenWidth / 8; j > 0; --j) {
            pSet[0] = ScreenYPair{ UINT16_MAX, 0 };
            pSet[1] = ScreenYPair{ UINT16_MAX, 0 };
            pSet[2] = ScreenYPair{ UINT16_MAX, 0 };
            pSet[3] = ScreenYPair{ UINT16_MAX, 0 };
            pSet[4] = ScreenYPair{ UINT16_MAX, 0 };
            pSet[5] = ScreenYPair{ UINT16_MAX, 0 };
            pSet[6] = ScreenYPair{ UINT16_MAX, 0 };
            pSet[7] = ScreenYPair{ UINT16_MAX, 0 };
            pSet += 8;
        }
    }

    return pPlane;
}

//----------------------------------------------------------------------------------------------------------------------
// Do a fake wall rendering so I can get all the visplane records.
// This is a fake-o routine so I can later draw the wall segments from back to front.
//----------------------------------------------------------------------------------------------------------------------
static void segLoop(const viswall_t& seg) noexcept {
    // Some useful stuff
    const float viewCenterY = (float) gCenterY;
    const uint32_t actionBits = seg.WallActions;

    // How much to step scale for each x pixel and starting scale
    const float segLeftScale = seg.LeftScale;
    const float segScaleStep = seg.ScaleStep;

    // Various y coordinates and stepping values used during the wall drawing simulation.
    // Store and step in a way that is consistent with the actual wall drawing loop - so the results agree.
    float floorYStep;
    float floorY;
    float newFloorYStep;
    float newFloorY;
    float ceilYStep;
    float ceilY;
    float newCeilYStep;
    float newCeilY;

    if (actionBits & AC_ADDFLOOR) {
        const float floorWorldY = seg.floorheight;
        floorYStep = -segScaleStep * floorWorldY;
        floorY = viewCenterY - floorWorldY * segLeftScale;
    }

    if (actionBits & AC_ADDCEILING) {
        const float ceilWorldY = seg.ceilingheight;
        ceilYStep = -segScaleStep * ceilWorldY;
        ceilY = viewCenterY - ceilWorldY * segLeftScale;
    }

    if (actionBits & AC_NEWFLOOR) {
        const float newFloorWorldY = seg.floornewheight;
        newFloorYStep = -segScaleStep * newFloorWorldY;
        newFloorY = viewCenterY - newFloorWorldY * segLeftScale;
    }

    if (actionBits & AC_NEWCEILING) {
        const float newCeilWorldY = seg.ceilingnewheight;
        newCeilYStep = -segScaleStep * newCeilWorldY;
        newCeilY = viewCenterY - newCeilWorldY * segLeftScale;
    }

    // Reset the visplane pointers.
    // Note: visplanes[0] is zero to force a FindPlane on the first pass
    visplane_t* pFloorPlane = gVisPlanes;
    visplane_t* pCeilPlane = gVisPlanes;

    // Init the scale fraction and step through all the columns in the seg
    float columnScale = segLeftScale;

    for (int32_t viewX = seg.leftX; viewX <= seg.rightX; ++viewX) {
        float scale = std::fmin(columnScale, MAX_RENDER_SCALE);     // Current scaling factor        
        const int32_t clipBoundTY = gClipBoundTop[viewX];          // Get the top y clip
        const int32_t clipBoundBY = gClipBoundBottom[viewX];         // Get the bottom y clip

        // Shall I add the floor?
        if (actionBits & AC_ADDFLOOR) {
            int32_t top = (int32_t) floorY;             // Y coord of top of floor
            top = std::max(top, clipBoundTY + 1);       // Clip the top of floor to the bottom of the visible area
            
            // Draw to the bottom of the screen if the span is valid
            int32_t bottom = clipBoundBY - 1;

            if (top <= bottom) {                                // Valid span?
                if (pFloorPlane->cols[viewX].isDefined()) {     // Not already covered?
                    pFloorPlane = findPlane(
                        *pFloorPlane,
                        seg.floorheight,
                        seg.FloorPic,
                        viewX,
                        seg.rightX,
                        seg.seglightlevel
                    );
                }
                pFloorPlane->cols[viewX] = ScreenYPair{ (uint16_t) top, (uint16_t) bottom };    // Set the new vertical span
            }
            floorY += floorYStep;
        }

        // Handle ceilings
        if (actionBits & AC_ADDCEILING) {
            int32_t top = clipBoundTY + 1;                  // Start from the ceiling
            int32_t bottom = (int32_t) ceilY - 1;           // Bottom of the height
            bottom = std::min(bottom, clipBoundBY - 1);     // Clip the bottom
            
            if (top <= bottom) {                                    // Valid span?
                if (pCeilPlane->cols[viewX].isDefined()) {          // Already in use?
                    pCeilPlane = findPlane(
                        *pCeilPlane,
                        seg.ceilingheight,
                        seg.CeilingPic,
                        viewX,
                        seg.rightX,
                        seg.seglightlevel
                    );
                }
                pCeilPlane->cols[viewX] = { (uint16_t) top, (uint16_t) bottom };    // Set the vertical span
            }
            ceilY += ceilYStep;
        }

        // Sprite clip sils
        if (actionBits & (AC_BOTTOMSIL|AC_NEWFLOOR)) {
            int32_t low = (int32_t) newFloorY;
            low = std::min(low, clipBoundBY);
            low = std::max(low, 0);

            if (actionBits & AC_BOTTOMSIL) {
                seg.BottomSil[viewX] = low;
            } 
            if (actionBits & AC_NEWFLOOR) {
                gClipBoundBottom[viewX] = low;
            }

            newFloorY += newFloorYStep;
        }

        if (actionBits & (AC_TOPSIL|AC_NEWCEILING)) {
            int32_t high = (int32_t) newCeilY - 1;
            high = std::max(high, clipBoundTY);
            high = std::min(high, (int32_t) gScreenHeight - 1);

            if (actionBits & AC_TOPSIL) {
                seg.TopSil[viewX] = high + 1;
            }
            if (actionBits & AC_NEWCEILING) {
                gClipBoundTop[viewX] = high;
            }

            newCeilY += newCeilYStep;
        }

        // I can draw the sky right now!!
        if (actionBits & AC_ADDSKY) {
            int32_t bottom = (int32_t)(viewCenterY - scale * seg.ceilingheight);
            if (bottom > clipBoundBY) {
                bottom = clipBoundBY;
            }

            if (clipBoundTY + 1 < bottom) {     // Valid?
                drawSkyColumn(viewX);           // Draw the sky
            }
        }

        // Step to the next column
        columnScale += segScaleStep;
    }
}

//----------------------------------------------------------------------------------------------------------------------
// Follow the list of walls and draw each and every wall fragment.
// Note : I draw the walls closest to farthest and I maintain a ZBuffer
//----------------------------------------------------------------------------------------------------------------------
void drawAllLineSegs() noexcept {
    // Init the vertical clipping records
    for (uint32_t i = 0; i < gScreenWidth; ++i) {
        gClipBoundTop[i] = -1;                  // Allow to the ceiling
        gClipBoundBottom[i] = gScreenHeight;    // Stop at the floor
    }

    // Process all the wall segments: create the visplanes and draw the sky only
    viswall_t* const pStartWall = gVisWalls;
    viswall_t* const pEndWall = gpEndVisWall;

    for (viswall_t* pWall = pStartWall; pWall < pEndWall; ++pWall) {
        segLoop(*pWall);
    }
    
    // Now I actually draw the walls back to front to allow for clipping because of slop.
    // Each wall is only drawn if needed...
    for (viswall_t* pWall = gpEndVisWall - 1; pWall >= pStartWall; --pWall) {
        drawSeg(*pWall);
    }
}

END_NAMESPACE(Renderer)
