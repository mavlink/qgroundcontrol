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
    DECLARE_QGC_COLOR(window,               "#ffffff", "#ffffff", "#2a0000", "#2a0000")
    DECLARE_QGC_COLOR(windowTransparent,    "#ccffffff", "#ccffffff", "#cc2a0000", "#cc2a0000")
    DECLARE_QGC_COLOR(windowShadeLight,     "#f9cfcf", "#f9cfcf", "#5b0e0e", "#4a0a0a")
    DECLARE_QGC_COLOR(windowShade,          "#f77474", "#f77474", "#550808", "#440606")
    DECLARE_QGC_COLOR(windowShadeDark,      "#f45050", "#f45050", "#3e0505", "#300404")
    DECLARE_QGC_COLOR(text,                 "#9d9d9d", "#000000", "#707070", "#ffffff")
    DECLARE_QGC_COLOR(windowTransparentText,"#9d9d9d", "#000000", "#707070", "#ffffff")
    DECLARE_QGC_COLOR(warningText,          "#cc0808", "#cc0808", "#ff4d4d", "#ff4d4d")
    DECLARE_QGC_COLOR(button,               "#ffffff", "#ffffff", "#6a0a0a", "#5a0000")
    DECLARE_QGC_COLOR(buttonBorder,         "#ffffff", "#d9d9d9", "#6a0a0a", "#ad2a2a")
    DECLARE_QGC_COLOR(buttonText,           "#9d9d9d", "#000000", "#A6A6A6", "#ffffff")
    DECLARE_QGC_COLOR(buttonHighlight,      "#ffecec", "#ff9a9a", "#4a0000", "#ff4d4d")
    DECLARE_QGC_COLOR(buttonHighlightText,  "#2c2c2c", "#ffffff", "#2c2c2c", "#000000")
    DECLARE_QGC_COLOR(primaryButton,        "#f45050", "#ff0000", "#800000", "#ff0000")
    DECLARE_QGC_COLOR(primaryButtonText,    "#2c2c2c", "#000000", "#ffffff", "#ffffff")
    DECLARE_QGC_COLOR(textField,            "#ffffff", "#ffffff", "#6a0a0a", "#5a0000")
    DECLARE_QGC_COLOR(textFieldText,        "#808080", "#000000", "#000000", "#000000")
    DECLARE_QGC_COLOR(mapButton,            "#585858", "#000000", "#585858", "#000000")
    DECLARE_QGC_COLOR(mapButtonHighlight,   "#585858", "#ff4d4d", "#585858", "#ff4d4d")
    DECLARE_QGC_COLOR(mapIndicator,         "#f45050", "#ff0000", "#b30000", "#ff0000")
    DECLARE_QGC_COLOR(mapIndicatorChild,    "#f77474", "#f45050", "#8b0000", "#a50000")
    DECLARE_QGC_COLOR(colorGreen,           "#f9cfcf", "#f77474", "#f45050", "#f45050") 
    DECLARE_QGC_COLOR(colorYellow,          "#f9cfcf", "#f77474", "#ff9999", "#ff9999")  
    DECLARE_QGC_COLOR(colorYellowGreen,     "#f9cfcf", "#f77474", "#f45050", "#f45050")  
    DECLARE_QGC_COLOR(colorOrange,          "#f77474", "#f45050", "#ff4d4d", "#ff4d4d")  
    DECLARE_QGC_COLOR(colorRed,             "#f45050", "#f45050", "#ff0000", "#ff0000")
    DECLARE_QGC_COLOR(colorGrey,            "#808080", "#808080", "#bfbfbf", "#bfbfbf")
    DECLARE_QGC_COLOR(colorBlue,            "#800000", "#800000", "#800000", "#800000")
    DECLARE_QGC_COLOR(alertBackground,      "#eecc44", "#eecc44", "#eecc44", "#eecc44")
    DECLARE_QGC_COLOR(alertBorder,          "#808080", "#808080", "#808080", "#808080")
    DECLARE_QGC_COLOR(alertText,            "#000000", "#000000", "#000000", "#000000")
    DECLARE_QGC_COLOR(missionItemEditor,    "#585858", "#ffbdbd", "#585858", "#8b2a2a")
    DECLARE_QGC_COLOR(toolStripHoverColor,  "#585858", "#9D9D9D", "#585858", "#8b2a2a")
    DECLARE_QGC_COLOR(statusFailedText,     "#9d9d9d", "#000000", "#707070", "#ffffff")
    DECLARE_QGC_COLOR(statusPassedText,     "#9d9d9d", "#000000", "#707070", "#ffffff")
    DECLARE_QGC_COLOR(statusPendingText,    "#9d9d9d", "#000000", "#707070", "#ffffff")
    DECLARE_QGC_COLOR(toolbarBackground,    "#00ffffff", "#00ffffff", "#002a0000", "#002a0000")
    DECLARE_QGC_COLOR(toolbarDivider,       "#00000000", "#00000000", "#00000000", "#00000000")
    DECLARE_QGC_COLOR(groupBorder,          "#bbbbbb", "#bbbbbb", "#707070", "#707070")

    // Colors not affecting by theming
    //                                              Disabled    Enabled
    DECLARE_QGC_NONTHEMED_COLOR(brandingPurple,     "#F77474", "#F77474")
    DECLARE_QGC_NONTHEMED_COLOR(brandingBlue,       "#FF0000", "#FF0000")
    DECLARE_QGC_NONTHEMED_COLOR(toolStripFGColor,   "#707070", "#ffffff")

    // Colors not affecting by theming or enable/disable
    DECLARE_QGC_SINGLE_COLOR(mapWidgetBorderLight,          "#ffffff")
    DECLARE_QGC_SINGLE_COLOR(mapWidgetBorderDark,           "#000000")
    DECLARE_QGC_SINGLE_COLOR(mapMissionTrajectory,          "#ff4d4d")
    DECLARE_QGC_SINGLE_COLOR(surveyPolygonInterior,         "green")
    DECLARE_QGC_SINGLE_COLOR(surveyPolygonTerrainCollision, "#800000")

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
