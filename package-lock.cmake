# CPM Package Lock
# This file should be committed to version control

# ulog_cpp (unversioned)
# CPMDeclarePackage(ulog_cpp
#  NAME ulog_cpp
#  GIT_TAG main
#  GITHUB_REPOSITORY PX4/ulog_cpp
#)
# ArduPilotParams (unversioned)
# CPMDeclarePackage(ArduPilotParams
#  NAME ArduPilotParams
#  GIT_TAG main
#  GITHUB_REPOSITORY ArduPilot/ParameterRepository
#)
# px4-gpsdrivers (unversioned)
# CPMDeclarePackage(px4-gpsdrivers
#  NAME px4-gpsdrivers
#  GIT_TAG caf5158061bd10e79c9f042abb62c86bc6f3e7a7
#  GITHUB_REPOSITORY PX4/PX4-GPSDrivers
#  SOURCE_SUBDIR src
#)
# sdl_gamecontrollerdb (unversioned)
# CPMDeclarePackage(sdl_gamecontrollerdb
#  NAME sdl_gamecontrollerdb
#  GIT_TAG master
#  GITHUB_REPOSITORY mdqinc/SDL_GameControllerDB
#)
# SDL3
CPMDeclarePackage(SDL3
  NAME SDL3
  VERSION 3.2.20
  GIT_TAG release-3.2.20
  GITHUB_REPOSITORY libsdl-org/SDL
  OPTIONS
    "SDL_INSTALL OFF"
    "SDL_UNINSTALL OFF"
    "SDL_SHARED OFF"
    "SDL_STATIC ON"
    "SDL_TEST_LIBRARY OFF"
    "SDL_EXAMPLES OFF"
    "SDL_AUDIO OFF"
    "SDL_VIDEO OFF"
    "SDL_GPU OFF"
    "SDL_RENDER OFF"
    "SDL_CAMERA OFF"
    "SDL_JOYSTICK ON"
    "SDL_HAPTIC ON"
    "SDL_HIDAPI ON"
    "SDL_POWER ON"
    "SDL_SENSOR OFF"
    "SDL_DIALOG OFF"
    "SDL_CCACHE ON"
    "SDL_DBUS OFF"
    "SDL_IBUS OFF"
    "SDL_MMX OFF"
    "SDL_VIRTUAL_JOYSTICK ON"
    "SDL_UNIX_CONSOLE_BUILD ON"
)
# mavlink (unversioned)
# CPMDeclarePackage(mavlink
#  NAME mavlink
#  GIT_TAG dd17c1a65de7b9ad8dd6e3491a8690c0d0b27ba1
#  GIT_REPOSITORY https://github.com/mavlink/mavlink.git
#  OPTIONS
#    "MAVLINK_DIALECT all"
#    "MAVLINK_VERSION 2.0"
#)
# libevents (unversioned)
# CPMDeclarePackage(libevents
#  NAME libevents
#  GIT_TAG main
#  GITHUB_REPOSITORY mavlink/libevents
#  SOURCE_SUBDIR libs/cpp
#)
# zlib (unversioned)
# CPMDeclarePackage(zlib
#  NAME zlib
#  GIT_TAG develop
#  GITHUB_REPOSITORY madler/zlib
#  OPTIONS
#    "ZLIB_BUILD_TESTING OFF"
#    "ZLIB_BUILD_SHARED OFF"
#    "ZLIB_BUILD_STATIC ON"
#    "ZLIB_BUILD_MINIZIP OFF"
#    "ZLIB_INSTALL OFF"
#    "ZLIB_PREFIX OFF"
#)
# xz-embedded
CPMDeclarePackage(xz-embedded
  NAME xz-embedded
  VERSION 2024-12-30
  GITHUB_REPOSITORY tukaani-project/xz-embedded
  OPTIONS
    "BUILD_SHAPELIB_CONTRIB OFF"
    "BUILD_APPS OFF"
    "BUILD_TESTING OFF"
)
# geographiclib
CPMDeclarePackage(geographiclib
  NAME geographiclib
  VERSION 2.5
  GIT_TAG r2.5
  GITHUB_REPOSITORY geographiclib/geographiclib
  OPTIONS
    "BUILD_BOTH_LIBS OFF"
    "BUILD_DOCUMENTATION OFF"
    "BUILD_MANPAGES OFF"
    "PACKAGE_DEBUG_LIBS OFF"
    "APPLE_MULTIPLE_ARCHITECTURES OFF"
    "INCDIR OFF"
    "BINDIR OFF"
    "SBINDIR OFF"
    "LIBDIR lib"
    "DLLDIR bin"
    "MANDIR OFF"
    "CMAKEDIR OFF"
    "PKGDIR OFF"
    "DOCDIR OFF"
    "EXAMPLEDIR OFF"
    "PATCHES"
    "geographiclib.patch"
)
# Shapelib
CPMDeclarePackage(Shapelib
  NAME Shapelib
  VERSION 1.6.1
  GITHUB_REPOSITORY OSGeo/shapelib
  OPTIONS
    "BUILD_SHAPELIB_CONTRIB OFF"
    "BUILD_APPS OFF"
    "BUILD_TESTING OFF"
)
# gstreamer_good_plugins (DO NOT LOCK - version must match platform GStreamer)
# Android uses 1.22.12, macOS uses 1.24.13, etc. Locking causes header mismatches.
# CPMDeclarePackage(gstreamer_good_plugins
#   NAME gstreamer_good_plugins
#   VERSION 1.24.13
#   URL "https://gstreamer.freedesktop.org/src/gst-plugins-good/gst-plugins-good-1.24.13.tar.xz"
# )
