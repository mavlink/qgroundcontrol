################################################################################
#
# (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
#
# QGroundControl is licensed according to the terms in the file
# COPYING.md in the root of the source code directory.
#
################################################################################

#
# This file contains configuration settings which are common to both the QGC Application and
# the Location Plugin. It should mainly contains initial CONFIG tag setup and compiler settings.
#

# Setup our supported build types. We do this once here and then use the defined config scopes
# to allow us to easily modify suported build types in one place instead of duplicated throughout
# the project file.

CONFIG -= debug_and_release
CONFIG += warn_on
CONFIG += resources_big
CONFIG += c++17

DEFINES += DISABLE_AIRMAP # AIRMAP SDK does not exist anymore

linux {
    linux-g++ | linux-g++-64 | linux-g++-32 | linux-clang {
        message("Linux build")
        CONFIG  += LinuxBuild
        DEFINES += __STDC_LIMIT_MACROS
        DEFINES += QGC_GST_TAISYNC_ENABLED
        DEFINES += QGC_GST_MICROHARD_ENABLED 
        linux-clang {
            message("Linux clang")
            QMAKE_CXXFLAGS += -Qunused-arguments -fcolor-diagnostics
        } else {
            #QMAKE_CXXFLAGS += -H # Handy for debugging why something is getting built when an include file is touched
            QMAKE_CXXFLAGS_WARN_ON += -Werror \
                -Wno-deprecated-copy \      # These come from mavlink headers
                -Wno-unused-parameter \     # gst_plugins-good has these errors
                -Wno-implicit-fallthrough   # gst_plugins-good has these errors
        }
    } else : linux-rasp-pi2-g++ {
        message("Linux R-Pi2 build")
        CONFIG += LinuxBuild
        DEFINES += __STDC_LIMIT_MACROS __rasp_pi2__
        DEFINES += QGC_GST_TAISYNC_ENABLED
        DEFINES += QGC_GST_MICROHARD_ENABLED 
    } else : android-clang {
        CONFIG += AndroidBuild MobileBuild
        DEFINES += __android__
        DEFINES += __STDC_LIMIT_MACROS
        DEFINES += QGC_ENABLE_BLUETOOTH
        DEFINES += QGC_GST_TAISYNC_ENABLED
        DEFINES += QGC_GST_MICROHARD_ENABLED 
        QMAKE_CXXFLAGS_WARN_ON += -Werror \
            -Wno-unused-parameter \             # gst_plugins-good has these errors
            -Wno-implicit-fallthrough \         # gst_plugins-good has these errors
            -Wno-unused-command-line-argument \ # from somewhere in Qt generated build files
            -Wno-parentheses-equality           # android gstreamer header files
        QMAKE_CFLAGS_WARN_ON += \
            -Wno-unused-command-line-argument   # from somewhere in Qt generated build files
        target.path = $$DESTDIR
        equals(ANDROID_TARGET_ARCH, armeabi-v7a)  {
            DEFINES += __androidArm32__
            message("Android Arm 32 bit build")
        } else:equals(ANDROID_TARGET_ARCH, arm64-v8a)  {
            DEFINES += __androidArm64__
            message("Android Arm 64 bit build")
        } else:equals(ANDROID_TARGET_ARCH, x86)  {
            CONFIG += Androidx86Build
            DEFINES += __androidx86__
            message("Android x86 build")
        } else {
            error("Unsupported Android architecture: $${ANDROID_TARGET_ARCH}")
        }
    } else {
        error("Unsuported Linux toolchain, only GCC 32- or 64-bit is supported")
    }
} else : win32 {
    contains(QMAKE_TARGET.arch, x86_64) {
        message("Windows build")
        CONFIG += WindowsBuild
        DEFINES += __STDC_LIMIT_MACROS
        DEFINES += QGC_GST_TAISYNC_ENABLED
        DEFINES += QGC_GST_MICROHARD_ENABLED 
        QMAKE_CFLAGS -= -Zc:strictStrings
        QMAKE_CFLAGS_RELEASE -= -Zc:strictStrings
        QMAKE_CFLAGS_RELEASE_WITH_DEBUGINFO -= -Zc:strictStrings
        QMAKE_CXXFLAGS -= -Zc:strictStrings
        QMAKE_CXXFLAGS_RELEASE -= -Zc:strictStrings
        QMAKE_CXXFLAGS_RELEASE_WITH_DEBUGINFO -= -Zc:strictStrings
        QMAKE_CXXFLAGS_WARN_ON += /WX /W3 \
            /wd4005 \   # silence warnings about macro redefinition, these come from the shapefile code with is external
            /wd4290 \   # ignore exception specifications
            /wd4267 \   # silence conversion from 'size_t' to 'int', possible loss of data, these come from gps drivers shared with px4
            /wd4100     # unreferenced formal parameter - gst-plugins-good
    } else {
        error("Unsupported Windows toolchain, only Visual Studio 2017 64 bit is supported")
    }
} else : macx {
    macx-clang | macx-llvm {
        message("Mac build")
        CONFIG  += MacBuild
        CONFIG  += x86_64
        CONFIG  -= x86
        DEFINES += QGC_GST_TAISYNC_ENABLED
        DEFINES += QGC_GST_MICROHARD_ENABLED 
        QMAKE_CXXFLAGS += -fvisibility=hidden
        QMAKE_CXXFLAGS_WARN_ON += -Werror \
            -Wno-unused-parameter \         # gst-plugins-good
            -Wno-deprecated-declarations    # eigen
    } else {
        error("Unsupported Mac toolchain, only 64-bit LLVM+clang is supported")
    }
} else : ios {
    message("iOS build")
    CONFIG  += iOSBuild MobileBuild app_bundle NoSerialBuild
    CONFIG  -= bitcode
    DEFINES += __ios__
    DEFINES += QGC_NO_GOOGLE_MAPS
    DEFINES += NO_SERIAL_LINK
    DEFINES += QGC_DISABLE_UVC
    DEFINES += QGC_GST_TAISYNC_ENABLED
    QMAKE_IOS_DEPLOYMENT_TARGET = 11.0
    QMAKE_APPLE_TARGETED_DEVICE_FAMILY = 1,2 # Universal
    QMAKE_LFLAGS += -Wl,-no_pie
} else {
    error("Unsupported build platform, only Linux, Windows, Android and Mac (Mac OS and iOS) are supported")
}

