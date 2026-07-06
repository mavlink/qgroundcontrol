/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @author Don Gagne <don@thegagnes.com>

#include "QGCPalette.h"
#include "QGCCorePlugin.h"

#include <QtCore/QDebug>

QList<QGCPalette*>   QGCPalette::_paletteObjects;

QGCPalette::Theme QGCPalette::_theme = QGCPalette::Dark;

QMap<int, QMap<int, QMap<QString, QColor>>> QGCPalette::_colorInfoMap;

QStringList QGCPalette::_colors;

QGCPalette::QGCPalette(QObject* parent) :
    QObject(parent),
    _colorGroupEnabled(true)
{
    if (_colorInfoMap.isEmpty()) {
        _buildMap();
    }

    // We have to keep track of all QGCPalette objects in the system so we can signal theme change to all of them
    _paletteObjects += this;
}

QGCPalette::~QGCPalette()
{
    bool fSuccess = _paletteObjects.removeOne(this);
    if (!fSuccess) {
        qWarning() << "Internal error";
    }
}

void QGCPalette::_buildMap()
{
    //                                      Light                 Dark
    //                                      Disabled   Enabled    Disabled   Enabled
    DECLARE_QGC_COLOR(window,               "#f5f7f8", "#f5f7f8", "#101215", "#101215")
    DECLARE_QGC_COLOR(windowShadeLight,     "#c5d0d6", "#dbe3e7", "#26292d", "#2b2f34")
    DECLARE_QGC_COLOR(windowShade,          "#d8e0e4", "#eef3f5", "#181a1d", "#1d2024")
    DECLARE_QGC_COLOR(windowShadeDark,      "#a9b7be", "#c0ccd2", "#101214", "#121416")
    DECLARE_QGC_COLOR(text,                 "#74818a", "#101820", "#b8c0c8", "#f4f6f8")
    DECLARE_QGC_COLOR(warningText,          "#b54708", "#b54708", "#ffb020", "#ffb020")
    DECLARE_QGC_COLOR(button,               "#e8eef1", "#ffffff", "#17191c", "#202327")
    DECLARE_QGC_COLOR(buttonBorder,         "#c8d3d9", "#b9c8cf", "#5d646b", "#6d747c")
    DECLARE_QGC_COLOR(buttonText,           "#6c7982", "#101820", "#aeb6bf", "#e8edf2")
    DECLARE_QGC_COLOR(buttonHighlight,      "#d8f8d1", "#2fb344", "#25282c", "#2d3237")
    DECLARE_QGC_COLOR(buttonHighlightText,  "#102014", "#ffffff", "#e8edf2", "#f7f9fb")
    DECLARE_QGC_COLOR(primaryButton,        "#d8f8d1", "#2fb344", "#00afff", "#00afff")
    DECLARE_QGC_COLOR(primaryButtonText,    "#102014", "#ffffff", "#03111a", "#03111a")
    DECLARE_QGC_COLOR(textField,            "#ffffff", "#ffffff", "#17191c", "#17191c")
    DECLARE_QGC_COLOR(textFieldText,        "#5f6d76", "#101820", "#aeb6bf", "#f4f6f8")
    DECLARE_QGC_COLOR(mapButton,            "#10202b", "#10202b", "#101215", "#101215")
    DECLARE_QGC_COLOR(mapButtonHighlight,   "#2fb344", "#2fb344", "#00afff", "#00afff")
    DECLARE_QGC_COLOR(mapIndicator,         "#10202b", "#10202b", "#101215", "#101215")
    DECLARE_QGC_COLOR(mapIndicatorChild,    "#163141", "#163141", "#181a1d", "#1d2024")
    DECLARE_QGC_COLOR(colorGreen,           "#198754", "#198754", "#00d6a3", "#00d6a3")
    DECLARE_QGC_COLOR(colorYellow,          "#b58900", "#b58900", "#ffb000", "#ffb000")
    DECLARE_QGC_COLOR(colorYellowGreen,     "#6c9a16", "#6c9a16", "#9ee95d", "#9ee95d")
    DECLARE_QGC_COLOR(colorOrange,          "#c65f21", "#c65f21", "#ff8a1f", "#ff8a1f")
    DECLARE_QGC_COLOR(colorRed,             "#b42318", "#b42318", "#ff3b4f", "#ff3b4f")
    DECLARE_QGC_COLOR(colorGrey,            "#6f7b85", "#6f7b85", "#9aa0a6", "#9aa0a6")
    DECLARE_QGC_COLOR(colorBlue,            "#0674a6", "#0674a6", "#00afff", "#00afff")
    DECLARE_QGC_COLOR(alertBackground,      "#ffd166", "#ffd166", "#ffd166", "#ffd166")
    DECLARE_QGC_COLOR(alertBorder,          "#9aa6b2", "#9aa6b2", "#9aa6b2", "#9aa6b2")
    DECLARE_QGC_COLOR(alertText,            "#000000", "#000000", "#000000", "#000000")
    DECLARE_QGC_COLOR(missionItemEditor,    "#eef8f2", "#e3f8ea", "#181a1d", "#1d2024")
    DECLARE_QGC_COLOR(toolStripHoverColor,  "#d8f8d1", "#e7fbea", "#25282c", "#2d3237")
    DECLARE_QGC_COLOR(statusFailedText,     "#74818a", "#101820", "#8f969e", "#f4f6f8")
    DECLARE_QGC_COLOR(statusPassedText,     "#74818a", "#101820", "#8f969e", "#f4f6f8")
    DECLARE_QGC_COLOR(statusPendingText,    "#74818a", "#101820", "#8f969e", "#f4f6f8")
    DECLARE_QGC_COLOR(toolbarBackground,    "#f7fafb", "#f7fafb", "#101215", "#101215")
    DECLARE_QGC_COLOR(groupBorder,          "#c8d3d9", "#b9c8cf", "#454b52", "#4c535a")

    // Colors not affecting by theming
    //                                              Disabled    Enabled
    DECLARE_QGC_NONTHEMED_COLOR(brandingPurple,     "#00afff", "#00afff")
    DECLARE_QGC_NONTHEMED_COLOR(brandingBlue,       "#00afff", "#00afff")
    DECLARE_QGC_NONTHEMED_COLOR(toolStripFGColor,   "#707070", "#ffffff")

    // Colors not affecting by theming or enable/disable
    DECLARE_QGC_SINGLE_COLOR(mapWidgetBorderLight,          "#ffffff")
    DECLARE_QGC_SINGLE_COLOR(mapWidgetBorderDark,           "#000000")
    DECLARE_QGC_SINGLE_COLOR(mapMissionTrajectory,          "#be781c")
    DECLARE_QGC_SINGLE_COLOR(surveyPolygonInterior,         "green")
    DECLARE_QGC_SINGLE_COLOR(surveyPolygonTerrainCollision, "red")

// Colors for UTM Adapter
#ifdef QGC_UTM_ADAPTER
    DECLARE_QGC_COLOR(switchUTMSP,        "#b0e0e6", "#b0e0e6", "#b0e0e6", "#b0e0e6");
    DECLARE_QGC_COLOR(sliderUTMSP,        "#9370db", "#9370db", "#9370db", "#9370db");
    DECLARE_QGC_COLOR(successNotifyUTMSP, "#3cb371", "#3cb371", "#3cb371", "#3cb371");
#endif
}

void QGCPalette::setColorGroupEnabled(bool enabled)
{
    _colorGroupEnabled = enabled;
    emit paletteChanged();
}

void QGCPalette::setGlobalTheme(Theme newTheme)
{
    // Mobile build does not have themes
    if (_theme != newTheme) {
        _theme = newTheme;
        _signalPaletteChangeToAll();
    }
}

void QGCPalette::_signalPaletteChangeToAll()
{
    // Notify all objects of the new theme
    for (QGCPalette *palette : std::as_const(_paletteObjects)) {
        palette->_signalPaletteChanged();
    }
}

void QGCPalette::_signalPaletteChanged()
{
    emit paletteChanged();
}
