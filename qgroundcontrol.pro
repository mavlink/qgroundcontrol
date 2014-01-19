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
} else : macx-clang {
    message(Mac build)
    CONFIG += MacBuild
} else {
    error(Unsupported build type)
}

TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS = libs/qwt src
