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
## @file echo_pollingmode.pro
## www.inbiza.com
##

include(../examples.pri)

DEPENDPATH += . $$QSERIALPORT_INCBASE/QtSerialPort

QT += network
!windows:TARGET = $$QSERIALPORT_BINDESTDIR/$$TARGET

# Input
SOURCES += main.cpp

#HEADERS +=

MOC_DIR = $$QSERIALPORT_BUILDDIR/GeneratedFiles/polling

CONFIG(debug, debug|release) {
  OBJECTS_DIR = $$QSERIALPORT_BUILDDIR/Debug/polling
  macx:TARGET = $$member(TARGET, 0)_debug
  windows:TARGET = $$member(TARGET, 0)d
  unix:!macx:TARGET = $$member(TARGET, 0).debug
}
else {
  OBJECTS_DIR = $$QSERIALPORT_BUILDDIR/Release/polling
}
