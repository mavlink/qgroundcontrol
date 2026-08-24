#pragma once

#include <QtCore/QDateTime>
#include <QtCore/QMap>
#include <QtCore/QObject>

#include "MAVLinkLib.h"

/// \brief Class which represents sensor info from the SYS_STATUS mavlink message
///
class SysStatusSensorInfo : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList sensorNames  READ sensorNames    NOTIFY sensorInfoChanged)
    Q_PROPERTY(QStringList sensorStatus READ sensorStatus   NOTIFY sensorInfoChanged)

public:
    explicit SysStatusSensorInfo(QObject *parent = nullptr);
    ~SysStatusSensorInfo();

    void update(const mavlink_sys_status_t &sysStatus);
    QStringList sensorNames() const;
    QStringList sensorStatus() const;

    /// Time the sensor's state was last reported in SYS_STATUS.
    /// Invalid QDateTime if the sensor has never been reported.
    Q_INVOKABLE QDateTime sensorLastUpdated(int sensorBitMask) const;

signals:
    void sensorInfoChanged();

private:
    struct SensorInfo {
        bool enabled = false;
        bool healthy = false;
        bool present = true;    ///< false: sensor dropped out of onboard_control_sensors_present mid-session
        QDateTime lastUpdated;  ///< last time this sensor was reported present
    };

    QMap<MAV_SYS_STATUS_SENSOR, SensorInfo> _sensorInfoMap;
};
