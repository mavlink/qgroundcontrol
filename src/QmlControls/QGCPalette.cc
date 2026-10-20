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
    // clang-format off
    //                                      Light                 Dark
    //                                      Disabled   Enabled    Disabled   Enabled
    // Surfaces nest window -> windowPanel -> windowCard; windowInset is for input wells inside any of them
    DECLARE_QGC_COLOR(window,               "#f2f3f5", "#f2f3f5", "#0b0d10", "#0b0d10")
    DECLARE_QGC_COLOR(windowTransparent,    "#ccf2f3f5", "#ccf2f3f5", "#cc0b0d10", "#cc0b0d10")
    DECLARE_QGC_COLOR(windowPanel,          "#ffffff", "#ffffff", "#121519", "#121519")
    DECLARE_QGC_COLOR(windowCard,           "#f7f8fa", "#f7f8fa", "#181b20", "#181b20")
    DECLARE_QGC_COLOR(windowInset,          "#f2f3f5", "#ffffff", "#0e1013", "#0e1013")
    DECLARE_QGC_COLOR(borderSubtle,         "#e3e5e9", "#d5d8dd", "#1f2228", "#2a2e35")
    DECLARE_QGC_COLOR(textPrimary,          "#a3a8b0", "#1d2025", "#50555c", "#e6e8eb")
    DECLARE_QGC_COLOR(textSecondary,        "#b8bcc2", "#5f6670", "#43474d", "#8b919a")
    DECLARE_QGC_COLOR(accent,               "#e4e4e4", "#3A9BDC", "#3a3a3a", "#3A9BDC")
    DECLARE_QGC_COLOR(accentSubtle,         "#f2f3f5", "#e1f0fa", "#15171a", "#10273a")
    DECLARE_QGC_COLOR(accentText,           "#2c2c2c", "#ffffff", "#2c2c2c", "#ffffff")
    DECLARE_QGC_COLOR(errorText,            "#bd0707", "#bd0707", "#ff7070", "#ff7070")
    DECLARE_QGC_COLOR(warningText,          "#9a4d00", "#9a4d00", "#e18700", "#e18700")
    DECLARE_QGC_COLOR(successText,          "#006b22", "#006b22", "#00e04b", "#00e04b")
    DECLARE_QGC_COLOR(mapButton,            "#585858", "#333333", "#585858", "#000000")
    DECLARE_QGC_COLOR(mapButtonHighlight,   "#585858", "#be781c", "#585858", "#be781c")
    DECLARE_QGC_COLOR(mapIndicator,         "#585858", "#be781c", "#585858", "#be781c")
    DECLARE_QGC_COLOR(mapIndicatorChild,    "#585858", "#766043", "#585858", "#766043")
    DECLARE_QGC_COLOR(colorGreen,           "#008f2d", "#008f2d", "#00e04b", "#00e04b")
    DECLARE_QGC_COLOR(colorYellow,          "#a2a200", "#a2a200", "#ffff00", "#ffff00")
    DECLARE_QGC_COLOR(colorYellowGreen,     "#799f26", "#799f26", "#9dbe2f", "#9dbe2f")
    DECLARE_QGC_COLOR(colorOrange,          "#bf7539", "#bf7539", "#de8500", "#de8500")
    DECLARE_QGC_COLOR(colorRed,             "#b52b2b", "#b52b2b", "#f32836", "#f32836")
    DECLARE_QGC_COLOR(colorGrey,            "#808080", "#808080", "#bfbfbf", "#bfbfbf")
    DECLARE_QGC_COLOR(colorBlue,            "#1a72ff", "#1a72ff", "#536dff", "#536dff")
    DECLARE_QGC_COLOR(alertBackground,      "#eecc44", "#eecc44", "#eecc44", "#eecc44")
    DECLARE_QGC_COLOR(alertBorder,          "#808080", "#808080", "#808080", "#808080")
    DECLARE_QGC_COLOR(alertText,            "#000000", "#000000", "#000000", "#000000")
    DECLARE_QGC_COLOR(missionItemEditor,    "#585858", "#dbfef8", "#585858", "#585d83")
    DECLARE_QGC_COLOR(toolStripHoverColor,  "#585858", "#9D9D9D", "#585858", "#585d83")
    DECLARE_QGC_COLOR(toolbarBackground,    "#00ffffff", "#00ffffff", "#00222222", "#00222222")
    DECLARE_QGC_COLOR(modifiedParamValue,   "#bf7539", "#bf7539", "#de8500", "#de8500")

    // Colors not affecting by theming
    //                                                      Disabled     Enabled
    DECLARE_QGC_NONTHEMED_COLOR(brandPrimary,               "#4A2C6D", "#4A2C6D")
    DECLARE_QGC_NONTHEMED_COLOR(brandAccent,                "#48D6FF", "#6045c5")
    DECLARE_QGC_NONTHEMED_COLOR(toolStripFGColor,           "#707070", "#ffffff")
    DECLARE_QGC_NONTHEMED_COLOR(photoCaptureButtonColor,    "#707070", "#ffffff")
    DECLARE_QGC_NONTHEMED_COLOR(videoCaptureButtonColor,    "#f89a9e", "#f32836")

    // Colors not affecting by theming or enable/disable
    DECLARE_QGC_SINGLE_COLOR(mapWidgetBorderLight,          "#ffffff")
    DECLARE_QGC_SINGLE_COLOR(mapWidgetBorderDark,           "#000000")
    DECLARE_QGC_SINGLE_COLOR(mapMissionTrajectory,          "#ffa000")
    DECLARE_QGC_SINGLE_COLOR(surveyPolygonInterior,         "green")
    DECLARE_QGC_SINGLE_COLOR(surveyPolygonTerrainCollision, "red")
    // Badge fills sized for indicatorText (white) at >= 4.5:1 contrast in either theme
    DECLARE_QGC_SINGLE_COLOR(errorIndicator,                "#c62828")
    DECLARE_QGC_SINGLE_COLOR(warningIndicator,              "#b45309")
    DECLARE_QGC_SINGLE_COLOR(successIndicator,              "#2e7d32")
    DECLARE_QGC_SINGLE_COLOR(indicatorText,                 "#ffffff")
    // clang-format on
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
