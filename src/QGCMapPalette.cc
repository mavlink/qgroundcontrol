/*=====================================================================

 QGroundControl Open Source Ground Control Station

 (c) 2009 - 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

 This file is part of the QGROUNDCONTROL project

 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

 ======================================================================*/

#include "QGCMapPalette.h"

#include <QApplication>
#include <QPalette>

QColor QGCMapPalette::_thumbJoystick[QGCMapPalette::_cColorGroups] = { QColor(255,255,255,127), QColor(0,0,0,127) };
QColor QGCMapPalette::_text         [QGCMapPalette::_cColorGroups] = { QColor(255,255,255),     QColor(0,0,0) };

QGCMapPalette::QGCMapPalette(QObject* parent) :
    QObject(parent)
{

}

void QGCMapPalette::setLightColors(bool lightColors)
{
    if ( _lightColors != lightColors) {
        _lightColors = lightColors;
        emit paletteChanged();
    }
}
