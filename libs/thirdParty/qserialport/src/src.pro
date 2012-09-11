##
## Unofficial Qt Serial Port Library
##
## Copyright (c) 2010 Inbiza Systems Inc. All rights reserved.
##
## This program is free software: you can redistribute it and/or modify it
## under the terms of the GNU Lesser General Public License as published by the
## Free Software Foundation, either version 3 of the License, or (at your
## option) any later version.
##
## This program is distributed in the hope that it will be useful, but WITHOUT
## ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
## FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
## more details.
##
## You should have received a copy of the GNU Lesser General Public License
## along with this program.  If not, see <http://www.gnu.org/licenses/>
##
##
## @file src.pro
## www.inbiza.com
##

include(../conf.pri)
include(../base.pri)

TEMPLATE = lib
QT      -= gui
TARGET   = QtSerialPort
DESTDIR  = $$QSERIALPORT_LIBDIR
windows:DLLDESTDIR = $$QSERIALPORT_LIBDIR

VERSION = 1.0.0

CONFIG += debug_and_release thread build_all
CONFIG += create_prl

windows|wince*:!staticlib:!static:DEFINES += QSERIALPORT_MAKEDLL
staticlib|static:PRL_EXPORT_DEFINES += QSERIALPORT_STATIC

QSERIALPORT_INC = $$QSERIALPORT_INCBASE/QtSerialPort
QSERIALPORT_CPP = $$QSERIALPORT_SRCBASE
INCLUDEPATH += $$QSERIALPORT_INC $$QSERIALPORT_CPP

unix: {
  PRIVATE_HEADERS += \
    $$QSERIALPORT_SRCBASE/posix/termioshelper.h
}
windows: {
  PRIVATE_HEADERS += \
    $$QSERIALPORT_SRCBASE/win32/commdcbhelper.h \
    $$QSERIALPORT_SRCBASE/win32/qwincommevtnotifier.h
  wince*: {
    PRIVATE_HEADERS += $$QSERIALPORT_SRCBASE/win32/wincommevtbreaker.h
  }
}
PUBLIC_HEADERS += \
    $$QSERIALPORT_INC/qserialport.h \
    $$QSERIALPORT_INC/qserialportnative.h \
    $$QSERIALPORT_INC/qserialport_export.h \
    $$QSERIALPORT_INC/qportsettings.h

HEADERS += $$PRIVATE_HEADERS $$PUBLIC_HEADERS

SOURCES += \
    $$QSERIALPORT_CPP/common/qportsettings.cpp \
    $$QSERIALPORT_CPP/common/qserialport.cpp

unix: {
  SOURCES += \
    $$QSERIALPORT_CPP/posix/qserialportnative_posix.cpp \
    $$QSERIALPORT_CPP/posix/termioshelper.cpp
}
windows: {
  !wince*: {
    SOURCES += $$QSERIALPORT_CPP/win32/qserialportnative_win32.cpp
  }
  else {
    SOURCES += \
      $$QSERIALPORT_CPP/win32/qserialportnative_wince.cpp \
      $$QSERIALPORT_CPP/win32/wincommevtbreaker.cpp
  }
  SOURCES += \
    $$QSERIALPORT_CPP/win32/commdcbhelper.cpp \
    $$QSERIALPORT_CPP/win32/qwincommevtnotifier.cpp
}

# create framework on macosx if specified.

macx:lib_bundle: {
  QMAKE_FRAMEWORK_BUNDLE_NAME = $$TARGET
  CONFIG(debug, debug|release) {
    !build_pass:CONFIG += build_all
  }
  else { #release
    !debug_and_release|build_pass {
      FRAMEWORK_HEADERS.version = Versions
      FRAMEWORK_HEADERS.files = $$PUBLIC_HEADERS $$QSERIALPORT_INC/QSerialPort
      FRAMEWORK_HEADERS.path = Headers
      QMAKE_BUNDLE_DATA += FRAMEWORK_HEADERS
    }
  }
  framework_dir = /Library/Frameworks
  target.path = $$framework_dir
  INSTALLS += target
}

macx: {
  QMAKE_LFLAGS_SONAME = -Wl,-install_name,"$$DESTDIR/"
}
unix:!macx {
  QMAKE_LFLAGS_RPATH = -Wl,-rpath,"$$DESTDIR/"
}

unix:!lib_bundle: {
  # install
  QSERPORT_INSTALL_DIR = /usr/local/qserialport
  QSERPORT_INSTALL_LIBDIR = $$QSERPORT_INSTALL_DIR/lib
  QSERPORT_INSTALL_INCDIR = $$QSERPORT_INSTALL_DIR/include

  CONFIG += copy_dir_files
  message( install dir: $$QSERPORT_INSTALL_DIR )

  target.path = $$QSERPORT_INSTALL_LIBDIR
  INSTALLS += target

  incfiles.path = $$QSERPORT_INSTALL_INCDIR/QtSerialPort
  incfiles.files = $$PUBLIC_HEADERS $$QSERIALPORT_INC/QSerialPort
  !lib_bundle:INSTALLS += incfiles

  manfiles.path = $$DATADIR/man/man1
  manfiles.files = $$QSERIALPORT_BASE/man/qserialport1.0
  INSTALLS += manfiles
}

!debug_and_release|build_pass {
  MOC_DIR = $$QSERIALPORT_BUILDDIR/GeneratedFiles
  CONFIG(debug, debug|release) {
    OBJECTS_DIR = $$QSERIALPORT_BUILDDIR/Debug
    macx: {
      TARGET = $$join(TARGET,,,_debug)
    }
    windows: {
      TARGET = $$join(TARGET,,,d)
    }
    unix:!macx: {
      TARGET = $$join(TARGET,,,.debug)
    }
  } else { # release
     OBJECTS_DIR = $$QSERIALPORT_BUILDDIR/Release
  }
}
