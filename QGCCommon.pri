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
linux {
    linux-g++ | linux-g++-64 | linux-g++-32 | linux-clang {
        message("Linux build")
        CONFIG  += LinuxBuild
        DEFINES += __STDC_LIMIT_MACROS
        DEFINES += QGC_GST_TAISYNC_ENABLED
        DEFINES += QGC_GST_MICROHARD_ENABLED 
        DEFINES += QGC_ENABLE_MAVLINK_INSPECTOR
        linux-clang {
            message("Linux clang")
            QMAKE_CXXFLAGS += -Qunused-arguments -fcolor-diagnostics
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
        QMAKE_CXXFLAGS += -Wno-address-of-packed-member
        QMAKE_CXXFLAGS += -Wno-unused-command-line-argument
        QMAKE_CFLAGS += -Wno-unused-command-line-argument
        QMAKE_LINK += -nostdlib++ # Hack fix?: https://forum.qt.io/topic/103713/error-cannot-find-lc-qt-5-12-android
        target.path = $$DESTDIR
        equals(ANDROID_TARGET_ARCH, armeabi-v7a)  {
            DEFINES += __androidArm32__
            DEFINES += QGC_ENABLE_MAVLINK_INSPECTOR
            message("Android Arm 32 bit build")
        } else:equals(ANDROID_TARGET_ARCH, arm64-v8a)  {
            DEFINES += __androidArm64__
            DEFINES += QGC_ENABLE_MAVLINK_INSPECTOR
            message("Android Arm 64 bit build")
        } else:equals(ANDROID_TARGET_ARCH, x86)  {
            CONFIG += Androidx86Build
            DEFINES += __androidx86__
            message("Android Arm build")
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
        CONFIG += WarningsAsErrorsOn
        DEFINES += __STDC_LIMIT_MACROS
        DEFINES += QGC_GST_TAISYNC_ENABLED
        DEFINES += QGC_GST_MICROHARD_ENABLED 
        DEFINES += QGC_ENABLE_MAVLINK_INSPECTOR
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
        DEFINES += QGC_ENABLE_MAVLINK_INSPECTOR
        equals(QT_MAJOR_VERSION, 5) | greaterThan(QT_MINOR_VERSION, 5) {
                QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.7
        } else {
                QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.6
        }
        #-- Not forcing anything. Let qmake find the latest, installed SDK.
        #QMAKE_MAC_SDK = macosx10.12
        QMAKE_CXXFLAGS += -fvisibility=hidden
        #-- Disable annoying warnings comming from mavlink.h
        QMAKE_CXXFLAGS += -Wno-address-of-packed-member
    } else {
        error("Unsupported Mac toolchain, only 64-bit LLVM+clang is supported")
    }
} else : ios {
    !equals(QT_MAJOR_VERSION, 5) | !greaterThan(QT_MINOR_VERSION, 4) {
        error("Unsupported Qt version, 5.5.x or greater is required for iOS")
    }
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

MobileBuild {
    DEFINES += __mobile__
}

# set the QGC version from git

exists ($$PWD/.git) {
    GIT_DESCRIBE = $$system(git --git-dir $$PWD/.git --work-tree $$PWD describe --always --tags)
    GIT_BRANCH   = $$system(git --git-dir $$PWD/.git --work-tree $$PWD rev-parse --abbrev-ref HEAD)
    GIT_HASH     = $$system(git --git-dir $$PWD/.git --work-tree $$PWD rev-parse --short HEAD)
    GIT_TIME     = $$system(git --git-dir $$PWD/.git --work-tree $$PWD show --oneline --format=\"%ci\" -s HEAD)

    # determine if we're on a tag matching vX.Y.Z (stable release)
    contains(GIT_DESCRIBE, v[0-9]+.[0-9]+.[0-9]+) {
        # release version "vX.Y.Z"
        GIT_VERSION = $${GIT_DESCRIBE}
        VERSION      = $$replace(GIT_DESCRIBE, "v", "")
        VERSION      = $$replace(VERSION, "-", ".")
        VERSION      = $$section(VERSION, ".", 0, 3)
    } else {
        # development version "Development branch:sha date"
        GIT_VERSION = "Development $${GIT_BRANCH}:$${GIT_HASH} $${GIT_TIME}"
        VERSION         = 0.0.0
    }

    MacBuild {
        MAC_VERSION  = $$section(VERSION, ".", 0, 2)
        MAC_BUILD    = $$section(VERSION, ".", 3, 3)
        message(QGroundControl version $${MAC_VERSION} build $${MAC_BUILD} describe $${GIT_VERSION})
    } else {
        message(QGroundControl $${GIT_VERSION})
    }
} else {
    GIT_VERSION     = None
    VERSION         = 0.0.0   # Marker to indicate out-of-tree build
    MAC_VERSION     = 0.0.0
    MAC_BUILD       = 0
}

DEFINES += GIT_VERSION=\"\\\"$$GIT_VERSION\\\"\"
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

BASEDIR      = $$IN_PWD

!iOSBuild {
    OBJECTS_DIR  = $${OUT_PWD}/obj
    MOC_DIR      = $${OUT_PWD}/moc
    UI_DIR       = $${OUT_PWD}/ui
    RCC_DIR      = $${OUT_PWD}/rcc
}

LANGUAGE = C++

LOCATION_PLUGIN_DESTDIR = $${OUT_PWD}/src/QtLocationPlugin
LOCATION_PLUGIN_NAME    = QGeoServiceProviderFactoryQGC

# Turn off serial port warnings
DEFINES += _TTY_NOWARN_

MacBuild | LinuxBuild {
    QMAKE_CXXFLAGS_WARN_ON += -Wall
    WarningsAsErrorsOn {
        QMAKE_CXXFLAGS_WARN_ON += -Werror
    }
    MacBuild {
        # Latest clang version has a buggy check for this which cause Qt headers to throw warnings on qmap.h
        QMAKE_CXXFLAGS_WARN_ON += -Wno-return-stack-address
        # Xcode 8.3 has issues on how MAVLink accesses (packed) message structure members.
        # Note that this will fail when Xcode version reaches 10.x.x
        XCODE_VERSION = $$system($$PWD/tools/get_xcode_version.sh)
        greaterThan(XCODE_VERSION, 8.2.0): QMAKE_CXXFLAGS_WARN_ON += -Wno-address-of-packed-member
    }
}

WindowsBuild {
    QMAKE_CFLAGS -= -Zc:strictStrings
    QMAKE_CFLAGS_RELEASE -= -Zc:strictStrings
    QMAKE_CFLAGS_RELEASE_WITH_DEBUGINFO -= -Zc:strictStrings
    QMAKE_CXXFLAGS -= -Zc:strictStrings
    QMAKE_CXXFLAGS_RELEASE -= -Zc:strictStrings
    QMAKE_CXXFLAGS_RELEASE_WITH_DEBUGINFO -= -Zc:strictStrings
    QMAKE_CXXFLAGS_WARN_ON += /W3 \
        /wd4996 \   # silence warnings about deprecated strcpy and whatnot, these come from the shapefile code with is external
        /wd4005 \   # silence warnings about macro redefinition, these come from the shapefile code with is external
        /wd4290 \   # ignore exception specifications
        /wd4267     # silence conversion from 'size_t' to 'int', possible loss of data, these come from gps drivers shared with px4
    WarningsAsErrorsOn {
        QMAKE_CXXFLAGS_WARN_ON += /WX
    }
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
