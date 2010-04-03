# -------------------------------------------------
# MAVGround - Micro Air Vehicle Groundstation
# Please see our website at <http://pixhawk.ethz.ch>
# Original Author:
# Lorenz Meier <mavteam@student.ethz.ch>
# Contributing Authors (in alphabetical order):
# (c) 2009 PIXHAWK Team
# This file is part of the mav groundstation project
# MAVGround is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# MAVGround is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
# You should have received a copy of the GNU General Public License
# along with MAVGround. If not, see <http://www.gnu.org/licenses/>.
# ABOUT THIS FILE:
# This file includes the MAVLink protocol from outside the groundstation directory
# Change the MAVLINKDIR to whereever the MAVLink .c and .h files are located.
# DO NOT USE A TRAILING SLASH!
# ------------------------------------------------------------------------------
MAVLINKDIR = ../embedded/src/comm/mavlink
INCLUDEPATH += $$MAVLINKDIR
HEADERS += \
    $$MAVLINKDIR/mavlink.h
SOURCES += $$MAVLINKDIR/protocol.c
