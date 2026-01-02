#include "QGCMapPalette.h"

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
