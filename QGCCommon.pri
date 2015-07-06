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
# the Location Plugin. It should mainly contains intial CONFIG tag setup and compiler settings.
#

# Setup our supported build types. We do this once here and then use the defined config scopes
# to allow us to easily modify suported build types in one place instead of duplicated throughout
# the project file.

linux {
    linux-g++ | linux-g++-64 {
        message("Linux build")
        CONFIG += LinuxBuild
    } else : android-g++ {
        message("Android build")
        CONFIG += AndroidBuild MobileBuild
        DEFINES += __android__
        warning("Android build is experimental and not fully functional")
    } else {
        error("Unsuported Linux toolchain, only GCC 32- or 64-bit is supported")
    }
} else : win32 {
    win32-msvc2010 | win32-msvc2012 | win32-msvc2013 {
        message("Windows build")
        CONFIG += WindowsBuild
    } else {
        error("Unsupported Windows toolchain, only Visual Studio 2010, 2012, and 2013 are supported")
    }
} else : macx {
    macx-clang | macx-llvm {
        message("Mac build")
        CONFIG += MacBuild
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
    warning("iOS build is experimental and not yet fully functional")
} else {
    error("Unsupported build platform, only Linux, Windows, Android and Mac (Mac OS and iOS) are supported")
}

MobileBuild {
    DEFINES += __mobile__
}

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

# Need to special case Windows debug_and_release since VS Project creation in this case does strange things [QTBUG-40351]
win32:debug_and_release {
    CONFIG += WindowsDebugAndRelease
}

# Setup our build directories

BASEDIR      = $$IN_PWD
DESTDIR      = $${OUT_PWD}/debug
BUILDDIR     = $${OUT_PWD}/build-debug

ReleaseBuild {
    DESTDIR  = $${OUT_PWD}/release
    BUILDDIR = $${OUT_PWD}/build-release
}

iOSBuild {
    # For whatever reason, the iOS build fails with these set. Some files have the full,
    # properly concatenaded path and file name while others have only the second portion,
    # as if BUILDDIR was empty.
    OBJECTS_DIR = $$(HOME)/tmp/qgcfoo
    MOC_DIR     = $$(HOME)/tmp/qgcfoo
    UI_DIR      = $$(HOME)/tmp/qgcfoo
    RCC_DIR     = $$(HOME)/tmp/qgcfoo
} else {
    OBJECTS_DIR = $${BUILDDIR}/obj
    MOC_DIR     = $${BUILDDIR}/moc
    UI_DIR      = $${BUILDDIR}/ui
    RCC_DIR     = $${BUILDDIR}/rcc
}

LANGUAGE = C++

AndroidBuild {
    target.path = $$DESTDIR
}

# We place the created plugin lib into the objects dir so that make clean will clean it as well
iOSBuild {
    LOCATION_PLUGIN_DESTDIR = $$(HOME)/tmp/qgcfoo
} else {
    LOCATION_PLUGIN_DESTDIR = $$OBJECTS_DIR
}

LOCATION_PLUGIN_NAME = QGeoServiceProviderFactoryQGC

message(BASEDIR $$BASEDIR DESTDIR $$DESTDIR TARGET $$TARGET OUTPUT $$OUT_PWD)

# Turn off serial port warnings
DEFINES += _TTY_NOWARN_

#
# OS Specific settings
#

AndroidBuild {
    DEFINES += __STDC_LIMIT_MACROS
}

iOSBuild {
    QMAKE_IOS_DEPLOYMENT_TARGET = 7.0
}

MacBuild {
    CONFIG += x86_64
    CONFIG -= x86
    QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.6
    QMAKE_MAC_SDK = macosx10.9
}

LinuxBuild {
	DEFINES += __STDC_LIMIT_MACROS
}

WindowsBuild {
	DEFINES += __STDC_LIMIT_MACROS
	# Specify multi-process compilation within Visual Studio.
	# (drastically improves compilation times for multi-core computers)
	QMAKE_CXXFLAGS_DEBUG += -MP
	QMAKE_CXXFLAGS_RELEASE += -MP
}

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
# We have to special case Windows debug_and_release builds because you can't have files
# which are only in the debug variant [QTBUG-40351]. So in this case we include unit tests
# even in the release variant. If you want a Windows release build with no unit tests run
# qmake with CONFIG-=debug_and_release CONFIG+=release.
#

DebugBuild|WindowsDebugAndRelease {
    DEFINES += UNITTEST_BUILD
}
