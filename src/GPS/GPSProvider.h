/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QLoggingCategory>
#include <QtCore/QMetaType>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QThread>

#include <gps_helper.h>

#include "satellite_info.h"
#include "sensor_gnss_relative.h"
#include "sensor_gps.h"

Q_DECLARE_LOGGING_CATEGORY(GPSProviderLog)

class QSerialPort;
class GPSBaseStationSupport;

class GPSProvider : public QThread
{
    Q_OBJECT

public:
    enum class GPSType {
        u_blox,
        trimble,
        septentrio,
        femto
    };

    struct rtk_data_s {
        double surveyInAccMeters = 0;
        int surveyInDurationSecs = 0;
        bool useFixedBaseLoction = false;
        double fixedBaseLatitude = 0.;
        double fixedBaseLongitude = 0.;
        float fixedBaseAltitudeMeters = 0.f;
        float fixedBaseAccuracyMeters = 0.f;
    };

    GPSProvider(const QString &device, GPSType type, const rtk_data_s &rtkData, const std::atomic_bool &requestStop, QObject *parent = nullptr);
    ~GPSProvider();

    int callback(GPSCallbackType type, void *data1, int data2);

signals:
    void satelliteInfoUpdate(const satellite_info_s &message);
    void sensorGnssRelativeUpdate(const sensor_gnss_relative_s &message);
    void sensorGpsUpdate(const sensor_gps_s &message);
    void RTCMDataUpdate(const QByteArray &message);
    void surveyInStatus(float duration, float accuracyMM, double latitude, double longitude, float altitude, bool valid, bool active);

private:
    void run() final;

    bool _connectSerial();
    GPSBaseStationSupport *_connectGPS();
    void _publishSensorGPS();
    void _publishSatelliteInfo();
    void _publishSensorGNSSRelative();

    void _gotRTCMData(const uint8_t *data, size_t len);
    void _sendRTCMData();

    static int _callbackEntry(GPSCallbackType type, void *data1, int data2, void *user);

    QString _device;
    GPSType _type;
    const std::atomic_bool &_requestStop;
    rtk_data_s _rtkData{};
    GPSHelper::GPSConfig _gpsConfig{};

    struct satellite_info_s _satelliteInfo{};
    struct sensor_gnss_relative_s _sensorGnssRelative{};
    struct sensor_gps_s _sensorGps{};

    QSerialPort *_serial = nullptr;

    enum GPSReceiveType {
        Position = 1,
        Satellite = 2
    };

    static constexpr uint32_t kGPSReceiveTimeout = 1200;
    static constexpr float kGPSHeadingOffset = 5.f;
};
