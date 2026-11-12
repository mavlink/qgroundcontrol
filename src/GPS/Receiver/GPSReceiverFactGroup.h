#pragma once

#include <initializer_list>
#include <optional>

#include <QtCore/QList>
#include <QtCore/QString>
#include <QtQmlIntegration/QtQmlIntegration>

#include "FactGroup.h"
#include "GPSReceiverConfig.h"
#include "GPSReceiverReports.h"

/// The satellites of one constellation in view, as the receiver status lists them.
struct GPSConstellationSatellites
{
    Q_GADGET
    QML_VALUE_TYPE(gpsConstellationSatellites)
    Q_PROPERTY(QString name READ name FINAL)
    Q_PROPERTY(int used MEMBER used FINAL)
    Q_PROPERTY(int inView MEMBER inView FINAL)

public:
    /// The constellation's translated name; "Other" for satellites of a constellation the receiver does not identify.
    QString name() const;

    GPSConstellation constellation = GPSConstellation::Unknown;
    /// -1 when the receiver does not report which satellites it uses.
    int used = -1;
    int inView = 0;

    bool operator==(const GPSConstellationSatellites&) const = default;
};

/// The local receiver's status as Facts, which its GPSReceiver writes as the receiver reports; a value anything else
/// writes does not notify the receiver's labels. Raw values notify when written; the values QML shows notify at most
/// once a second. telemetryAvailable follows whether the receiver finished configuration.
class GPSReceiverFactGroup : public FactGroup
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Provided by GPSReceiver")
    Q_PROPERTY(Fact* currentDuration READ currentDuration CONSTANT FINAL)
    Q_PROPERTY(Fact* currentAccuracy READ currentAccuracy CONSTANT FINAL)
    Q_PROPERTY(Fact* targetDuration READ targetDuration CONSTANT FINAL)
    Q_PROPERTY(Fact* targetAccuracy READ targetAccuracy CONSTANT FINAL)
    Q_PROPERTY(Fact* baseState READ baseState CONSTANT FINAL)
    Q_PROPERTY(Fact* baseLatitude READ baseLatitude CONSTANT FINAL)
    Q_PROPERTY(Fact* baseLongitude READ baseLongitude CONSTANT FINAL)
    Q_PROPERTY(Fact* baseHeight READ baseHeight CONSTANT FINAL)
    Q_PROPERTY(Fact* numSatellites READ numSatellites CONSTANT FINAL)
    Q_PROPERTY(Fact* numSatellitesUsed READ numSatellitesUsed CONSTANT FINAL)
    Q_PROPERTY(Fact* fixType READ fixType CONSTANT FINAL)
    Q_PROPERTY(Fact* jammingState READ jammingState CONSTANT FINAL)
    Q_PROPERTY(Fact* spoofingState READ spoofingState CONSTANT FINAL)
    Q_PROPERTY(Fact* antennaState READ antennaState CONSTANT FINAL)
    Q_PROPERTY(bool canSaveCurrentBasePosition READ canSaveCurrentBasePosition NOTIFY currentBasePositionChanged FINAL)
    /// The current base position was saved to the settings since the survey that produced it started.
    Q_PROPERTY(bool currentBasePositionSaved READ currentBasePositionSaved NOTIFY currentBasePositionChanged FINAL)
    Q_PROPERTY(bool receiverWarning READ receiverWarning NOTIFY receiverWarningChanged FINAL)

    friend class GPSReceiver;
    friend class GPSManager;

public:
    /// The values of the baseState Fact: GPSBaseState, which QGCGPSCore declares without QML.
    enum class BaseState
    {
        Inactive = static_cast<int>(GPSBaseState::Inactive),
        Active = static_cast<int>(GPSBaseState::Active),
        Valid = static_cast<int>(GPSBaseState::Valid),
    };
    Q_ENUM(BaseState)

    Fact* currentDuration() { return &_currentDurationFact; }

    Fact* currentAccuracy() { return &_currentAccuracyFact; }

    /// The time and the accuracy the connected base was asked to survey its position for, which each receiver family
    /// applies in its own way or not at all. Zero and unavailable for a base that does not survey, for a passive
    /// receiver and without a receiver.
    Fact* targetDuration() { return &_targetDurationFact; }

    Fact* targetAccuracy() { return &_targetAccuracyFact; }

    /// What a configured base reports of its base position, in every base mode.
    Fact* baseState() { return &_baseStateFact; }

    /// The base position as the receiver reports it, valid or not; NaN while it reports none.
    Fact* baseLatitude() { return &_baseLatitudeFact; }

    Fact* baseLongitude() { return &_baseLongitudeFact; }

    /// Above the WGS84 ellipsoid.
    Fact* baseHeight() { return &_baseHeightFact; }

    Fact* numSatellites() { return &_numSatellitesFact; }

    Fact* numSatellitesUsed() { return &_numSatellitesUsedFact; }

    Fact* fixType() { return &_fixTypeFact; }

    Fact* jammingState() { return &_jammingStateFact; }

    Fact* spoofingState() { return &_spoofingStateFact; }

    Fact* antennaState() { return &_antennaStateFact; }

    /// The receiver's valid base position with a known accuracy, which a fixed-position base can start from; empty
    /// otherwise. A valid receiver status alone does not establish usable coordinates or accuracy.
    std::optional<GPSBaseStationConfig::Fixed> currentBasePosition() const { return _currentBasePosition; }

    bool canSaveCurrentBasePosition() const { return _currentBasePosition.has_value(); }

    bool currentBasePositionSaved() const { return _currentBasePositionSaved; }

    /// Jamming at Warning or Critical, any spoofing indication, or an open or shorted antenna.
    bool receiverWarning() const { return _receiverWarning; }

    /// The constellations with satellites in view, in GPSConstellation order, which GPSReceiver notifies of. Empty
    /// when the receiver identifies none of them, as the list would only repeat the satellite totals.
    QList<GPSConstellationSatellites> constellations() const { return _constellations; }

