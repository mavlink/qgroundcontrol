/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#ifndef QGCPalette_h
#define QGCPalette_h

#include <QObject>
#include <QColor>

/*!
 QGCPalette is used in QML ui to expose color properties for the QGC palette. There are two
 separate palettes in QGC, light and dark. The light palette is for outdoor use and the dark
 palette is for indoor use. Each palette also has a set of different colors for enabled and
 disabled states.

 Usage:

        import QGroundControl.Palette 1.0

        Rectangle {
            anchors.fill:   parent
            color:          qgcPal.window

            QGCPalette { id: qgcPal: colorGroupEnabled: enabled }
        }
*/
class QGCPalette : public QObject
{
    Q_OBJECT
    
    Q_ENUMS(Theme)
    
    Q_PROPERTY(Theme globalTheme READ globalTheme WRITE setGlobalTheme NOTIFY paletteChanged)
    
    Q_PROPERTY(bool colorGroupEnabled READ colorGroupEnabled WRITE setColorGroupEnabled NOTIFY paletteChanged)

    Q_PROPERTY(QColor window                READ window                 WRITE setWindow                 NOTIFY paletteChanged)
    Q_PROPERTY(QColor windowShade           READ windowShade            WRITE setWindowShade            NOTIFY paletteChanged)
    Q_PROPERTY(QColor windowShadeDark       READ windowShadeDark        WRITE setWindowShadeDark        NOTIFY paletteChanged)
    Q_PROPERTY(QColor text                  READ text                   WRITE setText                   NOTIFY paletteChanged)
    Q_PROPERTY(QColor warningText           READ warningText            WRITE setWarningText            NOTIFY paletteChanged)
    Q_PROPERTY(QColor button                READ button                 WRITE setButton                 NOTIFY paletteChanged)
    Q_PROPERTY(QColor buttonText            READ buttonText             WRITE setButtonText             NOTIFY paletteChanged)
    Q_PROPERTY(QColor buttonHighlight       READ buttonHighlight        WRITE setButtonHighlight        NOTIFY paletteChanged)
    Q_PROPERTY(QColor buttonHighlightText   READ buttonHighlightText    WRITE setButtonHighlightText    NOTIFY paletteChanged)
    Q_PROPERTY(QColor primaryButton         READ primaryButton          WRITE setPrimaryButton          NOTIFY paletteChanged)
    Q_PROPERTY(QColor primaryButtonText     READ primaryButtonText      WRITE setPrimaryButtonText      NOTIFY paletteChanged)
    Q_PROPERTY(QColor textField             READ textField              WRITE setTextField              NOTIFY paletteChanged)
    Q_PROPERTY(QColor textFieldText         READ textFieldText          WRITE setTextFieldText          NOTIFY paletteChanged)
    Q_PROPERTY(QColor mapButton             READ mapButton              WRITE setMapButton              NOTIFY paletteChanged)
    Q_PROPERTY(QColor mapButtonHighlight    READ mapButtonHighlight     WRITE setMapButtonHighlight     NOTIFY paletteChanged)
    Q_PROPERTY(QColor mapWidgetBorderLight  READ mapWidgetBorderLight   WRITE setMapWidgetBorderLight   NOTIFY paletteChanged)
    Q_PROPERTY(QColor mapWidgetBorderDark   READ mapWidgetBorderDark    WRITE setMapWidgetBorderDark    NOTIFY paletteChanged)
    Q_PROPERTY(QColor brandingPurple        READ brandingPurple                                         NOTIFY paletteChanged)
    Q_PROPERTY(QColor brandingBlue          READ brandingBlue                                           NOTIFY paletteChanged)

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
    
    /// Background color for windows
    QColor window(void)                 const { return _window[_theme][_colorGroupEnabled ? 1 : 0]; }

    /// Color for shaded areas within a window. The windowShade color is a color between window and button.
    QColor windowShade(void)            const { return _windowShade[_theme][_colorGroupEnabled ? 1 : 0]; }

    /// Color for darker shared areas within a window. The windowShadeDark color is a color between window and windowShade.
    QColor windowShadeDark(void)        const { return _windowShadeDark[_theme][_colorGroupEnabled ? 1 : 0]; }

    /// Text color
    QColor text(void)                   const { return _text[_theme][_colorGroupEnabled ? 1 : 0]; }

    /// Color for warning text
    QColor warningText(void)            const { return _warningText[_theme][_colorGroupEnabled ? 1 : 0]; }

    /// Background color for buttons
    QColor button(void)                 const { return _button[_theme][_colorGroupEnabled ? 1 : 0]; }

    /// Text color for buttons
    QColor buttonText(void)             const { return _buttonText[_theme][_colorGroupEnabled ? 1 : 0]; }

    /// Background color for button in selected or hover state
    QColor buttonHighlight(void)        const { return _buttonHighlight[_theme][_colorGroupEnabled ? 1 : 0]; }

    /// Text color for button in selected or hover state
    QColor buttonHighlightText(void)    const { return _buttonHighlightText[_theme][_colorGroupEnabled ? 1 : 0]; }

    /// Background color for primary buttons. A primary button is the button which would be the normal default  button to press.
    /// For example in an Ok/Cancel situation where Ok would normally be pressed, Ok is the primary button.
    QColor primaryButton(void)          const { return _primaryButton[_theme][_colorGroupEnabled ? 1 : 0]; }

