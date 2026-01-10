#include "QGCPalette.h"

#include <QtCore/QDebug>

#include "QGCCorePlugin.h"

QList<QGCPalette*> QGCPalette::_paletteObjects;

QGCPalette::Theme QGCPalette::_theme = QGCPalette::Dark;

QMap<int, QMap<int, QMap<QString, QColor>>> QGCPalette::_colorInfoMap;

QStringList QGCPalette::_colors;

QGCPalette::QGCPalette(QObject* parent) : QObject(parent), _colorGroupEnabled(true)
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
    // FUTURISTIC NEON DARK THEME - Pure black backgrounds with cyan/blue accents
    DECLARE_QGC_COLOR(window, "#ffffff", "#ffffff", "#050508", "#050508")
    DECLARE_QGC_COLOR(windowTransparent, "#ccffffff", "#ccffffff", "#ee050508", "#ee050508")
    DECLARE_QGC_COLOR(windowShadeLight, "#909090", "#828282", "#0f1012", "#0f1012")
    DECLARE_QGC_COLOR(windowShade, "#d9d9d9", "#d9d9d9", "#0a0a0f", "#0a0a0f")
    DECLARE_QGC_COLOR(windowShadeDark, "#bdbdbd", "#bdbdbd", "#030305", "#030305")
    DECLARE_QGC_COLOR(text, "#9d9d9d", "#333333", "#707070", "#ffffff")
    DECLARE_QGC_COLOR(windowTransparentText, "#9d9d9d", "#000000", "#707070", "#ffffff")
    DECLARE_QGC_COLOR(warningText, "#cc0808", "#cc0808", "#ff3366", "#ff3366")
    DECLARE_QGC_COLOR(button, "#ffffff", "#ffffff", "#0a0c10", "#0a0c10")
    DECLARE_QGC_COLOR(buttonBorder, "#9d9d9d", "#3A9BDC", "#00a8ff", "#00a8ff")
    DECLARE_QGC_COLOR(buttonText, "#9d9d9d", "#333333", "#A6A6A6", "#ffffff")
    DECLARE_QGC_COLOR(buttonHighlight, "#e4e4e4", "#3A9BDC", "#001a1f", "#00fff2")
    DECLARE_QGC_COLOR(buttonHighlightText, "#2c2c2c", "#ffffff", "#00fff2", "#000000")
    DECLARE_QGC_COLOR(primaryButton, "#585858", "#8cb3be", "#001a1f", "#001a1f")
    DECLARE_QGC_COLOR(primaryButtonText, "#2c2c2c", "#333333", "#00fff2", "#00fff2")
    DECLARE_QGC_COLOR(textField, "#ffffff", "#ffffff", "#707070", "#ffffff")
    DECLARE_QGC_COLOR(textFieldText, "#808080", "#333333", "#000000", "#000000")
    DECLARE_QGC_COLOR(mapButton, "#585858", "#333333", "#0a0a0f", "#000000")
    DECLARE_QGC_COLOR(mapButtonHighlight, "#585858", "#be781c", "#001a1f", "#00fff2")
    DECLARE_QGC_COLOR(mapIndicator, "#585858", "#be781c", "#00a8ff", "#00fff2")
    DECLARE_QGC_COLOR(mapIndicatorChild, "#585858", "#766043", "#0066cc", "#00a8ff")
    DECLARE_QGC_COLOR(colorGreen, "#008f2d", "#008f2d", "#39ff14", "#39ff14")
    DECLARE_QGC_COLOR(colorYellow, "#a2a200", "#a2a200", "#ffff00", "#ffff00")
    DECLARE_QGC_COLOR(colorYellowGreen, "#799f26", "#799f26", "#aaff00", "#aaff00")
    DECLARE_QGC_COLOR(colorOrange, "#bf7539", "#bf7539", "#ff6600", "#ff6600")
    DECLARE_QGC_COLOR(colorRed, "#b52b2b", "#b52b2b", "#ff0044", "#ff0044")
    DECLARE_QGC_COLOR(colorGrey, "#808080", "#808080", "#888888", "#888888")
    DECLARE_QGC_COLOR(colorBlue, "#1a72ff", "#1a72ff", "#00fff2", "#00fff2")
    DECLARE_QGC_COLOR(alertBackground, "#eecc44", "#eecc44", "#1a1a00", "#1a1a00")
    DECLARE_QGC_COLOR(alertBorder, "#808080", "#808080", "#ffff00", "#ffff00")
    DECLARE_QGC_COLOR(alertText, "#000000", "#000000", "#ffffff", "#ffffff")
    DECLARE_QGC_COLOR(missionItemEditor, "#585858", "#dbfef8", "#0a0a0f", "#0a1520")
    DECLARE_QGC_COLOR(toolStripHoverColor, "#585858", "#9D9D9D", "#001a1f", "#001a1f")
    DECLARE_QGC_COLOR(statusFailedText, "#9d9d9d", "#000000", "#ff0044", "#ffffff")
    DECLARE_QGC_COLOR(statusPassedText, "#9d9d9d", "#000000", "#39ff14", "#ffffff")
    DECLARE_QGC_COLOR(statusPendingText, "#9d9d9d", "#000000", "#ffff00", "#ffffff")
    DECLARE_QGC_COLOR(toolbarBackground, "#00ffffff", "#00ffffff", "#ee050508", "#ee050508")
    DECLARE_QGC_COLOR(groupBorder, "#bbbbbb", "#3A9BDC", "#00a8ff", "#00a8ff")

    // Colors not affecting by theming
    //                                                      Disabled     Enabled
    DECLARE_QGC_NONTHEMED_COLOR(brandingPurple, "#00141a", "#00141a")
    DECLARE_QGC_NONTHEMED_COLOR(brandingBlue, "#00fff2", "#00fff2")
    DECLARE_QGC_NONTHEMED_COLOR(toolStripFGColor, "#00fff2", "#ffffff")
    DECLARE_QGC_NONTHEMED_COLOR(photoCaptureButtonColor, "#707070", "#ffffff")
    DECLARE_QGC_NONTHEMED_COLOR(videoCaptureButtonColor, "#f89a9e", "#f32836")

    // Colors not affecting by theming or enable/disable
    DECLARE_QGC_SINGLE_COLOR(mapWidgetBorderLight, "#ffffff")
    DECLARE_QGC_SINGLE_COLOR(mapWidgetBorderDark, "#000000")
    DECLARE_QGC_SINGLE_COLOR(mapMissionTrajectory, "#00fff2")
    DECLARE_QGC_SINGLE_COLOR(surveyPolygonInterior, "#39ff14")
    DECLARE_QGC_SINGLE_COLOR(surveyPolygonTerrainCollision, "#ff0044")

// Colors for UTM Adapter
#ifdef QGC_UTM_ADAPTER
    DECLARE_QGC_COLOR(switchUTMSP, "#b0e0e6", "#b0e0e6", "#b0e0e6", "#b0e0e6");
    DECLARE_QGC_COLOR(sliderUTMSP, "#9370db", "#9370db", "#9370db", "#9370db");
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
    for (QGCPalette* palette : std::as_const(_paletteObjects)) {
        palette->_signalPaletteChanged();
    }
}

void QGCPalette::_signalPaletteChanged()
{
    emit paletteChanged();
}
