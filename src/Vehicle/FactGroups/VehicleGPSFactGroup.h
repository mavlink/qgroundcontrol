#pragma once

#include "FactGroup.h"

#include <QtQmlIntegration/QtQmlIntegration>

class VehicleGPSFactGroup : public FactGroup
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

public:
    static constexpr uint8_t kInvalidSatelliteCount = 255;

    enum class GPSQuality {
        QualityNone      = 0,
        QualityPoor      = 1,
        QualityFair      = 2,
        QualityGood      = 3,
        QualityExcellent = 4,
    };
    Q_ENUM(GPSQuality)

    enum class GPSFixType {
        FixNone     = 0,
        FixNoFix    = 1,
        Fix2D       = 2,
        Fix3D       = 3,
        FixDGPS     = 4,
        FixRTKFloat = 5,
        FixRTKFixed = 6,
        FixStatic   = 7
    };
    Q_ENUM(GPSFixType)

    enum class JammingState {
        JammingUnknown   = 0,
        JammingOk        = 1,
        JammingMitigated = 2,
        JammingDetected  = 3,
        JammingInvalid   = 255
    };
    Q_ENUM(JammingState)

    enum class SpoofingState {
        SpoofingUnknown   = 0,
        SpoofingOk        = 1,
        SpoofingMitigated = 2,
        SpoofingDetected  = 3,
        SpoofingInvalid   = 255
    };
    Q_ENUM(SpoofingState)

    enum class AuthenticationState {
        AuthUnknown      = 0,
        AuthInitializing = 1,
        AuthError        = 2,
        AuthOk           = 3,
        AuthDisabled     = 4,
        AuthInvalid      = 255
    };
    Q_ENUM(AuthenticationState)

    // GPS_SYSTEM_ERROR_FLAGS bitmask (from GPS_STATUS MAVLink message)
    enum GPSSystemError {
        ErrorIncomingCorrection = 0x01,
        ErrorConfiguration      = 0x02,
        ErrorSoftware           = 0x04,
        ErrorAntenna            = 0x08,
        ErrorEventCongestion    = 0x10,
        ErrorCpuOverload        = 0x20,
        ErrorOutputCongestion   = 0x40,
    };
    Q_ENUM(GPSSystemError)

    Q_PROPERTY(Fact *lat                    READ lat                    CONSTANT)
    Q_PROPERTY(Fact *lon                    READ lon                    CONSTANT)
    Q_PROPERTY(Fact *mgrs                   READ mgrs                   CONSTANT)
    Q_PROPERTY(Fact *altitudeMSL            READ altitudeMSL            CONSTANT)
    Q_PROPERTY(Fact *altitudeEllipsoid      READ altitudeEllipsoid      CONSTANT)
    Q_PROPERTY(Fact *hdop                   READ hdop                   CONSTANT)
    Q_PROPERTY(Fact *vdop                   READ vdop                   CONSTANT)
    Q_PROPERTY(Fact *hAcc                   READ hAcc                   CONSTANT)
    Q_PROPERTY(Fact *vAcc                   READ vAcc                   CONSTANT)
    Q_PROPERTY(Fact *groundSpeed            READ groundSpeed            CONSTANT)
    Q_PROPERTY(Fact *velAcc                 READ velAcc                 CONSTANT)
    Q_PROPERTY(Fact *hdgAcc                 READ hdgAcc                 CONSTANT)
    Q_PROPERTY(Fact *courseOverGround       READ courseOverGround       CONSTANT)
    Q_PROPERTY(Fact *yaw                    READ yaw                    CONSTANT)
    Q_PROPERTY(Fact *count                  READ count                  CONSTANT)
    Q_PROPERTY(Fact *lock                   READ lock                   CONSTANT)
    Q_PROPERTY(Fact* systemErrors           READ systemErrors           CONSTANT)
    Q_PROPERTY(Fact* spoofingState          READ spoofingState          CONSTANT)
    Q_PROPERTY(Fact* jammingState           READ jammingState           CONSTANT)
    Q_PROPERTY(Fact* authenticationState    READ authenticationState    CONSTANT)
    Q_PROPERTY(Fact* correctionsQuality     READ correctionsQuality     CONSTANT)
    Q_PROPERTY(Fact* systemQuality          READ systemQuality          CONSTANT)
    Q_PROPERTY(Fact* gnssSignalQuality      READ gnssSignalQuality      CONSTANT)
    Q_PROPERTY(Fact* postProcessingQuality  READ postProcessingQuality  CONSTANT)
    // Vehicle-reported RTK status (from GPS_RTK MAVLink message).
    // Note: rtkBaseline is the vehicle-side baseline length — distinct from
    // GPSRTKFactGroup::baselineLength which is the local base station baseline.
    Q_PROPERTY(Fact* rtkHealth              READ rtkHealth              CONSTANT)
    Q_PROPERTY(Fact* rtkRate                READ rtkRate                CONSTANT)
    Q_PROPERTY(Fact* rtkNumSats             READ rtkNumSats             CONSTANT)
    Q_PROPERTY(Fact* rtkBaseline            READ rtkBaseline            CONSTANT)
    Q_PROPERTY(Fact* rtkAccuracy            READ rtkAccuracy            CONSTANT)
    Q_PROPERTY(Fact* rtkIAR                 READ rtkIAR                 CONSTANT)
    Q_PROPERTY(GPSQuality quality           READ quality                NOTIFY qualityChanged)

