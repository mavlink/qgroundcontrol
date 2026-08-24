#include "SysStatusSensorInfo.h"
#include "QGCLoggingCategory.h"
#include "QGCMAVLink.h"

QGC_LOGGING_CATEGORY(SysStatusSensorInfoLog, "MAVLink.SysStatusSensorInfo")

SysStatusSensorInfo::SysStatusSensorInfo(QObject *parent)
    : QObject(parent)
{
    // qCDebug(SysStatusSensorInfoLog) << Q_FUNC_INFO << this;
}

SysStatusSensorInfo::~SysStatusSensorInfo()
{
    // qCDebug(SysStatusSensorInfoLog) << Q_FUNC_INFO << this;
}

void SysStatusSensorInfo::update(const mavlink_sys_status_t &sysStatus)
{
    bool dirty = false;
    const QDateTime now = QDateTime::currentDateTimeUtc();

    // Walk the bits
    for (int bitPosition = 0; bitPosition < 32; bitPosition++) {
        const MAV_SYS_STATUS_SENSOR sensorBitMask = static_cast<MAV_SYS_STATUS_SENSOR>(1 << bitPosition);
        if (sysStatus.onboard_control_sensors_present & sensorBitMask) {
            if (_sensorInfoMap.contains(sensorBitMask)) {
                SensorInfo &sensorInfo = _sensorInfoMap[sensorBitMask];
                sensorInfo.lastUpdated = now;

                if (!sensorInfo.present) {
                    dirty = true;
                    sensorInfo.present = true;
                }

                const bool newEnabled = sysStatus.onboard_control_sensors_enabled & sensorBitMask;
                if (sensorInfo.enabled != newEnabled) {
                    dirty = true;
                    sensorInfo.enabled = newEnabled;
                }

                const bool newHealthy = sysStatus.onboard_control_sensors_health & sensorBitMask;
                if (sensorInfo.healthy != newHealthy) {
                    dirty = true;
                    sensorInfo.healthy = newHealthy;
                }
            } else {
                dirty = true;
                const SensorInfo sensorInfo = { !!(sysStatus.onboard_control_sensors_enabled & sensorBitMask), !!(sysStatus.onboard_control_sensors_health & sensorBitMask), true, now };
                _sensorInfoMap[sensorBitMask] = sensorInfo;
            }
        } else if (_sensorInfoMap.contains(sensorBitMask)) {
            // A sensor that stops being reported is a failure signal, not a reason to shrink the
            // list: retain the entry, flag it Lost, and keep the last time it was actually seen.
            SensorInfo &sensorInfo = _sensorInfoMap[sensorBitMask];
            if (sensorInfo.present) {
                dirty = true;
                sensorInfo.present = false;
            }
        }
    }

    if (dirty) {
        emit sensorInfoChanged();
    }
}

QDateTime SysStatusSensorInfo::sensorLastUpdated(int sensorBitMask) const
{
    return _sensorInfoMap.value(static_cast<MAV_SYS_STATUS_SENSOR>(sensorBitMask)).lastUpdated;
}

QStringList SysStatusSensorInfo::sensorNames() const
{
    QStringList rgNames;

    // List ordering is lost, unhealthy, healthy, disabled — must match sensorStatus()
    for (std::pair<MAV_SYS_STATUS_SENSOR, const SensorInfo&> sensorInfo : _sensorInfoMap.asKeyValueRange()) {
        if (!sensorInfo.second.present) {
            rgNames.append(QGCMAVLink::mavSysStatusSensorToString(sensorInfo.first));
        }
    }

    for (std::pair<MAV_SYS_STATUS_SENSOR, const SensorInfo&> sensorInfo : _sensorInfoMap.asKeyValueRange()) {
        if (sensorInfo.second.present && sensorInfo.second.enabled && !sensorInfo.second.healthy) {
            rgNames.append(QGCMAVLink::mavSysStatusSensorToString(sensorInfo.first));
        }
    }

    for (std::pair<MAV_SYS_STATUS_SENSOR, const SensorInfo&> sensorInfo : _sensorInfoMap.asKeyValueRange()) {
        if (sensorInfo.second.present && sensorInfo.second.enabled && sensorInfo.second.healthy) {
            rgNames.append(QGCMAVLink::mavSysStatusSensorToString(sensorInfo.first));
        }
    }

    for (std::pair<MAV_SYS_STATUS_SENSOR, const SensorInfo&> sensorInfo : _sensorInfoMap.asKeyValueRange()) {
        if (sensorInfo.second.present && !sensorInfo.second.enabled) {
            rgNames.append(QGCMAVLink::mavSysStatusSensorToString(sensorInfo.first));
        }
    }

    return rgNames;
}

QStringList SysStatusSensorInfo::sensorStatus() const
{
    QStringList rgStatus;

    // List ordering is lost, unhealthy, healthy, disabled — must match sensorNames()
    for (const SensorInfo &sensorInfo : _sensorInfoMap.values()) {
        if (!sensorInfo.present) {
            rgStatus.append(tr("Lost"));
        }
    }

    for (const SensorInfo &sensorInfo : _sensorInfoMap.values()) {
        if (sensorInfo.present && sensorInfo.enabled && !sensorInfo.healthy) {
            rgStatus.append(tr("Error"));
        }
    }

    for (const SensorInfo &sensorInfo : _sensorInfoMap.values()) {
        if (sensorInfo.present && sensorInfo.enabled && sensorInfo.healthy) {
            rgStatus.append(tr("Normal"));
        }
    }

    for (const SensorInfo &sensorInfo : _sensorInfoMap.values()) {
        if (sensorInfo.present && !sensorInfo.enabled) {
            rgStatus.append(tr("Disabled"));
        }
    }

    return rgStatus;
}
