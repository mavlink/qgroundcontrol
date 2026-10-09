#pragma once

#include <chrono>

#include <QtCore/QPointer>
#include <QtPositioning/QGeoPositionInfo>
#include <QtQmlIntegration/QtQmlIntegration>

#include "FactGroup.h"
#include "ScheduledTask.h"

class RuntimeScheduler;

/// One vehicle GNSS receiver: GPS_RAW_INT, GPS_RTK and GNSS_INTEGRITY id 0 for the primary, their GPS2 forms and id 1
/// for the secondary.
class VehicleGPSFactGroup : public FactGroup
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
    // Both enums have an Unknown value, so QML names them with their enum, such as AuthenticationState.Ok.
    Q_CLASSINFO("RegisterEnumClassesUnscoped", "false")
    Q_PROPERTY(Fact* lat READ lat CONSTANT)
    Q_PROPERTY(Fact* lon READ lon CONSTANT)
    Q_PROPERTY(Fact* mgrs READ mgrs CONSTANT)
    Q_PROPERTY(Fact* hdop READ hdop CONSTANT)
    Q_PROPERTY(Fact* vdop READ vdop CONSTANT)
    Q_PROPERTY(Fact* horizontalAccuracy READ horizontalAccuracy CONSTANT)
    Q_PROPERTY(Fact* verticalAccuracy READ verticalAccuracy CONSTANT)
    Q_PROPERTY(Fact* courseOverGround READ courseOverGround CONSTANT)
    Q_PROPERTY(Fact* yaw READ yaw CONSTANT)
    Q_PROPERTY(Fact* count READ count CONSTANT)
    Q_PROPERTY(Fact* lock READ lock CONSTANT)
    Q_PROPERTY(Fact* systemErrors READ systemErrors CONSTANT)
    Q_PROPERTY(Fact* spoofingState READ spoofingState CONSTANT)
    Q_PROPERTY(Fact* jammingState READ jammingState CONSTANT)
    Q_PROPERTY(Fact* authenticationState READ authenticationState CONSTANT)
    Q_PROPERTY(Fact* correctionsQuality READ correctionsQuality CONSTANT)
    Q_PROPERTY(Fact* systemQuality READ systemQuality CONSTANT)
    Q_PROPERTY(Fact* gnssSignalQuality READ gnssSignalQuality CONSTANT)
    Q_PROPERTY(Fact* postProcessingQuality READ postProcessingQuality CONSTANT)
    Q_PROPERTY(Fact* rtkBaseline READ rtkBaseline CONSTANT)
    Q_PROPERTY(Fact* rtkRate READ rtkRate CONSTANT)
    Q_PROPERTY(Fact* rtkSatellites READ rtkSatellites CONSTANT)
    Q_PROPERTY(QString systemErrorText READ systemErrorText NOTIFY systemErrorTextChanged)
    Q_PROPERTY(bool jammingReported READ jammingReported NOTIFY resilienceChanged)
    Q_PROPERTY(bool spoofingReported READ spoofingReported NOTIFY resilienceChanged)
    Q_PROPERTY(bool authenticationReported READ authenticationReported NOTIFY resilienceChanged)
    /// The worse of the reported spoofing and jamming states, or 0 when neither is reported.
    Q_PROPERTY(int interferenceState READ interferenceState NOTIFY resilienceChanged)
    /// Ranks the reported authentication state, from 0 when unreported through Disabled, Initializing and Ok to Error.
    Q_PROPERTY(int authenticationSeverity READ authenticationSeverity NOTIFY resilienceChanged)

