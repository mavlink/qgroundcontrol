/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "PlatformDetector.h"

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>

Q_DECLARE_LOGGING_CATEGORY(TileQualitySettingsLog)

enum class TileQuality {
    Low,       // 256x256, lower zoom levels
    Medium,    // 512x512, standard
    High,      // 1024x1024, retina/4K displays
    Adaptive   // Auto-adjust based on connection speed
};

Q_DECLARE_METATYPE(TileQuality)

class TileQualitySettings : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(TileQuality quality READ quality WRITE setQuality NOTIFY qualityChanged)
    Q_PROPERTY(int tileSize READ tileSize NOTIFY tileSizeChanged)
    Q_PROPERTY(bool adaptiveQuality READ adaptiveQuality WRITE setAdaptiveQuality NOTIFY adaptiveQualityChanged)
    Q_PROPERTY(quint64 bandwidthLimit READ bandwidthLimit WRITE setBandwidthLimit NOTIFY bandwidthLimitChanged)

public:
    static TileQualitySettings* instance();

    TileQuality quality() const { return _quality; }
    void setQuality(TileQuality quality);

    int tileSize() const;
    int tileSizeForQuality(TileQuality quality) const;

    bool adaptiveQuality() const { return _adaptiveQuality; }
    void setAdaptiveQuality(bool adaptive);

    quint64 bandwidthLimit() const { return _bandwidthLimit; }
    void setBandwidthLimit(quint64 bytesPerSec);

    // Adaptive quality adjustment
    void adjustQualityForBandwidth(quint64 currentBandwidth);

    // Persistence
    void load();
    void save();

signals:
    void qualityChanged();
    void tileSizeChanged();
    void adaptiveQualityChanged();
    void bandwidthLimitChanged();

private:
    explicit TileQualitySettings(QObject *parent = nullptr);

    TileQuality _quality = TileQuality::Medium;
    bool _adaptiveQuality = true;
    quint64 _bandwidthLimit = 0;
};
