# Video streaming application for simple UDP direct byte streaming


QT       += svg network

TEMPLATE = app
TARGET = qupgrade

BASEDIR = .

LANGUAGE = C++

linux-g++|linux-g++-64{
    debug {
        TARGETDIR = $${OUT_PWD}/debug
        BUILDDIR = $${OUT_PWD}/build-debug
    }
    release {
        TARGETDIR = $${OUT_PWD}/release
        BUILDDIR = $${OUT_PWD}/build-release
    }
} else {
    TARGETDIR = $${OUT_PWD}
    BUILDDIR = $${OUT_PWD}/build
}

INCLUDEPATH += . \
    src \
    src/ui \
    src/comm \
    include/ui \
    src/apps/qupgrade \

# Input

HEADERS += \
    src/comm/SerialLink.h \
    src/comm/LinkInterface.h \
    src/comm/SerialLinkInterface.h \
    src/comm/LinkManager.h \
    src/QGC.h \
    src/apps/qupgrade/QUpgradeApp.h \
    src/apps/qupgrade/QUpgradeMainWindow.h \
    src/apps/qupgrade/uploader.h \
    libs/qextserialport/qextserialenumerator.h \
    src/ui/PX4FirmwareUpgrader.h \
    src/PX4FirmwareUpgradeWorker.h

SOURCES += \
    src/comm/SerialLink.cc \
    src/comm/LinkManager.cc \
    src/QGC.cc \
    src/apps/qupgrade/main.cc \
    src/apps/qupgrade/QUpgradeApp.cc \
    src/apps/qupgrade/QUpgradeMainWindow.cc \
    src/apps/qupgrade/uploader.cpp \
    src/ui/PX4FirmwareUpgrader.cc \
    src/PX4FirmwareUpgradeWorker.cc

FORMS += \
    src/apps/qupgrade/QUpgradeMainWindow.ui \
    src/ui/PX4FirmwareUpgrader.ui

RESOURCES = qgroundcontrol.qrc

# Include serial library functions
DEPENDPATH += . \
    plugins \
    libs/thirdParty/qserialport/include \
    libs/thirdParty/qserialport/include/QtSerialPort \
    libs/thirdParty/qserialport \
    libs/qextserialport

INCLUDEPATH += . \
    libs/thirdParty/qserialport/include \
    libs/thirdParty/qserialport/include/QtSerialPort \
    libs/thirdParty/qserialport/src \
    libs/qextserialport

# Include serial port library (QSerial)
include(qserialport.pri)

# Serial port detection (ripped-off from qextserialport library)
macx|macx-g++|macx-g++42::SOURCES += libs/qextserialport/qextserialenumerator_osx.cpp
linux-g++::SOURCES += libs/qextserialport/qextserialenumerator_unix.cpp
linux-g++-64::SOURCES += libs/qextserialport/qextserialenumerator_unix.cpp
win32::SOURCES += libs/qextserialport/qextserialenumerator_win.cpp
win32-msvc2008|win32-msvc2010::SOURCES += libs/qextserialport/qextserialenumerator_win.cpp

# -------------------------------------------------
# QGroundControl - Micro Air Vehicle Groundstation
# Please see our website at <http://qgroundcontrol.org>
# Maintainer:
# Lorenz Meier <lm@inf.ethz.ch>
# (c) 2009-2011 QGroundControl Developers
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
message(Using Qt from $$(QTDIR))

# Turn off serial port warnings
DEFINES += _TTY_NOWARN_

# MAC OS X
macx|macx-g++42|macx-g++|macx-llvm: {

        CONFIG += x86_64 cocoa phonon
        CONFIG -= x86

        QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.6

        LIBS += -framework IOKit \
                -F$$BASEDIR/libs/lib/Frameworks \
                -framework CoreFoundation \
                -framework ApplicationServices \
                -lm

        ICON = $$BASEDIR/files/images/icons/macx.icns

        # For release builds remove support for various Qt debugging macros.
        CONFIG(release, debug|release) {
                DEFINES += QT_NO_DEBUG
        }
}

