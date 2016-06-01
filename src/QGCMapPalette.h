/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


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
