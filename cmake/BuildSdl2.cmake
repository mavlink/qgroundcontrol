# find_package(SDL2 QUIET)
if(SDL2_FOUND)
    message(STATUS "Using SDL2 via find_package")
endif()

# 2. Try using a vendored SDL library
if(NOT SDL2_FOUND AND EXISTS "${CMAKE_CURRENT_LIST_DIR}/SDL/CMakeLists.txt")
    message(STATUS "Using SDL2 via add_subdirectory")
    add_subdirectory(SDL)
    set(SDL2_FOUND TRUE)
endif()

# 3. Download SDL, and use that.
if(NOT SDL2_FOUND)
    message(STATUS "Using SDL2 via FetchContent")
    include(FetchContent)
    set(SDL2_DISABLE_SDL2MAIN TRUE CACHE INTERNAL "")
    set(SDL_SHARED FALSE CACHE INTERNAL "")
    set(SDL_STATIC TRUE CACHE INTERNAL "")
    set(SDL_TEST FALSE CACHE INTERNAL "")

    set(SDL_ATOMIC TRUE CACHE INTERNAL "")
    set(SDL_AUDIO FALSE CACHE INTERNAL "")
    set(SDL_CPUINFO TRUE CACHE INTERNAL "")
    set(SDL_EVENTS TRUE CACHE INTERNAL "")
    set(SDL_FILE FALSE CACHE INTERNAL "")
    set(SDL_FILESYSTEM FALSE CACHE INTERNAL "")
    set(SDL_HAPTIC FALSE CACHE INTERNAL "")
    set(SDL_HIDAPI TRUE CACHE INTERNAL "")
    set(SDL_JOYSTICK TRUE CACHE INTERNAL "")
    set(SDL_LOADSO TRUE CACHE INTERNAL "")
    set(SDL_LOCALE TRUE CACHE INTERNAL "")
    set(SDL_MISC FALSE CACHE INTERNAL "")
    set(SDL_POWER FALSE CACHE INTERNAL "")
    set(SDL_RENDER FALSE CACHE INTERNAL "")
    set(SDL_SENSOR FALSE CACHE INTERNAL "")
    set(SDL_THREADS TRUE CACHE INTERNAL "")
    set(SDL_TIMERS FALSE CACHE INTERNAL "")
    set(SDL_VIDEO FALSE CACHE INTERNAL "")

    FetchContent_Declare(
        SDL
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG release-2.30.1
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
    )
    FetchContent_MakeAvailable(SDL)
    set(SDL2_FOUND TRUE)
endif()
