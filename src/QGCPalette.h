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

/// @file
///     @brief QGCPalette is used by QML ui to bind colors to the QGC pallete. The implementation
///             is similar to the QML SystemPalette and should be used in the same way. Refer to
///             that documentation for details. The one difference is that QGCPalette also supports
///             a light and dark theme which you can switch between.
///
///     @author Don Gagne <don@thegagnes.com>

#ifndef QGCPalette_h
#define QGCPalette_h

#include <QObject>
#include <QColor>

class QGCPalette : public QObject
{
    Q_OBJECT
    
    Q_ENUMS(ColorGroup)
    
    Q_PROPERTY(ColorGroup colorGroup READ colorGroup WRITE setColorGroup NOTIFY paletteChanged)

    Q_PROPERTY(QColor alternateBase READ alternateBase NOTIFY paletteChanged)
    Q_PROPERTY(QColor base READ base NOTIFY paletteChanged)
    Q_PROPERTY(QColor button READ button NOTIFY paletteChanged)
    Q_PROPERTY(QColor buttonText READ buttonText NOTIFY paletteChanged)
    Q_PROPERTY(QColor text READ text NOTIFY paletteChanged)
    Q_PROPERTY(QColor window READ window NOTIFY paletteChanged)
    Q_PROPERTY(QColor windowText READ windowText NOTIFY paletteChanged)
    
    /// The buttonHighlight color identifies the button background color when hovered or selected.
    Q_PROPERTY(QColor buttonHighlight READ buttonHighlight NOTIFY paletteChanged)

    /// The windowShade color should be a color somewhere between window and button. It is used to shade window
    /// areas.
    Q_PROPERTY(QColor windowShade READ windowShade NOTIFY paletteChanged)
    
    /// The windowShadeDark color should be a color somewhere between window and windowShade. It is used to shade window
    /// darker areas.
    Q_PROPERTY(QColor windowShadeDark READ windowShadeDark NOTIFY paletteChanged)
    
public:
    enum ColorGroup {
        Disabled = 0,
        Active,
        Inactive
    };
    
    enum Theme {
        Light = 0,
        Dark
    };
    
    QGCPalette(QObject* parent = NULL);
    ~QGCPalette();
    
    ColorGroup colorGroup(void) const { return _colorGroup; }
    void setColorGroup(ColorGroup colorGroup);
    
    QColor alternateBase(void) const { return _alternateBase[_theme][_colorGroup]; }
    QColor base(void) const { return _base[_theme][_colorGroup]; }
    QColor button(void) const { return _button[_theme][_colorGroup]; }
    QColor buttonText(void) const { return _buttonText[_theme][_colorGroup]; }
    QColor text(void) const { return _text[_theme][_colorGroup]; }
    QColor window(void) const { return _window[_theme][_colorGroup]; }
    QColor windowShade(void) const { return _windowShade[_theme][_colorGroup]; }
    QColor windowShadeDark(void) const { return _windowShadeDark[_theme][_colorGroup]; }
    QColor windowText(void) const { return _windowText[_theme][_colorGroup]; }
    QColor buttonHighlight(void) const { return _buttonHighlight[_theme][_colorGroup]; }
    
    static Theme globalTheme(void) { return _theme; }
    static void setGlobalTheme(Theme newTheme);
    
signals:
    void paletteChanged(void);
    
private:
    static Theme    _theme;         ///< There is a single theme for all palettes
    ColorGroup      _colorGroup;    ///< Currently selected ColorGroup
    
    static const int _cThemes = 2;
    static const int _cColorGroups = 3;
    
    static QColor _alternateBase[_cThemes][_cColorGroups];
    static QColor _base[_cThemes][_cColorGroups];
    static QColor _button[_cThemes][_cColorGroups];
    static QColor _buttonText[_cThemes][_cColorGroups];
    static QColor _text[_cThemes][_cColorGroups];
    static QColor _window[_cThemes][_cColorGroups];
    static QColor _windowShade[_cThemes][_cColorGroups];
    static QColor _windowShadeDark[_cThemes][_cColorGroups];
    static QColor _windowText[_cThemes][_cColorGroups];
    static QColor _buttonHighlight[_cThemes][_cColorGroups];
    
    void _themeChanged(void);
    
    static QList<QGCPalette*>   _paletteObjects;    ///< List of all active QGCPalette objects
};

#endif
