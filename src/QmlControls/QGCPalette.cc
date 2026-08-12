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
    DECLARE_QGC_COLOR(window,               "#F5F7FA", "#FFFFFF", "#111111", "#111111")
    DECLARE_QGC_COLOR(windowShadeLight,     "#F8F9FA", "#FFFFFF", "#2B2F33", "#2B2F33")
    DECLARE_QGC_COLOR(windowShade,          "#ECEFF1", "#F5F7FA", "#1B1F24", "#1B1F24")
    DECLARE_QGC_COLOR(windowShadeDark,      "#D4D8DD", "#D4D8DD", "#000000", "#000000")

    DECLARE_QGC_COLOR(text,                 "#7A7F86", "#111111", "#B8BEC5", "#FFFFFF")
    DECLARE_QGC_COLOR(warningText,          "#E53935", "#E53935", "#E53935", "#E53935")

    DECLARE_QGC_COLOR(button,               "#ECEFF1", "#FFFFFF", "#1B1F24", "#2B2F33")
    DECLARE_QGC_COLOR(buttonBorder,         "#C7CDD4", "#C7CDD4", "#444A52", "#444A52")
    DECLARE_QGC_COLOR(buttonText,           "#6B7280", "#111111", "#D1D5DB", "#FFFFFF")

    DECLARE_QGC_COLOR(buttonHighlight,      "#40464D", "#40464D", "#5A626B", "#5A626B")
    DECLARE_QGC_COLOR(buttonHighlightText,  "#FFFFFF", "#FFFFFF", "#FFFFFF", "#FFFFFF")

    DECLARE_QGC_COLOR(primaryButton,        "#2B2F33", "#2B2F33", "#FFFFFF", "#FFFFFF")
    DECLARE_QGC_COLOR(primaryButtonText,    "#FFFFFF", "#FFFFFF", "#111111", "#111111")

    DECLARE_QGC_COLOR(textField,            "#ECEFF1", "#FFFFFF", "#1B1F24", "#2B2F33")
    DECLARE_QGC_COLOR(textFieldText,        "#6B7280", "#111111", "#D1D5DB", "#FFFFFF")

    DECLARE_QGC_COLOR(mapButton,            "#ECEFF1", "#FFFFFF", "#1B1F24", "#2B2F33")
    DECLARE_QGC_COLOR(mapButtonHighlight,   "#40464D", "#40464D", "#5A626B", "#5A626B")

    DECLARE_QGC_COLOR(mapIndicator,         "#40464D", "#40464D", "#FFFFFF", "#FFFFFF")
    DECLARE_QGC_COLOR(mapIndicatorChild,    "#7A7F86", "#7A7F86", "#D1D5DB", "#D1D5DB")

    DECLARE_QGC_COLOR(colorGreen,           "#2E7D32", "#2E7D32", "#2E7D32", "#2E7D32")
    DECLARE_QGC_COLOR(colorYellow,          "#6B7280", "#6B7280", "#6B7280", "#6B7280")
    DECLARE_QGC_COLOR(colorYellowGreen,     "#6B7280", "#6B7280", "#6B7280", "#6B7280")
    DECLARE_QGC_COLOR(colorOrange,          "#6B7280", "#6B7280", "#6B7280", "#6B7280")

    DECLARE_QGC_COLOR(colorRed,             "#E53935", "#E53935", "#E53935", "#E53935")

    DECLARE_QGC_COLOR(colorGrey,            "#9CA3AF", "#6B7280", "#6B7280", "#D1D5DB")
    DECLARE_QGC_COLOR(colorBlue,            "#40464D", "#40464D", "#40464D", "#40464D")

    DECLARE_QGC_COLOR(alertBackground,      "#FFF5F5", "#FFF5F5", "#3A1111", "#3A1111")
    DECLARE_QGC_COLOR(alertBorder,          "#E53935", "#E53935", "#E53935", "#E53935")
    DECLARE_QGC_COLOR(alertText,            "#E53935", "#E53935", "#FFFFFF", "#FFFFFF")

    DECLARE_QGC_COLOR(missionItemEditor,    "#ECEFF1", "#FFFFFF", "#1B1F24", "#2B2F33")
    DECLARE_QGC_COLOR(toolStripHoverColor,  "#E5E7EB", "#E5E7EB", "#3A4047", "#3A4047")

    DECLARE_QGC_COLOR(statusFailedText,     "#E53935", "#E53935", "#E53935", "#E53935")
    DECLARE_QGC_COLOR(statusPassedText,     "#2E7D32", "#2E7D32", "#2E7D32", "#2E7D32")
    DECLARE_QGC_COLOR(statusPendingText,    "#6B7280", "#6B7280", "#6B7280", "#6B7280")

    DECLARE_QGC_COLOR(toolbarBackground,    "#FFFFFF", "#FFFFFF", "#111111", "#111111")
    DECLARE_QGC_COLOR(groupBorder,          "#D4D8DD", "#D4D8DD", "#3A4047", "#3A4047")

    // Colors not affecting by theming
    //                                              Disabled    Enabled
    DECLARE_QGC_NONTHEMED_COLOR(brandingPurple,     "#1B1F24", "#1B1F24")
    DECLARE_QGC_NONTHEMED_COLOR(brandingBlue,       "#40464D", "#40464D")
    DECLARE_QGC_NONTHEMED_COLOR(toolStripFGColor,   "#6B7280", "#FFFFFF")

    // Colors not affecting by theming or enable/disable
    DECLARE_QGC_SINGLE_COLOR(mapWidgetBorderLight,          "#FFFFFF")
    DECLARE_QGC_SINGLE_COLOR(mapWidgetBorderDark,           "#000000")
    DECLARE_QGC_SINGLE_COLOR(mapMissionTrajectory,          "#40464D")
    DECLARE_QGC_SINGLE_COLOR(surveyPolygonInterior,         "#2E7D32")
    DECLARE_QGC_SINGLE_COLOR(surveyPolygonTerrainCollision, "#E53935")

// Colors for UTM Adapter
#ifdef QGC_UTM_ADAPTER
    DECLARE_QGC_COLOR(switchUTMSP,        "#E5E7EB", "#E5E7EB", "#3A4047", "#3A4047")
    DECLARE_QGC_COLOR(sliderUTMSP,        "#40464D", "#40464D", "#5A626B", "#5A626B")
    DECLARE_QGC_COLOR(successNotifyUTMSP, "#2E7D32", "#2E7D32", "#2E7D32", "#2E7D32")
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
