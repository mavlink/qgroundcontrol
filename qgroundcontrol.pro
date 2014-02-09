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

message(Qt version $$[QT_VERSION])

# Setup our supported build types. We do this once here and then use the defined config scopes
# to allow us to easily modify suported build types in one place instead of duplicated throughout
# the project file.

linux-g++ | linux-g++-64 {
    message(Linux build)
    CONFIG += LinuxBuild
} else : win32-msvc2008 | win32-msvc2010 | win32-msvc2012 {
    message(Windows build)
    CONFIG += WindowsBuild
} else : macx-clang | macx-llvm {
    message(Mac build)
    CONFIG += MacBuild
} else {
    error(Unsupported build type)
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

BASEDIR = $${IN_PWD}
DebugBuild {
    DESTDIR = $${OUT_PWD}/debug
    BUILDDIR = $${OUT_PWD}/build-debug
}
ReleaseBuild {
    DESTDIR = $${OUT_PWD}/release
    BUILDDIR = $${OUT_PWD}/build-release
}
OBJECTS_DIR = $${BUILDDIR}/obj
MOC_DIR = $${BUILDDIR}/moc
UI_DIR = $${BUILDDIR}/ui
RCC_DIR = $${BUILDDIR}/rcc
LANGUAGE = C++

message(BASEDIR $$BASEDIR DESTDIR $$DESTDIR TARGET $$TARGET)

# Qt configuration
CONFIG += qt \
    thread

QT += network \
    opengl \
    svg \
    xml \
    phonon \
    webkit \
    sql \
    declarative
        
#  testlib is needed even in release flavor for QSignalSpy support
QT += testlib

# Turn off serial port warnings
DEFINES += _TTY_NOWARN_

#
# OS Specific settings
#

MacBuild {
    QMAKE_INFO_PLIST = Custom-Info.plist
    CONFIG += x86_64
    CONFIG -= x86
	QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.6
	ICON = $$BASEDIR/files/images/icons/macx.icns
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

	# QWebkit is not needed on MS-Windows compilation environment
	CONFIG -= webkit

	RC_FILE = $$BASEDIR/qgroundcontrol.rc
}

#
# Warnings cleanup. Plan of attack is to turn off all existing warnings and turn on warnings as errors.
# Then we will clean up the warnings one type at a time, removing the override for that specific warning
# from the lists below. Eventually we will be left with no overlooked warnings and all future warnings
# generating an error and breaking the build.
#
# NEW WARNINGS SHOULD NOT BE ADDED TO THIS LIST. IF YOU GET AN ERROR, FIX IT BEFORE COMMITING.
#

MacBuild | LinuxBuild {
	QMAKE_CXXFLAGS_WARN_ON += \
        -Wall \
        -Wno-unused-parameter \
        -Wno-unused-variable \
        -Wno-narrowing \
        -Wno-unused-function
 }

LinuxBuild {
	QMAKE_CXXFLAGS_WARN_ON += \
        -Wno-unused-but-set-variable \
        -Wno-unused-local-typedefs
}

MacBuild {
	QMAKE_CXXFLAGS_WARN_ON += \
        -Wno-overloaded-virtual \
        -Wno-unused-private-field
}

WindowsBuild {
	QMAKE_CXXFLAGS_WARN_ON += \
        /W4 \
        /WX \
        /wd4005 \ # macro redefinition
        /wd4100 \ # unrefernced formal parameter
        /wd4101 \ # unreference local variable
        /wd4127 \ # conditional expression constant
        /wd4146 \ # unary minus operator applied to unsigned type
        /wd4189 \ # local variable initialized but not used
        /wd4201 \ # non standard extension: nameless struct/union
        /wd4245 \ # signed/unsigned mismtach
        /wd4290 \ # function declared using exception specification, but not supported
        /wd4305 \ # truncation from double to float
        /wd4309 \ # truncation of constant value
        /wd4389 \ # == signed/unsigned mismatch
        /wd4505 \ # unreferenced local function
        /wd4512 \ # assignment operation could not be generated
        /wd4701 \ # potentially uninitialized local variable
        /wd4702 \ # unreachable code
        /wd4996   # deprecated function
}

#
# Build flavor specific settings
#

DebugBuild {
    CONFIG += console
}

ReleaseBuild {
    DEFINES += QT_NO_DEBUG

	WindowsBuild {
		# Use link time code generation for beteer optimization (I believe this is supported in msvc express, but not 100% sure)
		QMAKE_LFLAGS_LTCG = /LTCG
		QMAKE_CFLAGS_LTCG = -GL
    }
}

#
# Unit Test specific configuration goes here (debug only)
#

DebugBuild {
    INCLUDEPATH += \
        src/qgcunittest

    HEADERS += \
        src/qgcunittest/AutoTest.h \
        src/qgcunittest/UASUnitTest.h \
        src/qgcunittest/MockUASManager.h \
        src/qgcunittest/MockUAS.h \
        src/qgcunittest/MockQGCUASParamManager.h \
        src/qgcunittest/MultiSignalSpy.h \
        src/qgcunittest/TCPLinkTest.h \
        src/qgcunittest/FlightModeConfigTest.h

    SOURCES += \
        src/qgcunittest/UASUnitTest.cc \
        src/qgcunittest/MockUASManager.cc \
        src/qgcunittest/MockUAS.cc \
        src/qgcunittest/MockQGCUASParamManager.cc \
        src/qgcunittest/MultiSignalSpy.cc \
        src/qgcunittest/TCPLinkTest.cc \
        src/qgcunittest/FlightModeConfigTest.cc
}

#
# External library configuration
#

include(QGCExternalLibs.pri)

#
# Post link configuration
#

include(QGCSetup.pri)

#
# Main QGroundControl portion of project file
#

RESOURCES += qgroundcontrol.qrc

TRANSLATIONS += \
    es-MX.ts \
    en-US.ts
    
DEPENDPATH += \
    . \
    plugins

INCLUDEPATH += .

INCLUDEPATH += \
    src \
    src/ui \
    src/ui/linechart \
    src/ui/uas \
    src/ui/map \
    src/uas \
    src/comm \
    include/ui \
    src/input \
    src/lib/qmapcontrol \
    src/ui/mavlink \
    src/ui/param \
    src/ui/watchdog \
    src/ui/map3D \
    src/ui/mission \
    src/ui/designer \
    src/ui/configuration \
    src/ui/main

FORMS += \

HEADERS += \

SOURCES += \