    /// Text color for primary buttons
    QColor primaryButtonText(void)      const { return _primaryButtonText[_theme][_colorGroupEnabled ? 1 : 0]; }

    /// Background color for TextFields
    QColor textField(void)              const { return _textField[_theme][_colorGroupEnabled ? 1 : 0]; }

    /// Text color for TextFields
    QColor textFieldText(void)          const { return _textFieldText[_theme][_colorGroupEnabled ? 1 : 0]; }

    /// Background color for map buttons
    QColor mapButton(void)              const { return _mapButton[_theme][_colorGroupEnabled ? 1 : 0]; }

    /// Background color for map button in selected or hover state
    QColor mapButtonHighlight(void)     const { return _mapButtonHighlight[_theme][_colorGroupEnabled ? 1 : 0]; }

    /// Widget border color which will stand out against light map tiles
    QColor mapWidgetBorderLight(void)   const { return _mapWidgetBorderLight[_theme][_colorGroupEnabled ? 1 : 0]; }

    /// Widget border color which will stand out against dark map tiles
    QColor mapWidgetBorderDark(void)    const { return _mapWidgetBorderDark[_theme][_colorGroupEnabled ? 1 : 0]; }

    /// Purple color from branding guidelines
    QColor brandingPurple(void)         const { return _brandingPurple[_theme][_colorGroupEnabled ? 1 : 0]; }

    /// Blue color from branding guidelines
    QColor brandingBlue(void)           const { return _brandingBlue[_theme][_colorGroupEnabled ? 1 : 0]; }

    void setWindow(QColor& color)               { _window[_theme][_colorGroupEnabled ? 1 : 0] = color; _signalPaletteChangeToAll(); }
    void setWindowShade(QColor& color)          { _windowShade[_theme][_colorGroupEnabled ? 1 : 0] = color; _signalPaletteChangeToAll(); }
    void setWindowShadeDark(QColor& color)      { _windowShadeDark[_theme][_colorGroupEnabled ? 1 : 0] = color; _signalPaletteChangeToAll(); }
    void setText(QColor& color)                 { _text[_theme][_colorGroupEnabled ? 1 : 0] = color; _signalPaletteChangeToAll(); }
    void setWarningText(QColor& color)          { _warningText[_theme][_colorGroupEnabled ? 1 : 0] = color; _signalPaletteChangeToAll(); }
    void setButton(QColor& color)               { _button[_theme][_colorGroupEnabled ? 1 : 0] = color; _signalPaletteChangeToAll(); }
    void setButtonText(QColor& color)           { _buttonText[_theme][_colorGroupEnabled ? 1 : 0] = color; _signalPaletteChangeToAll(); }
    void setButtonHighlight(QColor& color)      { _buttonHighlight[_theme][_colorGroupEnabled ? 1 : 0] = color; _signalPaletteChangeToAll(); }
    void setButtonHighlightText(QColor& color)  { _buttonHighlightText[_theme][_colorGroupEnabled ? 1 : 0] = color; _signalPaletteChangeToAll(); }
    void setPrimaryButton(QColor& color)        { _primaryButton[_theme][_colorGroupEnabled ? 1 : 0] = color; _signalPaletteChangeToAll(); }
    void setPrimaryButtonText(QColor& color)    { _primaryButtonText[_theme][_colorGroupEnabled ? 1 : 0] = color; _signalPaletteChangeToAll(); }
    void setTextField(QColor& color)            { _textField[_theme][_colorGroupEnabled ? 1 : 0] = color; _signalPaletteChangeToAll(); }
    void setTextFieldText(QColor& color)        { _textFieldText[_theme][_colorGroupEnabled ? 1 : 0] = color; _signalPaletteChangeToAll(); }
    void setMapButton(QColor& color)            { _mapButton[_theme][_colorGroupEnabled ? 1 : 0] = color; _signalPaletteChangeToAll(); }
    void setMapButtonHighlight(QColor& color)   { _mapButtonHighlight[_theme][_colorGroupEnabled ? 1 : 0] = color; _signalPaletteChangeToAll(); }
    void setMapWidgetBorderLight(QColor& color) { _mapWidgetBorderLight[_theme][_colorGroupEnabled ? 1 : 0] = color; _signalPaletteChangeToAll(); }
    void setMapWidgetBorderDark(QColor& color)  { _mapWidgetBorderDark[_theme][_colorGroupEnabled ? 1 : 0] = color; _signalPaletteChangeToAll(); }

    static Theme globalTheme(void) { return _theme; }
    static void setGlobalTheme(Theme newTheme);

signals:
    void paletteChanged(void);
    
private:
    static void _signalPaletteChangeToAll(void);
    void _signalPaletteChanged(void);

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
    
    static QColor _mapButton[_cThemes][_cColorGroups];
    static QColor _mapButtonHighlight[_cThemes][_cColorGroups];
    static QColor _mapWidgetBorderLight[_cThemes][_cColorGroups];
    static QColor _mapWidgetBorderDark[_cThemes][_cColorGroups];

    static QColor _brandingPurple[_cThemes][_cColorGroups];
    static QColor _brandingBlue[_cThemes][_cColorGroups];

    void _themeChanged(void);
    
    static QList<QGCPalette*>   _paletteObjects;    ///< List of all active QGCPalette objects
};

#endif
