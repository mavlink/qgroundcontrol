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

/// QGCMapPalette is a variant of QGCPalette which is used to hold colors used for display over
/// the map control. Since the coloring of a satellite map differs greatly from the coloring of
/// a street map you need to be able to switch between sets of color based on map type.

#ifndef QGCMapPalette_h
#define QGCMapPalette_h

#include <QObject>
#include <QColor>

class QGCMapPalette : public QObject
{
    Q_OBJECT
    
    Q_PROPERTY(bool lightColors READ lightColors WRITE setLightColors NOTIFY paletteChanged)

    // The colors are:
    //      text            - Text color
    //      thumbJoystick   - Thumb joystick indicator

    Q_PROPERTY(QColor text          READ text           NOTIFY paletteChanged)
    Q_PROPERTY(QColor thumbJoystick READ thumbJoystick NOTIFY paletteChanged)

public:    
    QGCMapPalette(QObject* parent = NULL);
    
    bool lightColors(void) const { return _lightColors; }
    void setLightColors(bool lightColors);
    
    QColor text(void)           const { return _text[_lightColors]; }
    QColor thumbJoystick(void)  const { return _thumbJoystick[_lightColors]; }

signals:
    void paletteChanged(void);
    
private:
    bool _lightColors;

    static const int _cColorGroups = 2;

    static QColor _thumbJoystick[_cColorGroups];
    static QColor _text[_cColorGroups];
};

#endif
