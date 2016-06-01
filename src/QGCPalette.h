/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


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

    // The colors are:
    //      window -                Background color for windows
    //      windowShade -           Color for shaded areas within a window. The windowShade color should be a color somewhere between window and button.
    //      windowShadeDark -       Color for darker shared areas within a window. The windowShadeDark color should be a color somewhere between window and windowShade.
    //      text -                  Standard text color for label text
    //      warningText -           Color for warning text
    //      button -                Background color for buttons
    //      buttonText -            Text color for buttons
    //      buttonHighlight -       Background color for button in selected or hover state
    //      buttonHighlightText -   Text color for button in selected or hover state

    //      primaryButton -         Background color for primary buttons. A primary button is the button which would be the
    //                              normal default  button to press. For example in an Ok/Cancel situation where Ok would normally
    //                              be pressed, Ok is the primary button.
    //      primaryButtonText -     Text color for primary buttons
    //      textField -             Background color for TextFields
    //      textFieldText -         Text color for TextFields
    //      mapButton -             Background color for map buttons
    //      mapButtonHighlight -    Background color for map button in selected or hover state
    //      mapWidgetBorderLight -  Widget border color which will stand out against light map tiles
    //      mapWidgetBorderDark -   Widget border color which will stand out against dark map tiles
    //      brandingPurple -        Purple color from branding guidelines
    //      brandingBlue -          Blue color from branding guidelines

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
    
    QColor window(void)                 const { return _window[_theme][_colorGroupEnabled ? 1 : 0]; }
    QColor windowShade(void)            const { return _windowShade[_theme][_colorGroupEnabled ? 1 : 0]; }
    QColor windowShadeDark(void)        const { return _windowShadeDark[_theme][_colorGroupEnabled ? 1 : 0]; }
    QColor text(void)                   const { return _text[_theme][_colorGroupEnabled ? 1 : 0]; }
    QColor warningText(void)            const { return _warningText[_theme][_colorGroupEnabled ? 1 : 0]; }
    QColor button(void)                 const { return _button[_theme][_colorGroupEnabled ? 1 : 0]; }
    QColor buttonText(void)             const { return _buttonText[_theme][_colorGroupEnabled ? 1 : 0]; }
    QColor buttonHighlight(void)        const { return _buttonHighlight[_theme][_colorGroupEnabled ? 1 : 0]; }
    QColor buttonHighlightText(void)    const { return _buttonHighlightText[_theme][_colorGroupEnabled ? 1 : 0]; }
    QColor primaryButton(void)          const { return _primaryButton[_theme][_colorGroupEnabled ? 1 : 0]; }
    QColor primaryButtonText(void)      const { return _primaryButtonText[_theme][_colorGroupEnabled ? 1 : 0]; }
    QColor textField(void)              const { return _textField[_theme][_colorGroupEnabled ? 1 : 0]; }
    QColor textFieldText(void)          const { return _textFieldText[_theme][_colorGroupEnabled ? 1 : 0]; }
    QColor mapButton(void)              const { return _mapButton[_theme][_colorGroupEnabled ? 1 : 0]; }
    QColor mapButtonHighlight(void)     const { return _mapButtonHighlight[_theme][_colorGroupEnabled ? 1 : 0]; }
    QColor mapWidgetBorderLight(void)   const { return _mapWidgetBorderLight[_theme][_colorGroupEnabled ? 1 : 0]; }
    QColor mapWidgetBorderDark(void)    const { return _mapWidgetBorderDark[_theme][_colorGroupEnabled ? 1 : 0]; }
    QColor brandingPurple(void)         const { return _brandingPurple[_theme][_colorGroupEnabled ? 1 : 0]; }
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
