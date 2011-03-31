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
## @file qserialportunittest.pro
## www.inbiza.com
##

include(../unittest.pri)

DEPENDPATH += . $$QSERIALPORT_INCBASE/QtSerialPort

CONFIG += qtestlib

# test target
QMAKE_EXTRA_TARGETS = test
test.depends = qserialportunittest
test.commands = ./qserialportunittest

# Input
SOURCES += qserialportunittest.cpp

HEADERS += qserialportunittest.h

MOC_DIR = $$QSERIALPORT_BUILDDIR/GeneratedFiles

CONFIG(debug, debug|release) {
  OBJECTS_DIR = $$QSERIALPORT_BUILDDIR/Debug
  macx:TARGET = $$member(TARGET, 0)_debug
  windows:TARGET = $$member(TARGET, 0)d
  unix:!macx:TARGET = $$member(TARGET, 0).debug
}
else {
  OBJECTS_DIR = $$QSERIALPORT_BUILDDIR/Release
}
