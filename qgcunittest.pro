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


TARGET = qgcunittest
QT += testlib

QGCS_UNITTEST_OVERRIDE = true
include (qgroundcontrol.pro)

TESTDIR = $$BASEDIR/src/qgcunittest

HEADERS += \
    $$TESTDIR/AutoTest.h \
    $$TESTDIR/UASUnitTest.h \

SOURCES += \
    $$TESTDIR/testSuite.cc \
    $$TESTDIR/UASUnitTest.cc