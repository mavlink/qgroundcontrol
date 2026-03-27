#pragma once

#include <atomic>

#include <QtCore/QByteArray>
#include <QtCore/QLoggingCategory>
#include <QtCore/QMetaType>
#include <QtCore/QMutex>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QWaitCondition>

#include <gps_helper.h>
#include "GPSTypes.h"

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
        enum BaseMode : int {
            BaseSurveyIn = 0,
            BaseFixed    = 1,
        };

        double surveyInAccMeters = 0;
        int surveyInDurationSecs = 0;
        BaseMode useFixedBaseLocation = BaseSurveyIn;
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
        int32_t gnssSystems = 0;        // GPSHelper::GNSSSystemsMask bitmask, 0 = receiver defaults
        float headingOffset = 5.f;      // Dual-antenna heading offset (degrees)
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

    GPSProvider(GPSTransport *transport, GPSType type, const rtk_data_s &rtkData, QObject *parent = nullptr);
    ~GPSProvider();

    void wakeFromReconnectWait();
    void requestStop() { _requestStop.store(true, std::memory_order_relaxed); }

public slots:
    void process();
    void injectRTCMData(const QByteArray &data);

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

    std::unique_ptr<GPSHelper> _connectGPS();
    void _publishSensorGPS();
    void _publishSatelliteInfo();
    void _publishSensorGNSSRelative();

    void _gotRTCMData(const uint8_t *data, size_t len);
    void _drainRTCMBuffer();

    static int _callbackEntry(GPSCallbackType type, void *data1, int data2, void *user);

    GPSTransport *_transport = nullptr;
    GPSType _type;
    std::atomic_bool _requestStop{false};
    rtk_data_s _rtkData{};
    GPSHelper::GPSConfig _gpsConfig{};

    // Worker-thread-only: written in _callback(), emitted by-value via queued signals
    struct satellite_info_s _satelliteInfo{};
    struct sensor_gnss_relative_s _sensorGnssRelative{};
    struct sensor_gps_s _sensorGps{};

    QMutex _rtcmMutex;
    QByteArray _rtcmBuffer;

    QMutex _stopMutex;
    QWaitCondition _stopCondition;

    // Unscoped enum: used as bitmask with GPS library's int return from receive()
    enum GPSReceiveType : int {
        Position = 1,
        Satellite = 2
    };

    static constexpr uint32_t kReceiveTimeoutMs = 1200;
    // Time to wait between provider-level reconnect attempts. Named with the
    // same `...IntervalMs` suffix as SerialGPSTransport::kOpenRetryIntervalMs
    // so the retry vocabulary is consistent across the GPS driver.
    static constexpr uint32_t kReconnectIntervalMs = 1000;
};

Q_DECLARE_METATYPE(GPSProvider::SurveyInStatusData)