# Enable ccache where we can
linux|macx|ios {
    system(which ccache) {
        message("Found ccache, enabling")
        !ios {
            QMAKE_CXX = ccache $$QMAKE_CXX
            QMAKE_CC  = ccache $$QMAKE_CC
        } else {
            QMAKE_CXX = $$PWD/tools/iosccachecc.sh
            QMAKE_CC  = $$PWD/tools/iosccachecxx.sh
        }
    }
}

!MacBuild:!AndroidBuild {
    # See QGCPostLinkCommon.pri for details on why MacBuild doesn't use DESTDIR
    DESTDIR = staging
}

MobileBuild {
    DEFINES += __mobile__
}

StableBuild {
    message("Stable Build")
} else {
    message("Daily Build")
    DEFINES += DAILY_BUILD
}

# Set the QGC version from git
APP_VERSION_STR = vUnknown
VERSION         = 0.0.0   # Marker to indicate out-of-tree build
MAC_VERSION     = 0.0.0
MAC_BUILD       = 0
exists ($$PWD/.git) {
    GIT_DESCRIBE = $$system(git --git-dir $$PWD/.git --work-tree $$PWD describe --always --tags)
    GIT_BRANCH   = $$system(git --git-dir $$PWD/.git --work-tree $$PWD rev-parse --abbrev-ref HEAD)
    GIT_HASH     = $$system(git --git-dir $$PWD/.git --work-tree $$PWD rev-parse --short HEAD)
    GIT_TIME     = $$system(git --git-dir $$PWD/.git --work-tree $$PWD show --oneline --format=\"%ci\" -s HEAD)

    message(GIT_DESCRIBE $${GIT_DESCRIBE})

    # Pull the version info from the last annotated version tag. Format: v#.#.#
    contains(GIT_DESCRIBE, ^v[0-9]+.[0-9]+.[0-9]+.*) {
        APP_VERSION_STR = $${GIT_DESCRIBE}
        VERSION         = $$replace(GIT_DESCRIBE, "v", "")
        VERSION         = $$replace(VERSION, "-", ".")
        VERSION         = $$section(VERSION, ".", 0, 3)
    }

    DailyBuild {
        APP_VERSION_STR = "Daily $${GIT_BRANCH}:$${GIT_HASH} $${GIT_TIME}"
    }

    message(QGroundControl APP_VERSION_STR VERSION $${APP_VERSION_STR} $${VERSION})

    MacBuild {
        MAC_VERSION  = $$section(VERSION, ".", 0, 2)
        MAC_BUILD    = $$section(VERSION, ".", 3, 3)
        message(QGroundControl MAC_VERSION MAC_BUILD $${MAC_VERSION} $${MAC_BUILD})
    }
}
DEFINES += APP_VERSION_STR=\"\\\"$$APP_VERSION_STR\\\"\"

AndroidBuild {
    message(VERSION $${VERSION})
    MAJOR_VERSION   = $$section(VERSION, ".", 0, 0)
    MINOR_VERSION   = $$section(VERSION, ".", 1, 1)
    PATCH_VERSION   = $$section(VERSION, ".", 2, 2)
    DEV_VERSION     = $$section(VERSION, ".", 3, 3)

    greaterThan(MAJOR_VERSION, 9) {
        error(Major version larger than 1 digit: $${MAJOR_VERSION})
    }
    greaterThan(MINOR_VERSION, 9) {
        error(Minor version larger than 1 digit: $${MINOR_VERSION})
    }
    greaterThan(PATCH_VERSION, 99) {
        error(Patch version larger than 2 digits: $${PATCH_VERSION})
    }
    greaterThan(DEV_VERSION, 999) {
        error(Dev version larger than 3 digits: $${DEV_VERSION})
    }

    lessThan(PATCH_VERSION, 10) {
        PATCH_VERSION = $$join(PATCH_VERSION, "", "0")
    }
    equals(DEV_VERSION, "") {
        DEV_VERSION = "0"
    }
    lessThan(DEV_VERSION, 10) {
        DEV_VERSION = $$join(DEV_VERSION, "", "0")
    }
    lessThan(DEV_VERSION, 100) {
        DEV_VERSION = $$join(DEV_VERSION, "", "0")
    }

    # Bitness for android version number is 66/34 instead of 64/32 in because of a required version number bump screw-up ages ago
    equals(ANDROID_TARGET_ARCH, arm64-v8a)  {
        ANDROID_TRUE_BITNESS = 64
        ANDROID_VERSION_BITNESS = 66
    } else {
        ANDROID_TRUE_BITNESS = 32
        ANDROID_VERSION_BITNESS = 34
    }

    # Version code format: BBMIPPDDD (B=Bitness, I=Minor)
    ANDROID_VERSION_CODE = "BBMIPPDDD"
    ANDROID_VERSION_CODE = $$replace(ANDROID_VERSION_CODE, "BB", $$ANDROID_VERSION_BITNESS)
    ANDROID_VERSION_CODE = $$replace(ANDROID_VERSION_CODE, "M", $$MAJOR_VERSION)
    ANDROID_VERSION_CODE = $$replace(ANDROID_VERSION_CODE, "I", $$MINOR_VERSION)
    ANDROID_VERSION_CODE = $$replace(ANDROID_VERSION_CODE, "PP", $$PATCH_VERSION)
    ANDROID_VERSION_CODE = $$replace(ANDROID_VERSION_CODE, "DDD", $$DEV_VERSION)

    message(Android version info: $${ANDROID_VERSION_CODE} bitness:$${ANDROID_VERSION_BITNESS} major:$${MAJOR_VERSION} minor:$${MINOR_VERSION} patch:$${PATCH_VERSION} dev:$${DEV_VERSION})

    ANDROID_VERSION_NAME    = APP_VERSION_STR
}

DEFINES += EIGEN_MPL2_ONLY

# Installer configuration

installer {
    CONFIG -= debug
    CONFIG -= debug_and_release
    CONFIG += release
    message(Build Installer)
}

# Setup our supported build flavors

CONFIG(debug, debug|release) {
    message(Debug flavor)
    CONFIG += DebugBuild
} else:CONFIG(release, debug|release) {
    message(Release flavor)
    CONFIG += ReleaseBuild
} else {
    error(Unsupported build flavor)
}

# Setup our build directories

SOURCE_DIR = $$IN_PWD

LANGUAGE = C++

LOCATION_PLUGIN_DESTDIR = $${OUT_PWD}/src/QtLocationPlugin
LOCATION_PLUGIN_NAME    = QGeoServiceProviderFactoryQGC

# Turn off serial port warnings
DEFINES += _TTY_NOWARN_

MacBuild {
    QMAKE_TARGET_BUNDLE_PREFIX =    org.qgroundcontrol
    QMAKE_BUNDLE =                  qgroundcontrol
}

#
# Build-specific settings
#

ReleaseBuild {
    DEFINES += QT_NO_DEBUG QT_MESSAGELOGCONTEXT
    CONFIG += force_debug_info  # Enable debugging symbols on release builds
    !iOSBuild {
        !AndroidBuild {
            CONFIG += ltcg              # Turn on link time code generation
        }
    }

    WindowsBuild {
        # Run compilation using VS compiler using multiple threads
        QMAKE_CXXFLAGS += -MP

        # Enable function level linking and enhanced optimized debugging
        QMAKE_CFLAGS_RELEASE   += /Gy /Zo
        QMAKE_CXXFLAGS_RELEASE += /Gy /Zo
        QMAKE_CFLAGS_RELEASE_WITH_DEBUGINFO   += /Gy /Zo
        QMAKE_CXXFLAGS_RELEASE_WITH_DEBUGINFO += /Gy /Zo

        # Eliminate duplicate COMDATs
        QMAKE_LFLAGS_RELEASE += /OPT:ICF
        QMAKE_LFLAGS_RELEASE_WITH_DEBUGINFO += /OPT:ICF
    }
}
