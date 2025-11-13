/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "TileQualitySettings.h"

#include <QtCore/QSettings>

QGC_LOGGING_CATEGORY(TileQualitySettingsLog, "qgc.qtlocationplugin.qualitysettings")

TileQualitySettings::TileQualitySettings(QObject *parent)
    : QObject(parent)
{
    // Apply platform defaults
    const auto defaults = PlatformDetector::defaults();
    _quality = (defaults.defaultTileSize == 256) ? TileQuality::Low :
               (defaults.defaultTileSize == 512) ? TileQuality::Medium : TileQuality::High;
    _bandwidthLimit = defaults.maxBandwidthBytesPerSec;

    load();
}

TileQualitySettings* TileQualitySettings::instance()
{
    static TileQualitySettings *_instance = nullptr;
    if (!_instance) {
        _instance = new TileQualitySettings();
    }
    return _instance;
}

void TileQualitySettings::setQuality(TileQuality quality)
{
    if (_quality != quality) {
        _quality = quality;
        emit qualityChanged();
        emit tileSizeChanged();
        save();
        qCDebug(TileQualitySettingsLog) << "Quality changed to:" << static_cast<int>(quality);
    }
}

int TileQualitySettings::tileSize() const
{
    if (_adaptiveQuality && _quality == TileQuality::Adaptive) {
        // Return adaptive size based on current conditions
        return tileSizeForQuality(TileQuality::Medium);
    }
    return tileSizeForQuality(_quality);
}

int TileQualitySettings::tileSizeForQuality(TileQuality quality) const
{
    switch (quality) {
    case TileQuality::Low: return 256;
    case TileQuality::Medium: return 512;
    case TileQuality::High: return 1024;
    case TileQuality::Adaptive: return 512;  // Default for adaptive
    }
    return 512;
}

void TileQualitySettings::setAdaptiveQuality(bool adaptive)
{
    if (_adaptiveQuality != adaptive) {
        _adaptiveQuality = adaptive;
        emit adaptiveQualityChanged();
        save();
        qCDebug(TileQualitySettingsLog) << "Adaptive quality:" << adaptive;
    }
}

void TileQualitySettings::setBandwidthLimit(quint64 bytesPerSec)
{
    if (_bandwidthLimit != bytesPerSec) {
        _bandwidthLimit = bytesPerSec;
        emit bandwidthLimitChanged();
        save();
        qCDebug(TileQualitySettingsLog) << "Bandwidth limit:" << bytesPerSec;
    }
}

void TileQualitySettings::adjustQualityForBandwidth(quint64 currentBandwidth)
{
    if (!_adaptiveQuality || _quality != TileQuality::Adaptive) {
        return;
    }

    // Adaptive thresholds (bytes per second)
    constexpr quint64 kHighQualityThreshold = 5 * 1024 * 1024;   // 5 MB/s
    constexpr quint64 kMediumQualityThreshold = 1 * 1024 * 1024; // 1 MB/s

    TileQuality newQuality = _quality;
    if (currentBandwidth >= kHighQualityThreshold) {
        newQuality = TileQuality::High;
    } else if (currentBandwidth >= kMediumQualityThreshold) {
        newQuality = TileQuality::Medium;
    } else {
        newQuality = TileQuality::Low;
    }

    if (_quality != newQuality) {
        qCDebug(TileQualitySettingsLog) << "Adaptive quality adjusted based on bandwidth:" << currentBandwidth;
        setQuality(newQuality);
    }
}

void TileQualitySettings::save()
{
    QSettings settings;
    settings.beginGroup("TileQuality");
    settings.setValue("quality", static_cast<int>(_quality));
    settings.setValue("adaptiveQuality", _adaptiveQuality);
    settings.setValue("bandwidthLimit", _bandwidthLimit);
    settings.endGroup();
}

void TileQualitySettings::load()
{
    QSettings settings;
    settings.beginGroup("TileQuality");
    _quality = static_cast<TileQuality>(settings.value("quality", static_cast<int>(_quality)).toInt());
    _adaptiveQuality = settings.value("adaptiveQuality", _adaptiveQuality).toBool();
    _bandwidthLimit = settings.value("bandwidthLimit", _bandwidthLimit).toULongLong();
    settings.endGroup();
}
