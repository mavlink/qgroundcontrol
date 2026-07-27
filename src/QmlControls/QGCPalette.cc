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
    DECLARE_QGC_COLOR(window,               "#ffffff", "#ffffff", "#101214", "#101214")
    DECLARE_QGC_COLOR(windowShadeLight,     "#909090", "#828282", "#282d37", "#282d37")
    DECLARE_QGC_COLOR(windowShade,          "#d9d9d9", "#d9d9d9", "#1f232b", "#1f232b")
    DECLARE_QGC_COLOR(windowShadeDark,      "#bdbdbd", "#bdbdbd", "#181b20", "#181b20")
    DECLARE_QGC_COLOR(text,                 "#9d9d9d", "#000000", "#4a505b", "#ffffff")
    DECLARE_QGC_COLOR(warningText,          "#cc0808", "#cc0808", "#f85761", "#f85761")
    DECLARE_QGC_COLOR(button,               "#ffffff", "#ffffff", "#181b20", "#1f232b")
    DECLARE_QGC_COLOR(buttonBorder,         "#ffffff", "#d9d9d9", "#282d37", "#4a505b")
    DECLARE_QGC_COLOR(buttonText,           "#9d9d9d", "#000000", "#8b949e", "#ffffff")
    DECLARE_QGC_COLOR(buttonHighlight,      "#e4e4e4", "#FF6A00", "#282d37", "#FF6A00")
    DECLARE_QGC_COLOR(buttonHighlightText,  "#2c2c2c", "#ffffff", "#e1e4e8", "#ffffff")
    DECLARE_QGC_COLOR(primaryButton,        "#585858", "#FF6A00", "#282d37", "#FF6A00")
    DECLARE_QGC_COLOR(primaryButtonText,    "#2c2c2c", "#ffffff", "#8b949e", "#ffffff")
    DECLARE_QGC_COLOR(textField,            "#ffffff", "#ffffff", "#181b20", "#1f232b")
    DECLARE_QGC_COLOR(textFieldText,        "#808080", "#000000", "#8b949e", "#ffffff")
    DECLARE_QGC_COLOR(mapButton,            "#585858", "#000000", "#4a505b", "#1f232b")
    DECLARE_QGC_COLOR(mapButtonHighlight,   "#585858", "#FF6A00", "#585858", "#FF6A00")
    DECLARE_QGC_COLOR(mapIndicator,         "#585858", "#FF6A00", "#585858", "#FF6A00")
    DECLARE_QGC_COLOR(mapIndicatorChild,    "#585858", "#FF8533", "#585858", "#FF8533")
    DECLARE_QGC_COLOR(colorGreen,           "#008f2d", "#008f2d", "#00e04b", "#00e04b") 
    DECLARE_QGC_COLOR(colorYellow,          "#a2a200", "#a2a200", "#ffff00", "#ffff00")  
    DECLARE_QGC_COLOR(colorYellowGreen,     "#799f26", "#799f26", "#9dbe2f", "#9dbe2f")  
    DECLARE_QGC_COLOR(colorOrange,          "#FF6A00", "#FF6A00", "#FF6A00", "#FF6A00")  
    DECLARE_QGC_COLOR(colorRed,             "#b52b2b", "#b52b2b", "#f32836", "#f32836")
    DECLARE_QGC_COLOR(colorGrey,            "#808080", "#808080", "#4a505b", "#8b949e")
    DECLARE_QGC_COLOR(colorBlue,            "#FF6A00", "#FF6A00", "#FF6A00", "#FF6A00")
    DECLARE_QGC_COLOR(alertBackground,      "#eecc44", "#eecc44", "#eecc44", "#eecc44")
    DECLARE_QGC_COLOR(alertBorder,          "#808080", "#808080", "#808080", "#808080")
    DECLARE_QGC_COLOR(alertText,            "#000000", "#000000", "#000000", "#000000")
    DECLARE_QGC_COLOR(missionItemEditor,    "#585858", "#FF6A00", "#282d37", "#1f232b")
    DECLARE_QGC_COLOR(toolStripHoverColor,  "#585858", "#9D9D9D", "#282d37", "#FF8533")
    DECLARE_QGC_COLOR(statusFailedText,     "#9d9d9d", "#000000", "#707070", "#ffffff")
    DECLARE_QGC_COLOR(statusPassedText,     "#9d9d9d", "#000000", "#707070", "#ffffff")
    DECLARE_QGC_COLOR(statusPendingText,    "#9d9d9d", "#000000", "#707070", "#ffffff")
    DECLARE_QGC_COLOR(toolbarBackground,    "#ffffff", "#ffffff", "#181b20", "#181b20")
    DECLARE_QGC_COLOR(groupBorder,          "#bbbbbb", "#bbbbbb", "#282d37", "#4a505b")

    // Colors not affecting by theming
    //                                              Disabled    Enabled
    DECLARE_QGC_NONTHEMED_COLOR(brandingPurple,     "#181b20", "#181b20")
    DECLARE_QGC_NONTHEMED_COLOR(brandingBlue,       "#FF6A00", "#FF6A00")
    DECLARE_QGC_NONTHEMED_COLOR(toolStripFGColor,   "#707070", "#ffffff")

    // Colors not affecting by theming or enable/disable
    DECLARE_QGC_SINGLE_COLOR(mapWidgetBorderLight,          "#ffffff")
    DECLARE_QGC_SINGLE_COLOR(mapWidgetBorderDark,           "#000000")
    DECLARE_QGC_SINGLE_COLOR(mapMissionTrajectory,          "#FF6A00")
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