public:
    explicit VehicleGPSFactGroup(QObject *parent = nullptr);

    Fact *lat() { return &_latFact; }
    Fact *lon() { return &_lonFact; }
    Fact *mgrs() { return &_mgrsFact; }
    Fact *altitudeMSL() { return &_altitudeMSLFact; }
    Fact *altitudeEllipsoid() { return &_altitudeEllipsoidFact; }
    Fact *hdop() { return &_hdopFact; }
    Fact *vdop() { return &_vdopFact; }
    Fact *hAcc() { return &_hAccFact; }
    Fact *vAcc() { return &_vAccFact; }
    Fact *groundSpeed() { return &_groundSpeedFact; }
    Fact *velAcc() { return &_velAccFact; }
    Fact *hdgAcc() { return &_hdgAccFact; }
    Fact *courseOverGround() { return &_courseOverGroundFact; }
    Fact *yaw() { return &_yawFact; }
    Fact *count() { return &_countFact; }
    Fact *lock() { return &_lockFact; }
    Fact *systemErrors() { return &_systemErrorsFact; }
    Fact *spoofingState() { return &_spoofingStateFact; }
    Fact *jammingState() { return &_jammingStateFact; }
    Fact *authenticationState() { return &_authenticationStateFact; }
    Fact *correctionsQuality() { return &_correctionsQualityFact; }
    Fact *systemQuality() { return &_systemQualityFact; }
    Fact *gnssSignalQuality() { return &_gnssSignalQualityFact; }
    Fact *postProcessingQuality() { return &_postProcessingQualityFact; }
    Fact *rtkHealth() { return &_rtkHealthFact; }
    Fact *rtkRate() { return &_rtkRateFact; }
    Fact *rtkNumSats() { return &_rtkNumSatsFact; }
    Fact *rtkBaseline() { return &_rtkBaselineFact; }
    Fact *rtkAccuracy() { return &_rtkAccuracyFact; }
    Fact *rtkIAR() { return &_rtkIARFact; }
    GPSQuality quality() const;

    // Overrides from FactGroup
    void handleMessage(Vehicle *vehicle, const mavlink_message_t &message) override;

signals:
    void gnssIntegrityReceived();
    void qualityChanged();

