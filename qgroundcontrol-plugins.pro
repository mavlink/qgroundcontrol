# -------------------------------------------------
# QGroundControl - Micro Air Vehicle Groundstation
# Please see our website at <http://qgroundcontrol.org>
# Author:
# Lorenz Meier <mavteam@student.ethz.ch>
# (c) 2009-2010 PIXHAWK Team
# This file is part of the mav groundstation project
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
# Include QMapControl map library
# prefer version from external directory /
# from http://github.com/pixhawk/qmapcontrol/
# over bundled version in lib directory
# Version from GIT repository is preferred
# include ( "../qmapcontrol/QMapControl/QMapControl.pri" ) #{
# Include bundled version if necessary

 CONFIG      += designer plugin
 TARGET      = $$qtLibraryTarget($$TARGET)
 TEMPLATE    = lib
 QTDIR_build:DESTDIR     = $$QT_BUILD_TREE/plugins/designer

 FORMS       = src/ui/designer/QGCParamSlider.ui

 HEADERS     = src/ui/designer/QGCParamSlider.h \
               src/ui/designer/QGCParamSliderPlugin.h
 SOURCES     = src/ui/designer/QGCParamSlider.cc \
               src/ui/designer/QGCParamSliderPlugin.cc

 # install
 target.path = $$[QT_INSTALL_PLUGINS]/designer
 sources.files = $$SOURCES $$HEADERS *.pro
 sources.path = $$[QT_INSTALL_EXAMPLES]/designer/worldtimeclockplugin
 INSTALLS += target

 symbian: include($$QT_SOURCE_TREE/examples/symbianpkgrules.pri)
