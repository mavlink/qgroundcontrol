#pragma once

#include <atomic>

#include <QtCore/QByteArray>
#include <QtCore/QLoggingCategory>
#include <QtCore/QMetaType>
#include <QtCore/QMutex>
#include <QtCore/QObject>
#include <QtCore/QString>

#include <gps_helper.h>
#include "GPSTypes.h"
#include "Settings/RTKSettings.h"

#include "satellite_info.h"
#include "sensor_gnss_relative.h"
#include "sensor_gps.h"

Q_DECLARE_LOGGING_CATEGORY(GPSProviderLog)

class GPSTransport;

class GPSProvider : public QObject
{
    Q_OBJECT

public:
    struct rtk_data_s {
        double surveyInAccMeters = 0;
        int surveyInDurationSecs = 0;
        BaseModeDefinition::Mode useFixedBaseLocation = BaseModeDefinition::Mode::BaseSurveyIn;
        double fixedBaseLatitude = 0.;
        double fixedBaseLongitude = 0.;
        float fixedBaseAltitudeMeters = 0.f;
        float fixedBaseAccuracyMeters = 0.f;

        uint8_t outputMode = 1;     // GPSHelper::OutputMode: 0=GPS, 1=GPSAndRTCM, 2=RTCM
        uint8_t ubxMode = 0;       // GPSDriverUBX::UBXMode: 0=Normal, 1-6 see ubx.h
        uint8_t ubxDynamicModel = 7;
        int32_t ubxUart2Baudrate = 57600;
        bool ubxPpkOutput = false;
        uint16_t ubxDgnssTimeout = 0;
        uint8_t ubxMinCno = 0;
        uint8_t ubxMinElevation = 0;
        uint16_t ubxOutputRate = 0;
        bool ubxJamDetSensitivityHi = false;
    };

    struct SurveyInStatusData {
        float duration = 0.f;
        float accuracyMM = 0.f;
        double latitude = 0.;
        double longitude = 0.;
        float altitude = 0.f;
        bool valid = false;
        bool active = false;
    };

    GPSProvider(GPSTransport *transport, GPSType type, const rtk_data_s &rtkData, const std::atomic_bool &requestStop, QObject *parent = nullptr);
    ~GPSProvider();

    void injectRTCMData(const QByteArray &data);

    static rtk_data_s rtkDataFromSettings(RTKSettings &settings);

public slots:
    void process();

signals:
    void connectionEstablished();
    void satelliteInfoUpdate(const satellite_info_s &message);
    void sensorGnssRelativeUpdate(const sensor_gnss_relative_s &message);
    void sensorGpsUpdate(const sensor_gps_s &message);
    void RTCMDataUpdate(const QByteArray &message);
    void surveyInStatus(const GPSProvider::SurveyInStatusData &status);
    void error(const QString &errorMessage);
    void finished();

private:
    int _callback(GPSCallbackType type, void *data1, int data2);

    GPSHelper *_connectGPS();
    void _publishSensorGPS();
    void _publishSatelliteInfo();
    void _publishSensorGNSSRelative();

    void _gotRTCMData(const uint8_t *data, size_t len);
    void _drainRTCMBuffer();

    static int _callbackEntry(GPSCallbackType type, void *data1, int data2, void *user);

    GPSTransport *_transport = nullptr;
    GPSType _type;
    const std::atomic_bool &_requestStop; // must outlive this object
    rtk_data_s _rtkData{};
    GPSHelper::GPSConfig _gpsConfig{};

    // Worker-thread-only: written in _callback(), emitted by-value via queued signals
    struct satellite_info_s _satelliteInfo{};
    struct sensor_gnss_relative_s _sensorGnssRelative{};
    struct sensor_gps_s _sensorGps{};

    QMutex _rtcmMutex;
    QByteArray _rtcmBuffer;

    enum GPSReceiveType {
        Position = 1,
        Satellite = 2
    };

    static constexpr uint32_t kGPSReceiveTimeout = 1200;
    static constexpr uint32_t kReconnectDelayMs = 1000;
    static constexpr float kGPSHeadingOffset = 5.f;
};

Q_DECLARE_METATYPE(GPSProvider::SurveyInStatusData)
