/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "QGCMapPalette.h"

#include <QApplication>
#include <QPalette>

QColor QGCMapPalette::_thumbJoystick[QGCMapPalette::_cColorGroups] = { QColor(255,255,255,127), QColor(0,0,0,127) };
QColor QGCMapPalette::_text         [QGCMapPalette::_cColorGroups] = { QColor(255,255,255),     QColor(0,0,0) };
QColor QGCMapPalette::_textOutline  [QGCMapPalette::_cColorGroups] = { QColor(0,0,0,192),       QColor(255,255,255,192) };

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