protected:
    void _handleGpsRawInt(const mavlink_message_t &message);
    void _handleHighLatency(const mavlink_message_t &message);
    void _handleHighLatency2(const mavlink_message_t &message);
    void _handleGnssIntegrity(const mavlink_message_t& message);
    void _handleGpsRtk(const mavlink_message_t& message);

    // Shared MAVLink GPS field parsing — used by GPS_RAW_INT and GPS2_RAW handlers
    template<typename T>
    void _applyGpsRawFields(const T &raw);

    Fact _latFact = Fact(0, QStringLiteral("lat"), FactMetaData::valueTypeDouble);
    Fact _lonFact = Fact(0, QStringLiteral("lon"), FactMetaData::valueTypeDouble);
    Fact _mgrsFact = Fact(0, QStringLiteral("mgrs"), FactMetaData::valueTypeString);
    Fact _altitudeMSLFact = Fact(0, QStringLiteral("altitudeMSL"), FactMetaData::valueTypeDouble);
    Fact _altitudeEllipsoidFact = Fact(0, QStringLiteral("altitudeEllipsoid"), FactMetaData::valueTypeDouble);
    Fact _hdopFact = Fact(0, QStringLiteral("hdop"), FactMetaData::valueTypeDouble);
    Fact _vdopFact = Fact(0, QStringLiteral("vdop"), FactMetaData::valueTypeDouble);
    Fact _hAccFact = Fact(0, QStringLiteral("hAcc"), FactMetaData::valueTypeDouble);
    Fact _vAccFact = Fact(0, QStringLiteral("vAcc"), FactMetaData::valueTypeDouble);
    Fact _groundSpeedFact = Fact(0, QStringLiteral("groundSpeed"), FactMetaData::valueTypeDouble);
    Fact _velAccFact = Fact(0, QStringLiteral("velAcc"), FactMetaData::valueTypeDouble);
    Fact _hdgAccFact = Fact(0, QStringLiteral("hdgAcc"), FactMetaData::valueTypeDouble);
    Fact _courseOverGroundFact = Fact(0, QStringLiteral("courseOverGround"), FactMetaData::valueTypeDouble);
    Fact _yawFact = Fact(0, QStringLiteral("yaw"), FactMetaData::valueTypeDouble);
    Fact _countFact = Fact(0, QStringLiteral("count"), FactMetaData::valueTypeInt32);
    Fact _lockFact = Fact(0, QStringLiteral("lock"), FactMetaData::valueTypeInt32);
    Fact _systemErrorsFact = Fact(0, QStringLiteral("systemErrors"), FactMetaData::valueTypeUint32);
    Fact _spoofingStateFact = Fact(0, QStringLiteral("spoofingState"), FactMetaData::valueTypeUint8);
    Fact _jammingStateFact = Fact(0, QStringLiteral("jammingState"), FactMetaData::valueTypeUint8);
    Fact _authenticationStateFact = Fact(0, QStringLiteral("authenticationState"), FactMetaData::valueTypeUint8);
    Fact _correctionsQualityFact = Fact(0, QStringLiteral("correctionsQuality"), FactMetaData::valueTypeUint8);
    Fact _systemQualityFact = Fact(0, QStringLiteral("systemQuality"), FactMetaData::valueTypeUint8);
    Fact _gnssSignalQualityFact = Fact(0, QStringLiteral("gnssSignalQuality"), FactMetaData::valueTypeUint8);
    Fact _postProcessingQualityFact = Fact(0, QStringLiteral("postProcessingQuality"), FactMetaData::valueTypeUint8);
    Fact _rtkHealthFact = Fact(0, QStringLiteral("rtkHealth"), FactMetaData::valueTypeUint8);
    Fact _rtkRateFact = Fact(0, QStringLiteral("rtkRate"), FactMetaData::valueTypeUint8);
    Fact _rtkNumSatsFact = Fact(0, QStringLiteral("rtkNumSats"), FactMetaData::valueTypeUint8);
    Fact _rtkBaselineFact = Fact(0, QStringLiteral("rtkBaseline"), FactMetaData::valueTypeDouble);
    Fact _rtkAccuracyFact = Fact(0, QStringLiteral("rtkAccuracy"), FactMetaData::valueTypeDouble);
    Fact _rtkIARFact = Fact(0, QStringLiteral("rtkIAR"), FactMetaData::valueTypeInt32);

    uint8_t _gnssIntegrityId {};
    uint8_t _rtkReceiverId {};
};
