#pragma once

#include "Base/Input.h"
#include "Controls.h"
#include <string>

BEGIN_NAMESPACE(Config)

// Game data settings
extern std::string  gGameDataCDImagePath;
extern bool         gbUseGameDataDirectory;
extern std::string  gGameDataDirectoryPath;

// Video settings
extern bool         gbFullscreen;
extern int32_t      gOutputResolutionW;
extern int32_t      gOutputResolutionH;

// Input general settings
extern float    gInputAnalogToDigitalThreshold;
extern bool     gbDefaultAlwaysRun;

// Keyboard key bindings
extern Controls::MenuActionBits gKeyboardMenuActions[Input::NUM_KEYBOARD_KEYS];
extern Controls::GameActionBits gKeyboardGameActions[Input::NUM_KEYBOARD_KEYS];

// Mouse controls and bindings
extern float                        gMouseTurnSensitivity;
extern bool                         gbInvertMouseWheelAxis[Input::NUM_MOUSE_WHEEL_AXES];
extern Controls::MenuActionBits     gMouseMenuActions[NUM_MOUSE_BUTTONS];
extern Controls::GameActionBits     gMouseGameActions[NUM_MOUSE_BUTTONS];
extern Controls::AxisBits           gMouseWheelAxisBindings[Input::NUM_MOUSE_WHEEL_AXES];

// Game controller and bindings
extern float                        gGamepadDeadZone;
extern float                        gGamepadTurnSensitivity;
extern Controls::MenuActionBits     gGamepadMenuActions[NUM_CONTROLLER_INPUTS];
extern Controls::GameActionBits     gGamepadGameActions[NUM_CONTROLLER_INPUTS];
extern Controls::AxisBits           gGamepadAxisBindings[NUM_CONTROLLER_INPUTS];

// Debug stuff
extern bool         gbAllowDebugCameraUpDownMovement;
extern uint32_t     gPerfCounterNumFramesToAverage;

// Cheat key sequences: an array of up to 16 SDL scan codes.
// Unused key slots in the sequence will be set to '0'.
struct CheatKeySequence {
    uint8_t keys[16];
    static constexpr uint32_t MAX_KEYS = (uint32_t) C_ARRAY_SIZE(keys);
};

extern CheatKeySequence gCheatKeys_GodMode;
extern CheatKeySequence gCheatKeys_NoClip;
extern CheatKeySequence gCheatKeys_MapAndThingsRevealToggle;
extern CheatKeySequence gCheatKeys_AllWeaponsAndAmmo;
extern CheatKeySequence gCheatKeys_AllWeaponsAmmoAndKeys;
extern CheatKeySequence gCheatKeys_WarpToMap;
extern CheatKeySequence gCheatKeys_ChangeMusic;
extern CheatKeySequence gCheatKeys_GrantInvisibility;
extern CheatKeySequence gCheatKeys_GrantRadSuit;
extern CheatKeySequence gCheatKeys_GrantBeserk;
extern CheatKeySequence gCheatKeys_GrantInvulnerability;

// Startup and shutdown the prefs module
void init() noexcept;
void shutdown() noexcept;

END_NAMESPACE(Config)
