#pragma once

#include <chrono>

#include <QtCore/QPointer>

#include "FactGroup.h"
#include "GPSObservation.h"
#include "ScheduledTask.h"

class GPSSourceHealth;
class RuntimeScheduler;

class VehicleGPSFactGroup : public FactGroup
{
    Q_OBJECT
    Q_PROPERTY(Fact *lat                    READ lat                    CONSTANT)
    Q_PROPERTY(Fact *lon                    READ lon                    CONSTANT)
    Q_PROPERTY(Fact *mgrs                   READ mgrs                   CONSTANT)
    Q_PROPERTY(Fact *hdop                   READ hdop                   CONSTANT)
    Q_PROPERTY(Fact *vdop                   READ vdop                   CONSTANT)
    Q_PROPERTY(Fact* horizontalAccuracy READ horizontalAccuracy CONSTANT)
    Q_PROPERTY(Fact* verticalAccuracy READ verticalAccuracy CONSTANT)
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
    Q_PROPERTY(Fact* rtkBaseline READ rtkBaseline CONSTANT)
    Q_PROPERTY(Fact* rtkRate READ rtkRate CONSTANT)
    Q_PROPERTY(Fact* rtkSatellites READ rtkSatellites CONSTANT)

public:
    enum class ReceiverIndex
    {
        Primary,
        Secondary,
    };

    explicit VehicleGPSFactGroup(QObject* parent = nullptr, RuntimeScheduler* scheduler = nullptr,
                                 ReceiverIndex receiver = ReceiverIndex::Primary);

    Fact *lat() { return &_latFact; }
    Fact *lon() { return &_lonFact; }
    Fact *mgrs() { return &_mgrsFact; }
    Fact *hdop() { return &_hdopFact; }
    Fact *vdop() { return &_vdopFact; }
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

    Fact* horizontalAccuracy() { return &_horizontalAccuracyFact; }

    Fact* verticalAccuracy() { return &_verticalAccuracyFact; }

    /// GPS_RTK (primary) or GPS2_RTK (secondary); cleared when the vehicle stops reporting them.
    Fact* rtkBaseline() { return &_rtkBaselineFact; }

    Fact* rtkRate() { return &_rtkRateFact; }

    Fact* rtkSatellites() { return &_rtkSatellitesFact; }

    static constexpr std::chrono::seconds RTK_STATUS_TIMEOUT{5};

    /// Receipt time in the scheduler's monotonic clock domain; zero until an integrity report arrives.
    quint64 gnssIntegrityTimestampUs() const { return _gnssIntegrityTimestampUs; }

    std::optional<GPSObservation> acceptedObservation() const;

    // Overrides from FactGroup
    void handleMessage(Vehicle *vehicle, const mavlink_message_t &message) override;

signals:
    void gnssIntegrityReceived();

private:
    void _handleGpsRaw(const mavlink_message_t& message);
    void _handleHighLatency(const mavlink_message_t &message);
    void _handleHighLatency2(const mavlink_message_t &message);
    void _handleGnssIntegrity(const mavlink_message_t& message);
    void _handleGpsRtk(const mavlink_message_t& message);
    void _clearRtkStatus();

    Fact _latFact = Fact(0, QStringLiteral("lat"), FactMetaData::valueTypeDouble);
    Fact _lonFact = Fact(0, QStringLiteral("lon"), FactMetaData::valueTypeDouble);
    Fact _mgrsFact = Fact(0, QStringLiteral("mgrs"), FactMetaData::valueTypeString);
    Fact _hdopFact = Fact(0, QStringLiteral("hdop"), FactMetaData::valueTypeDouble);
    Fact _vdopFact = Fact(0, QStringLiteral("vdop"), FactMetaData::valueTypeDouble);
    Fact _horizontalAccuracyFact = Fact(0, QStringLiteral("horizontalAccuracy"), FactMetaData::valueTypeDouble);
    Fact _verticalAccuracyFact = Fact(0, QStringLiteral("verticalAccuracy"), FactMetaData::valueTypeDouble);
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
    Fact _rtkBaselineFact = Fact(0, QStringLiteral("rtkBaseline"), FactMetaData::valueTypeDouble);
    Fact _rtkRateFact = Fact(0, QStringLiteral("rtkRate"), FactMetaData::valueTypeDouble);
    Fact _rtkSatellitesFact = Fact(0, QStringLiteral("rtkSatellites"), FactMetaData::valueTypeInt32);

    void _updateGpsObservation(GPSObservation observation, int fixType, int satellitesVisible, double yaw = qQNaN());

    const ReceiverIndex _receiver;
    RuntimeScheduler* const _scheduler;
    GPSSourceHealth* _positionHealth = nullptr;
    ScheduledTask _rtkStatusExpiry;
    quint64 _gnssIntegrityTimestampUs = 0;
};
