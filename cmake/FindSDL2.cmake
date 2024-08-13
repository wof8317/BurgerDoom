# FindSDL2.cmake - Locate the SDL2 library and headers
# This module defines the following variables:
#   SDL2_FOUND        - True if SDL2 is found
#   SDL2_INCLUDE_DIRS - Where to find SDL2 headers
#   SDL2_LIBRARIES    - The libraries to link against

if (WIN32)
    set(SDL2_SEARCH_PATHS
        "C:/SDL2/include"
        "C:/SDL2/lib"
        "C:/Program Files/SDL2/include"
        "C:/Program Files/SDL2/lib"
    )
elseif(APPLE)
    find_library(SDL2_LIBRARY SDL2)
    set(SDL2_INCLUDE_DIRS "/Library/Frameworks/SDL2.framework/Headers")
else()
    set(SDL2_SEARCH_PATHS
        "/usr/include/SDL2"
        "/usr/local/include/SDL2"
        "/usr/lib"
        "/usr/local/lib"
    )
endif()

find_path(SDL2_INCLUDE_DIR SDL.h PATH_SUFFIXES SDL2 PATHS ${SDL2_SEARCH_PATHS})
find_library(SDL2_LIBRARY NAMES SDL2 PATHS ${SDL2_SEARCH_PATHS})

if (SDL2_INCLUDE_DIR AND SDL2_LIBRARY)
    set(SDL2_FOUND TRUE)
    set(SDL2_INCLUDE_DIRS ${SDL2_INCLUDE_DIR})
    set(SDL2_LIBRARIES ${SDL2_LIBRARY})
else()
    set(SDL2_FOUND FALSE)
endif()

# Print the status to help with debugging
if(SDL2_FOUND)
    message(STATUS "Found SDL2: ${SDL2_LIBRARY}")
    message(STATUS "SDL2 include dir: ${SDL2_INCLUDE_DIR}")
else()
    message(STATUS "Could NOT find SDL2")
endif()