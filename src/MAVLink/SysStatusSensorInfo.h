#pragma once

#include <QtCore/QMap>
#include <QtCore/QObject>

#include "MAVLinkLib.h"


/// Class which represents sensor info from the SYS_STATUS mavlink message
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

signals:
    void sensorInfoChanged();

private:
    struct SensorInfo {
        bool enabled = false;
        bool healthy = false;
    };

    QMap<MAV_SYS_STATUS_SENSOR, SensorInfo> _sensorInfoMap;
};
