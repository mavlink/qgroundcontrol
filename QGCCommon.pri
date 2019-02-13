# -------------------------------------------------
# QGroundControl - Micro Air Vehicle Groundstation
# Please see our website at <http://qgroundcontrol.org>
# Maintainer:
# Lorenz Meier <lm@inf.ethz.ch>
# (c) 2009-2014 QGroundControl Developers
# License terms set in COPYING.md
# -------------------------------------------------

#
# This file contains configuration settings which are common to both the QGC Application and
# the Location Plugin. It should mainly contains initial CONFIG tag setup and compiler settings.
#

# Setup our supported build types. We do this once here and then use the defined config scopes
# to allow us to easily modify suported build types in one place instead of duplicated throughout
# the project file.

linux {
    linux-g++ | linux-g++-64 | linux-g++-32 | linux-clang {
        message("Linux build")
        CONFIG  += LinuxBuild
        DEFINES += __STDC_LIMIT_MACROS
        DEFINES += QGC_GST_TAISYNC_ENABLED
        linux-clang {
            message("Linux clang")
            QMAKE_CXXFLAGS += -Qunused-arguments -fcolor-diagnostics
        }
    } else : linux-rasp-pi2-g++ {
        message("Linux R-Pi2 build")
        CONFIG += LinuxBuild
        DEFINES += __STDC_LIMIT_MACROS __rasp_pi2__
        DEFINES += QGC_GST_TAISYNC_ENABLED
    } else : android-g++ | android-clang {
        CONFIG += AndroidBuild MobileBuild
        DEFINES += __android__
        DEFINES += __STDC_LIMIT_MACROS
        DEFINES += QGC_ENABLE_BLUETOOTH
        DEFINES += QGC_GST_TAISYNC_ENABLED
        target.path = $$DESTDIR
        equals(ANDROID_TARGET_ARCH, x86)  {
            CONFIG += Androidx86Build
            DEFINES += __androidx86__
            message("Android x86 build")
        } else {
            message("Android Arm build")
        }
    } else {
        error("Unsuported Linux toolchain, only GCC 32- or 64-bit is supported")
    }
} else : win32 {
    win32-msvc2015 {
        message("Windows build")
        CONFIG += WindowsBuild
        DEFINES += __STDC_LIMIT_MACROS
        DEFINES += QGC_GST_TAISYNC_ENABLED
    } else {
        error("Unsupported Windows toolchain, only Visual Studio 2015 is supported")
    }
} else : macx {
    macx-clang | macx-llvm {
        message("Mac build")
        CONFIG  += MacBuild
        CONFIG  += x86_64
        CONFIG  -= x86
        DEFINES += QGC_GST_TAISYNC_ENABLED
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
    QMAKE_IOS_DEPLOYMENT_TARGET = 8.0
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
    } else {
        # development version "Development branch:sha date"
        GIT_VERSION = "Development $${GIT_BRANCH}:$${GIT_HASH} $${GIT_TIME}"
    }

    VERSION      = $$replace(GIT_DESCRIBE, "v", "")
    VERSION      = $$replace(VERSION, "-", ".")
    VERSION      = $$section(VERSION, ".", 0, 3)
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

#
# By default warnings as errors are turned off. Even so, in order for a pull request
# to be accepted you must compile cleanly with warnings as errors turned on the default
# set of OS builds. See http://www.qgroundcontrol.org/dev/contribute for more details.
# You can use the WarningsAsErrorsOn CONFIG switch to turn warnings as errors on for your
# own builds.
#

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
    win32-msvc2015 {
        QMAKE_CFLAGS -= -Zc:strictStrings
        QMAKE_CXXFLAGS -= -Zc:strictStrings
    }
    QMAKE_CFLAGS_RELEASE -= -Zc:strictStrings
    QMAKE_CFLAGS_RELEASE_WITH_DEBUGINFO -= -Zc:strictStrings

    QMAKE_CXXFLAGS_RELEASE -= -Zc:strictStrings
    QMAKE_CXXFLAGS_RELEASE_WITH_DEBUGINFO -= -Zc:strictStrings
    QMAKE_CXXFLAGS_WARN_ON += /W3 \
        /wd4996 \   # silence warnings about deprecated strcpy and whatnot
        /wd4005 \   # silence warnings about macro redefinition
        /wd4290     # ignore exception specifications

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
