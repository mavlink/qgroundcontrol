# -------------------------------------------------
# QGroundControl - Micro Air Vehicle Groundstation
# Please see our website at <http://qgroundcontrol.org>
# Maintainer:
# Lorenz Meier <lm@inf.ethz.ch>
# (c) 2009-2014 QGroundControl Developers
# This file is part of the open groundstation project
# QGroundControl is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# QGroundControl is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
# You should have received a copy of the GNU General Public License
# along with QGroundControl. If not, see <http://www.gnu.org/licenses/>.
# -------------------------------------------------

#
# This file contains configuration settings which are common to both the QGC Application and
# the Location Plugin. It should mainly contains initial CONFIG tag setup and compiler settings.
#

# Setup our supported build types. We do this once here and then use the defined config scopes
# to allow us to easily modify suported build types in one place instead of duplicated throughout
# the project file.

linux {
    linux-g++ | linux-g++-64 | linux-g++-32 {
        message("Linux build")
        CONFIG += LinuxBuild
        DEFINES += __STDC_LIMIT_MACROS
    } else : linux-rasp-pi2-g++ {
        message("Linux R-Pi2 build")
        CONFIG += LinuxBuild
        DEFINES += __STDC_LIMIT_MACROS __rasp_pi2__
    } else : android-g++ {
        message("Android build")
        CONFIG += AndroidBuild MobileBuild
        DEFINES += __android__
        DEFINES += __STDC_LIMIT_MACROS
        DEFINES += QGC_ENABLE_BLUETOOTH
        target.path = $$DESTDIR
    } else {
        error("Unsuported Linux toolchain, only GCC 32- or 64-bit is supported")
    }
} else : win32 {
    win32-msvc2010 | win32-msvc2012 | win32-msvc2013 {
        message("Windows build")
        CONFIG += WindowsBuild
        DEFINES += __STDC_LIMIT_MACROS
    } else {
        error("Unsupported Windows toolchain, only Visual Studio 2010, 2012, and 2013 are supported")
    }
} else : macx {
    macx-clang | macx-llvm {
        message("Mac build")
        CONFIG += MacBuild
        DEFINES += __macos__
        CONFIG += x86_64
        CONFIG -= x86
equals(QT_MAJOR_VERSION, 5) | greaterThan(QT_MINOR_VERSION, 5) {
        QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.7
} else {
        QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.6
}
        QMAKE_MAC_SDK = macosx10.11
        QMAKE_CXXFLAGS += -fvisibility=hidden
    } else {
        error("Unsupported Mac toolchain, only 64-bit LLVM+clang is supported")
    }
} else : ios {
    !equals(QT_MAJOR_VERSION, 5) | !greaterThan(QT_MINOR_VERSION, 4) {
        error("Unsupported Qt version, 5.5.x or greater is required for iOS")
    }
    message("iOS build")
    CONFIG += iOSBuild MobileBuild app_bundle
    DEFINES += __ios__
    QMAKE_IOS_DEPLOYMENT_TARGET = 8.0
    QMAKE_IOS_TARGETED_DEVICE_FAMILY = 1,2 # Universal
    QMAKE_LFLAGS += -Wl,-no_pie
} else {
    error("Unsupported build platform, only Linux, Windows, Android and Mac (Mac OS and iOS) are supported")
}

MobileBuild {
    DEFINES += __mobile__
}

# set the QGC version from git

exists ($$PWD/.git) {
    GIT_DESCRIBE = $$system(git --git-dir $$PWD/.git --work-tree $$PWD describe --always --tags)
    GIT_HASH     = $$system(git rev-parse HEAD)
    VERSION      = $$replace(GIT_DESCRIBE, "v", "")
    VERSION      = $$replace(VERSION, "-", ".")
    VERSION      = $$section(VERSION, ".", 0, 3)
    MacBuild {
        MAC_VERSION  = $$section(VERSION, ".", 0, 2)
        MAC_BUILD    = $$section(VERSION, ".", 3, 3)
        message(QGroundControl version $${MAC_VERSION} build $${MAC_BUILD} describe $${GIT_DESCRIBE} hash $${GIT_HASH})
    } else {
        message(QGroundControl version $${VERSION} describe $${GIT_DESCRIBE} hash $${GIT_HASH})
    }
} else {
    GIT_DESCRIBE    = None
    GIT_HASH        = None
    VERSION         = 0.0.0   # Marker to indicate out-of-tree build
    MAC_VERSION     = 0.0.0
    MAC_BUILD       = 0
}

DEFINES += GIT_TAG=\"\\\"$$GIT_DESCRIBE\\\"\"
DEFINES += GIT_HASH=\"\\\"$$GIT_HASH\\\"\"

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
    }
}

WindowsBuild {
    QMAKE_CXXFLAGS_WARN_ON += /W3 \
        /wd4996 \   # silence warnings about deprecated strcpy and whatnot
        /wd4005 \   # silence warnings about macro redefinition
        /wd4290 \   # ignore exception specifications
        /Zc:strictStrings-  # work around win 8.1 sdk sapi.h problem
    WarningsAsErrorsOn {
        QMAKE_CXXFLAGS_WARN_ON += /WX
    }
}

#
# Build-specific settings
#

ReleaseBuild {
    DEFINES += QT_NO_DEBUG
    WindowsBuild {
        # Use link time code generation for better optimization (I believe this is supported in MSVC Express, but not 100% sure)
        QMAKE_LFLAGS_LTCG = /LTCG
        QMAKE_CFLAGS_LTCG = -GL

        # Turn on debugging information so we can collect good crash dumps from release builds
        QMAKE_CXXFLAGS_RELEASE += /Zi
        QMAKE_LFLAGS_RELEASE += /DEBUG
    }
}

#
# Unit Test specific configuration goes here
#

DebugBuild {
    DEFINES += UNITTEST_BUILD
}