signals:
    void currentBasePositionChanged();
    void receiverWarningChanged();

private:
    /// Starts with no receiver: every Fact at its metadata default.
    explicit GPSReceiverFactGroup(QObject* parent);

    // Written by GPSReceiver.
    /// The survey a base is connected for; none for a base in another mode and for a passive receiver.
    void _setSurveyTarget(const std::optional<GPSBaseStationConfig::SurveyIn>& survey);
    void _setBaseStatus(const GPSBaseStatus& report);
    void _setSatellites(const GPSSatelliteReport& report);
    void _setSolution(GPSFixQuality fixType, const GPSIntegrityReport& integrity);
    /// Clears the fix, satellites, and integrity of a receiver that stopped reporting position.
    void _clearSolution();
    /// Restores the status of no receiver.
    void _reset();

    void _setConfigured(bool configured) { _setTelemetryAvailable(configured); }

    static void _update(Fact& fact, const QVariant& value);
    /// Writes a value of a report into the status it was reported for: the one whose writer read _resets as
    /// @a status. The Facts notify as they are written, and an observer of one may end the session, which resets
    /// the status: what the report holds after that is the ended session's and is not written.
    void _write(quint64 status, Fact& fact, const QVariant& value);
    static void _restoreDefaults(std::initializer_list<Fact*> facts);
    void _setCurrentBasePosition(const std::optional<GPSBaseStationConfig::Fixed>& base);
    void _setCurrentBasePositionSaved(bool saved);
    void _updateReceiverWarning();

    std::optional<GPSBaseStationConfig::Fixed> _currentBasePosition;
    bool _currentBasePositionSaved = false;
    bool _receiverWarning = false;
    QList<GPSConstellationSatellites> _constellations;
    /// How often the status was reset, which tells the writer of a report that the status it began with is gone.
    quint64 _resets = 0;

    Fact _currentDurationFact = Fact(0, QStringLiteral("currentDuration"), FactMetaData::valueTypeDouble);
    Fact _currentAccuracyFact = Fact(0, QStringLiteral("currentAccuracy"), FactMetaData::valueTypeDouble);
    Fact _targetDurationFact = Fact(0, QStringLiteral("targetDuration"), FactMetaData::valueTypeDouble);
    Fact _targetAccuracyFact = Fact(0, QStringLiteral("targetAccuracy"), FactMetaData::valueTypeDouble);
    Fact _baseStateFact = Fact(0, QStringLiteral("baseState"), FactMetaData::valueTypeUint8);
    Fact _baseLatitudeFact = Fact(0, QStringLiteral("baseLatitude"), FactMetaData::valueTypeDouble);
    Fact _baseLongitudeFact = Fact(0, QStringLiteral("baseLongitude"), FactMetaData::valueTypeDouble);
    Fact _baseHeightFact = Fact(0, QStringLiteral("baseHeight"), FactMetaData::valueTypeDouble);
    Fact _numSatellitesFact = Fact(0, QStringLiteral("numSatellites"), FactMetaData::valueTypeInt32);
    Fact _numSatellitesUsedFact = Fact(0, QStringLiteral("numSatellitesUsed"), FactMetaData::valueTypeInt32);
    Fact _fixTypeFact = Fact(0, QStringLiteral("fixType"), FactMetaData::valueTypeUint32);
    Fact _jammingStateFact = Fact(0, QStringLiteral("jammingState"), FactMetaData::valueTypeUint8);
    Fact _spoofingStateFact = Fact(0, QStringLiteral("spoofingState"), FactMetaData::valueTypeUint8);
    Fact _antennaStateFact = Fact(0, QStringLiteral("antennaState"), FactMetaData::valueTypeUint8);
};