# GNU/Linux
linux-g++|linux-g++-64{

        CONFIG -= console

        release {
                DEFINES += QT_NO_DEBUG
        }

        INCLUDEPATH += /usr/include \
        /usr/local/include

        # For release builds remove support for various Qt debugging macros.
        CONFIG(release, debug|release) {
                DEFINES += QT_NO_DEBUG
        }

        LIBS += \
                -L/usr/lib \
                -L/usr/local/lib64 \
                -lm

        # Validated copy commands
        !exists($$TARGETDIR){
                QMAKE_POST_LINK += && mkdir -p $$TARGETDIR
        }
        DESTDIR = $$TARGETDIR
}

linux-g++ {
        message("Building for GNU/Linux 32bit/i386")
}
linux-g++-64 {
        message("Building for GNU/Linux 64bit/x64 (g++-64)")
        exists(/usr/local/lib64) {
                LIBS += -L/usr/local/lib64
        }
}

# Windows (32bit), Visual Studio
win32-msvc2008|win32-msvc2010 {

        win32-msvc2008 {
                message(Building for Windows Visual Studio 2008 (32bit))
        }
        win32-msvc2010 {
                message(Building for Windows Visual Studio 2010 (32bit))
        }

        # Specify multi-process compilation within Visual Studio.
        # (drastically improves compilation times for multi-core computers)
        QMAKE_CXXFLAGS_DEBUG += -MP
        QMAKE_CXXFLAGS_RELEASE += -MP

        # For release builds remove support for various Qt debugging macros.
        CONFIG(release, debug|release) {
                DEFINES += QT_NO_DEBUG
        }

        # For debug releases we just want the debugging console.
        CONFIG(debug, debug|release) {
                CONFIG += console
        }

        INCLUDEPATH += $$BASEDIR/libs/lib/msinttypes

        LIBS += -lsetupapi

        RC_FILE = $$BASEDIR/qgroundcontrol.rc

        # Copy dependencies
        BASEDIR_WIN = $$replace(BASEDIR,"/","\\")
        TARGETDIR_WIN = $$replace(TARGETDIR,"/","\\")

        CONFIG(debug, debug|release) {
                # Copy application resources
                QMAKE_POST_LINK += $$quote(xcopy /D /Y "$$BASEDIR_WIN\\files" "$$TARGETDIR_WIN\\debug\\files" /E /I $$escape_expand(\\n))
                # Copy Qt DLLs
                QMAKE_POST_LINK += $$quote(xcopy /D /Y "$$(QTDIR)\\plugins" "$$TARGETDIR_WIN\\debug" /E /I $$escape_expand(\\n))
                QMAKE_POST_LINK += $$quote(xcopy /D /Y "$$(QTDIR)\\bin\\QtCored4.dll" "$$TARGETDIR_WIN\\debug"$$escape_expand(\\n))
                QMAKE_POST_LINK += $$quote(xcopy /D /Y "$$(QTDIR)\\bin\\QtGuid4.dll" "$$TARGETDIR_WIN\\debug"$$escape_expand(\\n))
                QMAKE_POST_LINK += $$quote(xcopy /D /Y "$$(QTDIR)\\bin\\QtMultimediad4.dll" "$$TARGETDIR_WIN\\debug"$$escape_expand(\\n))
                QMAKE_POST_LINK += $$quote(xcopy /D /Y "$$(QTDIR)\\bin\\QtNetworkd4.dll" "$$TARGETDIR_WIN\\debug"$$escape_expand(\\n))
                QMAKE_POST_LINK += $$quote(xcopy /D /Y "$$(QTDIR)\\bin\\QtSqld4.dll" "$$TARGETDIR_WIN\\debug"$$escape_expand(\\n))
                QMAKE_POST_LINK += $$quote(xcopy /D /Y "$$(QTDIR)\\bin\\QtSvgd4.dll" "$$TARGETDIR_WIN\\debug"$$escape_expand(\\n))
                QMAKE_POST_LINK += $$quote(xcopy /D /Y "$$(QTDIR)\\bin\\QtTestd4.dll" "$$TARGETDIR_WIN\\debug"$$escape_expand(\\n))
                QMAKE_POST_LINK += $$quote(xcopy /D /Y "$$(QTDIR)\\bin\\QtWebKitd4.dll" "$$TARGETDIR_WIN\\debug"$$escape_expand(\\n))
                QMAKE_POST_LINK += $$quote(xcopy /D /Y "$$(QTDIR)\\bin\\QtXmld4.dll" "$$TARGETDIR_WIN\\debug"$$escape_expand(\\n))
                QMAKE_POST_LINK += $$quote(xcopy /D /Y "$$(QTDIR)\\bin\\QtXmlPatternsd4.dll" "$$TARGETDIR_WIN\\debug"$$escape_expand(\\n))
        }

        CONFIG(release, debug|release) {
                # Copy application resources
                QMAKE_POST_LINK += $$quote(xcopy /D /Y "$$BASEDIR_WIN\\files" "$$TARGETDIR_WIN\\release\\files" /E /I $$escape_expand(\\n))

                # Copy Qt DLLs
                QMAKE_POST_LINK += $$quote(xcopy /D /Y "$$(QTDIR)\\plugins" "$$TARGETDIR_WIN\\release" /E /I $$escape_expand(\\n))
                QMAKE_POST_LINK += $$quote(xcopy /D /Y "$$(QTDIR)\\bin\\QtCore4.dll" "$$TARGETDIR_WIN\\release"$$escape_expand(\\n))
                QMAKE_POST_LINK += $$quote(xcopy /D /Y "$$(QTDIR)\\bin\\QtGui4.dll" "$$TARGETDIR_WIN\\release"$$escape_expand(\\n))
                QMAKE_POST_LINK += $$quote(xcopy /D /Y "$$(QTDIR)\\bin\\QtMultimedia4.dll" "$$TARGETDIR_WIN\\release"$$escape_expand(\\n))
                QMAKE_POST_LINK += $$quote(xcopy /D /Y "$$(QTDIR)\\bin\\QtNetwork4.dll" "$$TARGETDIR_WIN\\release"$$escape_expand(\\n))
                QMAKE_POST_LINK += $$quote(xcopy /D /Y "$$(QTDIR)\\bin\\QtOpenGL4.dll" "$$TARGETDIR_WIN\\release"$$escape_expand(\\n))
                QMAKE_POST_LINK += $$quote(xcopy /D /Y "$$(QTDIR)\\bin\\QtSql4.dll" "$$TARGETDIR_WIN\\release"$$escape_expand(\\n))
                QMAKE_POST_LINK += $$quote(xcopy /D /Y "$$(QTDIR)\\bin\\QtSvg4.dll" "$$TARGETDIR_WIN\\release"$$escape_expand(\\n))
                QMAKE_POST_LINK += $$quote(xcopy /D /Y "$$(QTDIR)\\bin\\QtTestd4.dll" "$$TARGETDIR_WIN\\release"$$escape_expand(\\n))
                QMAKE_POST_LINK += $$quote(xcopy /D /Y "$$(QTDIR)\\bin\\QtWebKit4.dll" "$$TARGETDIR_WIN\\release"$$escape_expand(\\n))
                QMAKE_POST_LINK += $$quote(xcopy /D /Y "$$(QTDIR)\\bin\\QtXml4.dll" "$$TARGETDIR_WIN\\release"$$escape_expand(\\n))
                QMAKE_POST_LINK += $$quote(xcopy /D /Y "$$(QTDIR)\\bin\\QtXmlPatterns4.dll" "$$TARGETDIR_WIN\\release"$$escape_expand(\\n))
                QMAKE_POST_LINK += $$quote(del /F "$$TARGETDIR_WIN\\release\\qupgrade.exp"$$escape_expand(\\n))
                QMAKE_POST_LINK += $$quote(del /F "$$TARGETDIR_WIN\\release\\qupgrade.lib"$$escape_expand(\\n))

                # Copy Visual Studio DLLs
                # Note that this is only done for release because the debugging versions of these DLLs cannot be redistributed.
                # I'm not certain of the path for VS2008, so this only works for VS2010.
                win32-msvc2010 {
                        QMAKE_POST_LINK += $$quote(xcopy /D /Y "\"C:\\Program Files \(x86\)\\Microsoft Visual Studio 10.0\\VC\\redist\\x86\\Microsoft.VC100.CRT\\*.dll\""  "$$TARGETDIR_WIN\\release\\"$$escape_expand(\\n))
                }
        }
}
