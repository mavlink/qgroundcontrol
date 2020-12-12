/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#ifndef QGCPalette_h
#define QGCPalette_h

#include <QObject>
#include <QColor>
#include <QMap>

#define DECLARE_QGC_COLOR(name, lightDisabled, lightEnabled, darkDisabled, darkEnabled) \
    { \
        PaletteColorInfo_t colorInfo = { \
            { QColor(lightDisabled), QColor(lightEnabled) }, \
            { QColor(darkDisabled), QColor(darkEnabled) } \
        }; \
        qgcApp()->toolbox()->corePlugin()->paletteOverride(#name, colorInfo); \
        _colorInfoMap[Light][ColorGroupEnabled][QStringLiteral(#name)] = colorInfo[Light][ColorGroupEnabled]; \
        _colorInfoMap[Light][ColorGroupDisabled][QStringLiteral(#name)] = colorInfo[Light][ColorGroupDisabled]; \
        _colorInfoMap[Dark][ColorGroupEnabled][QStringLiteral(#name)] = colorInfo[Dark][ColorGroupEnabled]; \
        _colorInfoMap[Dark][ColorGroupDisabled][QStringLiteral(#name)] = colorInfo[Dark][ColorGroupDisabled]; \
        _colors << #name; \
    }

#define DECLARE_QGC_NONTHEMED_COLOR(name, disabledColor, enabledColor) \
    { \
        PaletteColorInfo_t colorInfo = { \
            { QColor(disabledColor), QColor(enabledColor) }, \
            { QColor(disabledColor), QColor(enabledColor) } \
        }; \
        qgcApp()->toolbox()->corePlugin()->paletteOverride(#name, colorInfo); \
        _colorInfoMap[Light][ColorGroupEnabled][QStringLiteral(#name)] = colorInfo[Light][ColorGroupEnabled]; \
        _colorInfoMap[Light][ColorGroupDisabled][QStringLiteral(#name)] = colorInfo[Light][ColorGroupDisabled]; \
        _colorInfoMap[Dark][ColorGroupEnabled][QStringLiteral(#name)] = colorInfo[Dark][ColorGroupEnabled]; \
        _colorInfoMap[Dark][ColorGroupDisabled][QStringLiteral(#name)] = colorInfo[Dark][ColorGroupDisabled]; \
        _colors << #name; \
    }

#define DECLARE_QGC_SINGLE_COLOR(name, color) \
    { \
        PaletteColorInfo_t colorInfo = { \
            { QColor(color), QColor(color) }, \
            { QColor(color), QColor(color) } \
        }; \
        qgcApp()->toolbox()->corePlugin()->paletteOverride(#name, colorInfo); \
        _colorInfoMap[Light][ColorGroupEnabled][QStringLiteral(#name)] = colorInfo[Light][ColorGroupEnabled]; \
        _colorInfoMap[Light][ColorGroupDisabled][QStringLiteral(#name)] = colorInfo[Light][ColorGroupDisabled]; \
        _colorInfoMap[Dark][ColorGroupEnabled][QStringLiteral(#name)] = colorInfo[Dark][ColorGroupEnabled]; \
        _colorInfoMap[Dark][ColorGroupDisabled][QStringLiteral(#name)] = colorInfo[Dark][ColorGroupDisabled]; \
        _colors << #name; \
    }

#define DEFINE_QGC_COLOR(NAME, SETNAME) \
    Q_PROPERTY(QColor NAME READ NAME WRITE SETNAME NOTIFY paletteChanged) \
    Q_PROPERTY(QStringList NAME ## Colors READ NAME ## Colors NOTIFY paletteChanged) \
    QColor NAME() const { return _colorInfoMap[_theme][_colorGroupEnabled  ? ColorGroupEnabled : ColorGroupDisabled][QStringLiteral(#NAME)]; } \
    QStringList NAME ## Colors() const { \
        QStringList c; \
        c << _colorInfoMap[Light][ColorGroupEnabled][QStringLiteral(#NAME)].name(QColor::HexRgb); \
        c << _colorInfoMap[Light][ColorGroupDisabled][QStringLiteral(#NAME)].name(QColor::HexRgb); \
        c << _colorInfoMap[Dark][ColorGroupEnabled][QStringLiteral(#NAME)].name(QColor::HexRgb); \
        c << _colorInfoMap[Dark][ColorGroupDisabled][QStringLiteral(#NAME)].name(QColor::HexRgb); \
        return c; \
    } \
    void SETNAME(const QColor& color) { _colorInfoMap[_theme][_colorGroupEnabled  ? ColorGroupEnabled : ColorGroupDisabled][QStringLiteral(#NAME)] = color; _signalPaletteChangeToAll(); }

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

public:
    enum ColorGroup {
        ColorGroupDisabled = 0,
        ColorGroupEnabled,
        cMaxColorGroup
    };

    enum Theme {
        Light = 0,
        Dark,
        cMaxTheme
    };
    Q_ENUM(Theme)

    typedef QColor PaletteColorInfo_t[cMaxTheme][cMaxColorGroup];

    Q_PROPERTY(Theme        globalTheme         READ globalTheme        WRITE setGlobalTheme        NOTIFY paletteChanged)
    Q_PROPERTY(bool         colorGroupEnabled   READ colorGroupEnabled  WRITE setColorGroupEnabled  NOTIFY paletteChanged)
    Q_PROPERTY(QStringList  colors              READ colors             CONSTANT)

    DEFINE_QGC_COLOR(window,                        setWindow)
    DEFINE_QGC_COLOR(windowShadeLight,              setWindowShadeLight)
    DEFINE_QGC_COLOR(windowShade,                   setWindowShade)
    DEFINE_QGC_COLOR(windowShadeDark,               setWindowShadeDark)
    DEFINE_QGC_COLOR(text,                          setText)
    DEFINE_QGC_COLOR(warningText,                   setWarningText)
    DEFINE_QGC_COLOR(button,                        setButton)
    DEFINE_QGC_COLOR(buttonText,                    setButtonText)
    DEFINE_QGC_COLOR(buttonHighlight,               setButtonHighlight)
    DEFINE_QGC_COLOR(buttonHighlightText,           setButtonHighlightText)
    DEFINE_QGC_COLOR(primaryButton,                 setPrimaryButton)
    DEFINE_QGC_COLOR(primaryButtonText,             setPrimaryButtonText)
    DEFINE_QGC_COLOR(textField,                     setTextField)
    DEFINE_QGC_COLOR(textFieldText,                 setTextFieldText)
    DEFINE_QGC_COLOR(mapButton,                     setMapButton)
    DEFINE_QGC_COLOR(mapButtonHighlight,            setMapButtonHighlight)
    DEFINE_QGC_COLOR(mapIndicator,                  setMapIndicator)
    DEFINE_QGC_COLOR(mapIndicatorChild,             setMapIndicatorChild)
    DEFINE_QGC_COLOR(mapWidgetBorderLight,          setMapWidgetBorderLight)
    DEFINE_QGC_COLOR(mapWidgetBorderDark,           setMapWidgetBorderDark)
    DEFINE_QGC_COLOR(mapMissionTrajectory,          setMapMissionTrajectory)
    DEFINE_QGC_COLOR(brandingPurple,                setBrandingPurple)
    DEFINE_QGC_COLOR(brandingBlue,                  setBrandingBlue)
    DEFINE_QGC_COLOR(colorGreen,                    setColorGreen)
    DEFINE_QGC_COLOR(colorOrange,                   setColorOrange)
    DEFINE_QGC_COLOR(colorRed,                      setColorRed)
    DEFINE_QGC_COLOR(colorGrey,                     setColorGrey)
    DEFINE_QGC_COLOR(colorBlue,                     setColorBlue)
    DEFINE_QGC_COLOR(alertBackground,               setAlertBackground)
    DEFINE_QGC_COLOR(alertBorder,                   setAlertBorder)
    DEFINE_QGC_COLOR(alertText,                     setAlertText)
    DEFINE_QGC_COLOR(missionItemEditor,             setMissionItemEditor)
    DEFINE_QGC_COLOR(statusFailedText,              setstatusFailedText)
    DEFINE_QGC_COLOR(statusPassedText,              setstatusPassedText)
    DEFINE_QGC_COLOR(statusPendingText,             setstatusPendingText)
    DEFINE_QGC_COLOR(surveyPolygonInterior,         setSurveyPolygonInterior)
    DEFINE_QGC_COLOR(surveyPolygonTerrainCollision, setSurveyPolygonTerrainCollision)
    DEFINE_QGC_COLOR(toolbarBackground,             setToolbarBackground)
    DEFINE_QGC_COLOR(toolStripFGColor,              setToolStripFGColor)
    DEFINE_QGC_COLOR(toolStripHoverColor,           setToolStripHoverColor)

     QGCPalette(QObject* parent = nullptr);
    ~QGCPalette();

    QStringList colors                      () const { return _colors; }
    bool        colorGroupEnabled           () const { return _colorGroupEnabled; }
    void        setColorGroupEnabled        (bool enabled);

    static Theme    globalTheme             () { return _theme; }
    static void     setGlobalTheme          (Theme newTheme);

signals:
    void paletteChanged ();

private:
    static void _buildMap                   ();
    static void _signalPaletteChangeToAll   ();
    void        _signalPaletteChanged       ();
    void        _themeChanged               ();

    static Theme                _theme;             ///< There is a single theme for all palettes
    bool                        _colorGroupEnabled; ///< Currently selected ColorGroup. true: enabled, false: disabled
    static QStringList          _colors;

    static QMap<int, QMap<int, QMap<QString, QColor>>> _colorInfoMap;   // theme -> colorGroup -> color name -> color
    static QList<QGCPalette*> _paletteObjects;    ///< List of all active QGCPalette objects
};

#endif
