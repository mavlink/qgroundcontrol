# CPM Package Lock
# This file should be committed to version control

# ulog_cpp (unversioned)
# CPMDeclarePackage(ulog_cpp
#  NAME ulog_cpp
#  GIT_TAG main
#  GITHUB_REPOSITORY PX4/ulog_cpp
#)
# EXPAT
CPMDeclarePackage(EXPAT
  NAME EXPAT
  VERSION 2.7.0
  GIT_TAG R_2_7_0
  GITHUB_REPOSITORY libexpat/libexpat
  SOURCE_SUBDIR expat
  OPTIONS
    "EXPAT_BUILD_DOCS OFF"
    "EXPAT_BUILD_EXAMPLES OFF"
    "EXPAT_BUILD_FUZZERS OFF"
    "EXPAT_BUILD_PKGCONFIG OFF"
    "EXPAT_BUILD_TESTS OFF"
    "EXPAT_BUILD_TOOLS OFF"
    "EXPAT_ENABLE_INSTALL OFF"
    "EXPAT_SHARED_LIBS OFF"
)
# exiv2
CPMDeclarePackage(exiv2
  NAME exiv2
  VERSION 0.28.5
  GITHUB_REPOSITORY Exiv2/exiv2
  OPTIONS
    "EXIV2_ENABLE_XMP ON"
    "EXIV2_ENABLE_EXTERNAL_XMP OFF"
    "EXIV2_ENABLE_PNG OFF"
    "EXIV2_ENABLE_NLS OFF"
    "EXIV2_ENABLE_LENSDATA OFF"
    "EXIV2_ENABLE_DYNAMIC_RUNTIME OFF"
    "EXIV2_ENABLE_WEBREADY OFF"
    "EXIV2_ENABLE_CURL OFF"
    "EXIV2_ENABLE_BMFF OFF"
    "EXIV2_ENABLE_BROTLI OFF"
    "EXIV2_ENABLE_VIDEO OFF"
    "EXIV2_ENABLE_INIH OFF"
    "EXIV2_ENABLE_FILESYSTEM_ACCESS OFF"
    "EXIV2_BUILD_SAMPLES OFF"
    "EXIV2_BUILD_EXIV2_COMMAND OFF"
    "EXIV2_BUILD_UNIT_TESTS OFF"
    "EXIV2_BUILD_FUZZ_TESTS OFF"
    "EXIV2_BUILD_DOC OFF"
    "BUILD_WITH_CCACHE OFF"
)
# px4-gpsdrivers (unversioned)
# CPMDeclarePackage(px4-gpsdrivers
#  NAME px4-gpsdrivers
#  GIT_TAG main
#  GITHUB_REPOSITORY PX4/PX4-GPSDrivers
#  SOURCE_SUBDIR src
#)
# sdl_gamecontrollerdb (unversioned)
# CPMDeclarePackage(sdl_gamecontrollerdb
#  NAME sdl_gamecontrollerdb
#  GIT_TAG master
#  GITHUB_REPOSITORY mdqinc/SDL_GameControllerDB
#)
# SDL2
CPMDeclarePackage(SDL2
  NAME SDL2
  VERSION 2.32.4
  GIT_TAG release-2.32.4
  GITHUB_REPOSITORY libsdl-org/SDL
  OPTIONS
    "SDL2_DISABLE_INSTALL ON"
    "SDL2_DISABLE_INSTALL ON"
    "SDL2_DISABLE_UNINSTALL ON"
    "SDL2_DISABLE_SDL2MAIN ON"
    "SDL_SHARED OFF"
    "SDL_STATIC ON"
    "SDL_TEST OFF"
    "SDL_ATOMIC ON"
    "SDL_AUDIO OFF"
    "SDL_CPUINFO ON"
    "SDL_EVENTS ON"
    "SDL_FILE OFF"
    "SDL_FILESYSTEM OFF"
    "SDL_HAPTIC ON"
    "SDL_HIDAPI ON"
    "SDL_JOYSTICK ON"
    "SDL_LOADSO ON"
    "SDL_LOCALE OFF"
    "SDL_MISC OFF"
    "SDL_POWER ON"
    "SDL_RENDER OFF"
    "SDL_SENSOR OFF"
    "SDL_THREADS ON"
    "SDL_TIMERS OFF"
    "SDL_VIDEO OFF"
    "SDL_3DNOW OFF"
    "SDL_DBUS OFF"
    "SDL_IBUS OFF"
    "SDL_MMX OFF"
    "SDL_VIRTUAL_JOYSTICK ON"
)
# mavlink (unversioned)
# CPMDeclarePackage(mavlink
#  NAME mavlink
#  GIT_TAG 19f9955598af9a9181064619bd2e3c04bd2d848a
#  GIT_REPOSITORY https://github.com/mavlink/c_library_v2.git
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
# gstreamer_good_plugins
CPMDeclarePackage(gstreamer_good_plugins
  NAME gstreamer_good_plugins
  VERSION 1.24.12
  URL
    "https://gstreamer.freedesktop.org/src/gst-plugins-good/gst-plugins-good-1.24.12.tar.xz"
)
