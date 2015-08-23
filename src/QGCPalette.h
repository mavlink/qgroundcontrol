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
    
    Q_ENUMS(Theme)
    
    Q_PROPERTY(Theme globalTheme READ globalTheme WRITE setGlobalTheme NOTIFY paletteChanged)
    
    Q_PROPERTY(bool colorGroupEnabled READ colorGroupEnabled WRITE setColorGroupEnabled NOTIFY paletteChanged)

    /// Background color for windows
    Q_PROPERTY(QColor window READ window NOTIFY paletteChanged)
    
    /// Color for shaded areas within a window. The windowShade color should be a color somewhere between window and button.
    Q_PROPERTY(QColor windowShade READ windowShade NOTIFY paletteChanged)
    
    /// Color for darker shared areas within a window. The windowShadeDark color should be a color somewhere between window and windowShade.
    Q_PROPERTY(QColor windowShadeDark READ windowShadeDark NOTIFY paletteChanged)
    
    /// Standard text color for label text
    Q_PROPERTY(QColor text READ text NOTIFY paletteChanged)
    
	/// Color for warning text
	Q_PROPERTY(QColor warningText READ warningText NOTIFY paletteChanged)

	/// Background color for buttons
    Q_PROPERTY(QColor button READ button NOTIFY paletteChanged)
    
    /// Text color for buttons
    Q_PROPERTY(QColor buttonText READ buttonText NOTIFY paletteChanged)
    
    /// Background color for button in selected or hover state
    Q_PROPERTY(QColor buttonHighlight READ buttonHighlight NOTIFY paletteChanged)
    
    /// Text color for button in selected or hover state
    Q_PROPERTY(QColor buttonHighlightText READ buttonHighlightText NOTIFY paletteChanged)

    /// Background color for primary buttons. A primary button is the button which would be the
    /// normal default  button to press. For example in an Ok/Cancel situation where Ok would normally
    /// be pressed, Ok is the primary button.
    Q_PROPERTY(QColor primaryButton READ primaryButton NOTIFY paletteChanged)
    
    /// Text color for buttons
    Q_PROPERTY(QColor primaryButtonText READ primaryButtonText NOTIFY paletteChanged)
    
    // Background color for TextFields
    Q_PROPERTY(QColor textField READ textField NOTIFY paletteChanged)
    
    // Text color for TextFields
    Q_PROPERTY(QColor textFieldText READ textFieldText NOTIFY paletteChanged)
    
public:
    enum ColorGroup {
        Disabled = 0,
        Enabled
    };
    
    enum Theme {
        Light = 0,
        Dark
    };
    
    QGCPalette(QObject* parent = NULL);
    ~QGCPalette();
    
    bool colorGroupEnabled(void) const { return _colorGroupEnabled; }
    void setColorGroupEnabled(bool enabled);
    
    QColor window(void) const { return _window[_theme][_colorGroupEnabled ? 1 : 0]; }
    QColor windowShade(void) const { return _windowShade[_theme][_colorGroupEnabled ? 1 : 0]; }
    QColor windowShadeDark(void) const { return _windowShadeDark[_theme][_colorGroupEnabled ? 1 : 0]; }
    
    QColor text(void) const { return _text[_theme][_colorGroupEnabled ? 1 : 0]; }
	QColor warningText(void) const { return _warningText[_theme][_colorGroupEnabled ? 1 : 0]; }

    QColor button(void) const { return _button[_theme][_colorGroupEnabled ? 1 : 0]; }
    QColor buttonText(void) const { return _buttonText[_theme][_colorGroupEnabled ? 1 : 0]; }
    QColor buttonHighlight(void) const { return _buttonHighlight[_theme][_colorGroupEnabled ? 1 : 0]; }
    QColor buttonHighlightText(void) const { return _buttonHighlightText[_theme][_colorGroupEnabled ? 1 : 0]; }
    
    QColor primaryButton(void) const { return _primaryButton[_theme][_colorGroupEnabled ? 1 : 0]; }
    QColor primaryButtonText(void) const { return _primaryButtonText[_theme][_colorGroupEnabled ? 1 : 0]; }
    
    QColor textField(void) const { return _textField[_theme][_colorGroupEnabled ? 1 : 0]; }
    QColor textFieldText(void) const { return _textFieldText[_theme][_colorGroupEnabled ? 1 : 0]; }
    
    static Theme globalTheme(void) { return _theme; }
    static void setGlobalTheme(Theme newTheme);

signals:
    void paletteChanged(void);
    
private:
    static Theme    _theme;             ///< There is a single theme for all palettes
    bool            _colorGroupEnabled; ///< Currently selected ColorGroup. true: enabled, false: disabled
    
    static const int _cThemes = 2;
    static const int _cColorGroups = 2;
    
    static QColor _window[_cThemes][_cColorGroups];
    static QColor _windowShade[_cThemes][_cColorGroups];
    static QColor _windowShadeDark[_cThemes][_cColorGroups];
    
	static QColor _warningText[_cThemes][_cColorGroups];
	static QColor _text[_cThemes][_cColorGroups];
    
    static QColor _button[_cThemes][_cColorGroups];
    static QColor _buttonText[_cThemes][_cColorGroups];
    static QColor _buttonHighlight[_cThemes][_cColorGroups];
    static QColor _buttonHighlightText[_cThemes][_cColorGroups];
    
    static QColor _primaryButton[_cThemes][_cColorGroups];
    static QColor _primaryButtonText[_cThemes][_cColorGroups];
    static QColor _primaryButtonHighlight[_cThemes][_cColorGroups];
    static QColor _primaryButtonHighlightText[_cThemes][_cColorGroups];
    
    static QColor _textField[_cThemes][_cColorGroups];
    static QColor _textFieldText[_cThemes][_cColorGroups];
    
    void _themeChanged(void);
    
    static QList<QGCPalette*>   _paletteObjects;    ///< List of all active QGCPalette objects
};

#endif
