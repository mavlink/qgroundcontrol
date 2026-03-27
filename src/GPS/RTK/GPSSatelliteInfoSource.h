#pragma once

#include <QtCore/QLoggingCategory>
#include <QtPositioning/QGeoSatelliteInfoSource>

#include "satellite_info.h"

Q_DECLARE_LOGGING_CATEGORY(GPSSatelliteInfoSourceLog)

/// Bridges satellite_info_s updates from GPSProvider into Qt's QGeoSatelliteInfoSource API.
class GPSSatelliteInfoSource : public QGeoSatelliteInfoSource
{
    Q_OBJECT

public:
    explicit GPSSatelliteInfoSource(QObject *parent = nullptr);
    ~GPSSatelliteInfoSource() override;

    int minimumUpdateInterval() const override;
    Error error() const override;

public slots:
    void startUpdates() override;
    void stopUpdates() override;
    void requestUpdate(int timeout = 0) override;

    void updateFromSatelliteInfo(const satellite_info_s &info);

private:
    QList<QGeoSatelliteInfo> _lastInView;
    QList<QGeoSatelliteInfo> _lastInUse;
    bool _running = false;

    static constexpr int kMinUpdateIntervalMs = 100;
};