public:
    enum class ReceiverIndex
    {
        Primary,
        Secondary,
    };

    /// MAVLink GPS_JAMMING_STATE and GPS_SPOOFING_STATE, which share their values.
    enum class InterferenceState
    {
        Unknown = 0,
        NotDetected = 1,
        Mitigated = 2,
        Detected = 3,
    };
    Q_ENUM(InterferenceState)

    /// MAVLink GPS_AUTHENTICATION_STATE.
    enum class AuthenticationState
    {
        Unknown = 0,
        Initializing = 1,
        Error = 2,
        Ok = 3,
        Disabled = 4,
    };
    Q_ENUM(AuthenticationState)

    /// The value of a state the vehicle has not reported.
    static constexpr int NOT_REPORTED = 255;

    explicit VehicleGPSFactGroup(QObject* parent = nullptr, RuntimeScheduler* scheduler = nullptr,
                                 ReceiverIndex receiver = ReceiverIndex::Primary);

    Fact* lat() { return &_latFact; }

    Fact* lon() { return &_lonFact; }

    Fact* mgrs() { return &_mgrsFact; }

    Fact* hdop() { return &_hdopFact; }

    Fact* vdop() { return &_vdopFact; }

    Fact* horizontalAccuracy() { return &_horizontalAccuracyFact; }

    Fact* verticalAccuracy() { return &_verticalAccuracyFact; }

    Fact* courseOverGround() { return &_courseOverGroundFact; }

    Fact* yaw() { return &_yawFact; }

    Fact* count() { return &_countFact; }

    Fact* lock() { return &_lockFact; }

    Fact* systemErrors() { return &_systemErrorsFact; }

    Fact* spoofingState() { return &_spoofingStateFact; }

    Fact* jammingState() { return &_jammingStateFact; }

    Fact* authenticationState() { return &_authenticationStateFact; }

    Fact* correctionsQuality() { return &_correctionsQualityFact; }

    Fact* systemQuality() { return &_systemQualityFact; }

    Fact* gnssSignalQuality() { return &_gnssSignalQualityFact; }

    Fact* postProcessingQuality() { return &_postProcessingQualityFact; }

    /// GPS_RTK (primary) or GPS2_RTK (secondary); cleared when the vehicle stops reporting them.
    Fact* rtkBaseline() { return &_rtkBaselineFact; }

    Fact* rtkRate() { return &_rtkRateFact; }

    Fact* rtkSatellites() { return &_rtkSatellitesFact; }

    /// The receiver's reported system errors, comma separated; empty when there are none.
    QString systemErrorText() const;

    bool jammingReported() const;
    bool spoofingReported() const;
    bool authenticationReported() const;
    int interferenceState() const;
    int authenticationSeverity() const;

    /// A state other than Unknown or NOT_REPORTED.
    [[nodiscard]] static constexpr bool reported(int state) { return state > 0 && state < NOT_REPORTED; }

    static constexpr std::chrono::seconds RTK_STATUS_TIMEOUT{5};
    /// The GNSS_INTEGRITY facts return to unreported when the vehicle stops sending it for this long.
    static constexpr std::chrono::seconds GNSS_INTEGRITY_TIMEOUT{5};

    // Overrides from FactGroup
    void handleMessage(Vehicle* vehicle, const mavlink_message_t& message) override;

signals:
    /// Emitted for every position report after the facts update. The altitude is above mean sea level and @a fixType
    /// is a MAVLink GPS_FIX_TYPE.
    void positionReported(const QGeoPositionInfo& position, int fixType);
    void systemErrorTextChanged();
    void resilienceChanged();

private:
    void _handleGpsRaw(const mavlink_message_t& message);
    void _handleHighLatency(const mavlink_message_t& message);
    void _handleHighLatency2(const mavlink_message_t& message);
    void _handleGnssIntegrity(const mavlink_message_t& message);
    void _handleGpsRtk(const mavlink_message_t& message);
    void _clearIntegrity();
    void _clearRtkStatus();
    void _updateFix(int fixType, int satellitesVisible, double hdopValue, double vdopValue, double courseValue,
                    double yawValue);
    /// Updates the position facts, then emits positionReported().
    void _reportPosition(QGeoPositionInfo position, int fixType);

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

    const ReceiverIndex _receiver;
    QPointer<RuntimeScheduler> _scheduler;
    ScheduledTask _rtkStatusExpiry;
    ScheduledTask _integrityExpiry;
};
