#include "GPSSettingsBindings.h"

#include <chrono>

#include <QtCore/QMetaEnum>
#include <QtCore/QSet>

#include "Fact.h"
#include "GPSCorrectionManager.h"
#include "GPSCorrectionSettings.h"
#include "GPSReceiver.h"
#include "NTRIPManager.h"
#include "NTRIPSettings.h"
#include "PositionManager.h"
#include "PositionReceiverSettings.h"
#include "QGCLoggingCategory.h"
#include "RTKSettings.h"

QGC_LOGGING_CATEGORY(GPSSettingsBindingsLog, "GPS.GPSSettingsBindings")

namespace {

using CorrectionsConfig = GPSCorrectionManager::Configuration;
using NTRIPConfig = NTRIPManager::Configuration;
using ReceiverConfig = GPSReceiver::Configuration;
using PositionConfig = PositionManager::Configuration;

/// Reads setting values and records the Facts read, which the binding then watches.
class FactReader
{
public:
    QVariant operator()(Fact* fact)
    {
        _facts.append(fact);
        return fact->rawValue();
    }

    const QList<Fact*>& facts() const { return _facts; }

private:
    QList<Fact*> _facts;
};

CorrectionsConfig correctionsFromSettings(GPSCorrectionSettings* settings, FactReader& read)
{
    CorrectionsConfig configuration;
    const int source = read(settings->correctionSource()).toInt();
    if (source >= GPSCorrectionSettings::HighestPriority && source < GPS_CORRECTION_SOURCE_COUNT) {
        configuration.source = static_cast<GPSCorrectionSettings::CorrectionSource>(source);
    } else {
        qCWarning(GPSSettingsBindingsLog) << "Invalid correction source; using automatic selection";
    }
    if (read(settings->correctionVehicles()).toInt() == GPSCorrectionSettings::ActiveVehicle) {
        configuration.vehicles = GPSCorrectionSettings::ActiveVehicle;
    }
    configuration.udpInput.enabled = read(settings->rtcmUdpInputEnabled()).toBool();
    configuration.udpInput.port = read(settings->rtcmUdpInputPort()).value<quint16>();
    configuration.udpOutput.enabled = read(settings->rtcmUdpOutputEnabled()).toBool();
    configuration.udpOutput.address = read(settings->rtcmUdpOutputAddress()).toString().trimmed();
    configuration.udpOutput.port = read(settings->rtcmUdpOutputPort()).value<quint16>();
    configuration.messageFilter = read(settings->ntripWhitelist()).toString();
    return configuration;
}

NTRIPConfig ntripFromSettings(NTRIPSettings* settings, FactReader& read)
{
    NTRIPConfig configuration;
    configuration.enabled = read(settings->ntripServerConnectEnabled()).toBool();
    NTRIPConnectionConfig& connection = configuration.connection;
    connection.host = read(settings->ntripServerHostAddress()).toString();
    connection.port = read(settings->ntripServerPort()).toInt();
    connection.username = read(settings->ntripUsername()).toString();
    connection.password = read(settings->ntripPassword()).toString();
    connection.mountpoint = read(settings->ntripMountpoint()).toString();
    connection.useTls = read(settings->ntripUseTls()).toBool();
    connection.allowSelfSignedCerts = read(settings->ntripAllowSelfSignedCerts()).toBool();
    connection.pinnedCertificate = read(settings->ntripPinnedCertificate()).toString();
    const int ggaSource = read(settings->ntripGgaPositionSource()).toInt();
    if (QMetaEnum::fromType<NTRIPGgaReporter::PositionSource>().valueToKey(ggaSource)) {
        configuration.gga.source = static_cast<NTRIPGgaReporter::PositionSource>(ggaSource);
    } else {
        qCWarning(GPSSettingsBindingsLog) << "Invalid GGA position source; using automatic selection";
    }
    configuration.gga.interval = std::chrono::seconds(read(settings->ntripGgaIntervalSec()).toUInt());
    configuration.version1Casters = read(settings->ntripVersion1Casters()).toString();
    configuration.failover = read(settings->ntripFailoverEnabled()).toBool();
    // Read to be watched: the settings have parsed the stored list by the time a change reaches this binding.
    (void) read(settings->ntripSavedCasters());
    for (const NTRIPSavedCaster& saved : settings->savedCasters()) {
        NTRIPConnectionConfig caster;
        caster.host = saved.host;
        caster.port = saved.port;
        caster.username = saved.username;
        caster.password = saved.password;
        caster.mountpoint = saved.mountpoint;
        caster.useTls = saved.useTls;
        caster.allowSelfSignedCerts = saved.allowSelfSignedCerts;
        caster.pinnedCertificate = saved.pinnedCertificate;
        configuration.savedCasters.append({saved.name, caster});
    }
    configuration.requireVehicle = read(settings->ntripRequireVehicle()).toBool();
    configuration.dataCapMB = read(settings->ntripDataCapMB()).toUInt();
    return configuration;
}

/// Every base setting is read, whichever mode is selected, so each is watched.
GPSBaseStationConfig baseStationFromSettings(RTKSettings* settings, FactReader& read)
{
    using Mode = BaseModeDefinition::Mode;
    Mode mode = Mode::BaseSurveyIn;
    const int persistedMode = read(settings->baseMode()).toInt();
    if (persistedMode >= static_cast<int>(Mode::BaseSurveyIn) &&
        persistedMode <= static_cast<int>(Mode::BaseReceiverAveraging)) {
        mode = static_cast<Mode>(persistedMode);
    } else {
        qCWarning(GPSSettingsBindingsLog) << "Invalid base mode; using survey-in";
    }
    // A receiver is given where its antenna is, which is the antenna height above the saved position: a height is
    // measured along the ellipsoid's normal, so it adds to the ellipsoid height and leaves latitude and longitude.
    const GPSBaseStationConfig::Fixed fixed{
        .position = {.latitudeDegrees = read(settings->fixedBasePositionLatitude()).toDouble(),
                     .longitudeDegrees = read(settings->fixedBasePositionLongitude()).toDouble(),
                     .altitudeMeters = read(settings->fixedBasePositionAltitude()).toDouble() +
                                       read(settings->fixedBaseAntennaHeight()).toDouble()},
        .accuracyMeters = read(settings->fixedBasePositionAccuracy()).toFloat()};
    const GPSBaseStationConfig::SurveyIn surveyIn{
        .accuracyMeters = read(settings->surveyInAccuracyLimit()).toDouble(),
        .duration = std::chrono::seconds(read(settings->surveyInMinObservationDuration()).value<int64_t>())};
    const GPSBaseStationConfig::ReceiverAveraging averaging{
        .maximumDuration = std::chrono::seconds(read(settings->receiverAveragingDuration()).value<uint32_t>())};

    GPSBaseStationConfig base{.compactObservations = read(settings->compactRtcmCorrections()).toBool()};
    switch (mode) {
        case Mode::BaseFixed:
            base.mode = fixed;
            break;
        case Mode::BaseReceiverAveraging:
            base.mode = averaging;
            break;
        case Mode::BaseSurveyIn:
            base.mode = surveyIn;
            break;
    }
    return base;
}

ReceiverConfig receiverFromSettings(RTKSettings* settings, FactReader& read)
{
    ReceiverConfig configuration;
    configuration.receiverRole = static_cast<RTKSettings::ReceiverRole>(read(settings->receiverRole()).toInt());
    configuration.forwardReceiverRtcm = read(settings->forwardReceiverRtcm()).toBool();
    configuration.baseReceiverManufacturer = read(settings->baseReceiverManufacturer()).toInt();
    configuration.connectionType = static_cast<RTKSettings::ConnectionType>(read(settings->connectionType()).toInt());
    configuration.tcpHost = read(settings->tcpHost()).toString();
    configuration.tcpPort = read(settings->tcpPort()).toUInt();
    configuration.udpPort = read(settings->udpPort()).toUInt();
    configuration.serialDevice = read(settings->serialDevice()).toString();
    configuration.serialBaudRate = read(settings->serialBaudRate()).toUInt();
    configuration.base = baseStationFromSettings(settings, read);
    configuration.autoConnect = read(settings->autoConnect()).toBool();
    return configuration;
}

/// The position receiver has a connection and nothing else: it is never configured, and what RTCM it sends is not a
/// correction source.
ReceiverConfig positionReceiverFromSettings(PositionReceiverSettings* settings, FactReader& read)
{
    ReceiverConfig configuration;
    configuration.receiverRole = RTKSettings::Passive;
    configuration.forwardReceiverRtcm = false;
    configuration.connectionType = static_cast<RTKSettings::ConnectionType>(read(settings->connectionType()).toInt());
    configuration.tcpHost = read(settings->tcpHost()).toString();
    configuration.tcpPort = read(settings->tcpPort()).toUInt();
    configuration.udpPort = read(settings->udpPort()).toUInt();
    configuration.serialDevice = read(settings->serialDevice()).toString();
    configuration.serialBaudRate = read(settings->serialBaudRate()).toUInt();
    // In use, it is kept connected as a saved receiver is.
    configuration.autoConnect = read(settings->enabled()).toBool();
    return configuration;
}

PositionManager::SourceMode sourceModeFromSettings(RTKSettings* settings, FactReader& read)
{
    return static_cast<PositionManager::SourceMode>(read(settings->gcsPositionSource()).toInt());
}

/// The groups the ground station position is configured in.
struct PositionSettings
{
    RTKSettings* source = nullptr;
    PositionReceiverSettings* positionReceiver = nullptr;
};

/// A position receiver in use is the receiver source, connected or not.
PositionConfig positionFromSettings(PositionSettings settings, FactReader& read)
{
    return {.sourceMode = sourceModeFromSettings(settings.source, read),
            .receiver = read(settings.positionReceiver->enabled()).toBool() ? PositionManager::Receiver::Position
                                                                            : PositionManager::Receiver::Rtk};
}

/// The log names whether these are set, never their values: credentials and the base station's location.
bool redacted(const Fact* fact)
{
    static const QSet<QString> names{
        QString::fromLatin1(NTRIPSettings::ntripPasswordName),
        // Each saved caster is stored with its password.
        QString::fromLatin1(NTRIPSettings::ntripSavedCastersName),
        QString::fromLatin1(RTKSettings::fixedBasePositionLatitudeName),
        QString::fromLatin1(RTKSettings::fixedBasePositionLongitudeName),
        QString::fromLatin1(RTKSettings::fixedBasePositionAltitudeName),
    };
    return names.contains(fact->name());
}

void logSetting(const Fact* fact)
{
    const QString value = fact->rawValue().toString();
    const QString shown =
        !redacted(fact) ? value : (value.isEmpty() ? QStringLiteral("<empty>") : QStringLiteral("<set>"));
    qCDebug(GPSSettingsBindingsLog).noquote() << fact->name() << "=" << shown;
}

/// @a settings is a settings group, or the groups a configuration is built from.
template <typename Settings, typename Configuration>
QList<Fact*> readFacts(Settings settings, Configuration (*build)(Settings, FactReader&))
{
    FactReader reader;
    (void) build(settings, reader);
    return reader.facts();
}

/// Applies the built configuration to @a target now and whenever a Fact it reads changes, logging each value.
template <typename Settings, typename Configuration, typename Target>
void bind(Settings settings, Configuration (*build)(Settings, FactReader&), Target* target,
          void (Target::*setter)(const Configuration&))
{
    const auto apply = [settings, build, target, setter]() {
        FactReader reader;
        (target->*setter)(build(settings, reader));
        return reader.facts();
    };
    for (Fact* const fact : apply()) {
        logSetting(fact);
        QObject::connect(fact, &Fact::rawValueChanged, target, [apply, fact]() {
            logSetting(fact);
            (void) apply();
        });
    }
}

}  // namespace

QList<Fact*> GPSSettingsBindings::_boundFacts(GPSCorrectionSettings* settings)
{
    return readFacts(settings, correctionsFromSettings);
}

QList<Fact*> GPSSettingsBindings::_boundFacts(NTRIPSettings* settings)
{
    return readFacts(settings, ntripFromSettings);
}

QList<Fact*> GPSSettingsBindings::_boundFacts(PositionReceiverSettings* settings)
{
    return readFacts(settings, positionReceiverFromSettings);
}

QList<Fact*> GPSSettingsBindings::_boundFacts(RTKSettings* settings)
{
    return readFacts(settings, receiverFromSettings) + readFacts(settings, sourceModeFromSettings);
}

void GPSSettingsBindings::bindCorrections(GPSCorrectionSettings* settings, GPSCorrectionManager* corrections)
{
    if (!settings || !corrections) {
        return;
    }
    bind(settings, correctionsFromSettings, corrections, &GPSCorrectionManager::setConfiguration);
}

void GPSSettingsBindings::bindNtrip(NTRIPSettings* settings, NTRIPManager* ntrip)
{
    if (!settings || !ntrip) {
        return;
    }
    // Queued: the manager emits these inside its own operation (connecting, trusting a certificate), and storing the
    // Fact applies a new configuration to it, which must wait until that returns.
    QObject::connect(
        ntrip, &NTRIPManager::enabledRequested, ntrip,
        [settings](bool enabled) { settings->ntripServerConnectEnabled()->setRawValue(enabled); },
        Qt::QueuedConnection);
    QObject::connect(
        ntrip, &NTRIPManager::certificatePinChanged, ntrip,
        [settings](const QString& pin) { settings->ntripPinnedCertificate()->setRawValue(pin); }, Qt::QueuedConnection);
    QObject::connect(
        ntrip, &NTRIPManager::version1CastersChanged, ntrip,
        [settings](const QString& casters) { settings->ntripVersion1Casters()->setRawValue(casters); },
        Qt::QueuedConnection);
    bind(settings, ntripFromSettings, ntrip, &NTRIPManager::setConfiguration);
}

void GPSSettingsBindings::bindReceiver(RTKSettings* settings, GPSReceiver* receiver)
{
    if (!settings || !receiver) {
        return;
    }
    bind(settings, receiverFromSettings, receiver, &GPSReceiver::setConfiguration);
}

void GPSSettingsBindings::bindPositionReceiver(PositionReceiverSettings* settings, GPSReceiver* receiver)
{
    if (!settings || !receiver) {
        return;
    }
    bind(settings, positionReceiverFromSettings, receiver, &GPSReceiver::setConfiguration);
    // Out of use it is not wanted connected either. The RTK receiver's automatic connection, turned off, leaves
    // the connection the user made.
    QObject::connect(settings->enabled(), &Fact::rawValueChanged, receiver, [receiver](const QVariant& enabled) {
        if (!enabled.toBool()) {
            receiver->disconnectReceiver();
        }
    });
}

void GPSSettingsBindings::bindPosition(RTKSettings* settings, PositionReceiverSettings* positionReceiver,
                                       PositionManager* positions)
{
    if (!settings || !positionReceiver || !positions) {
        return;
    }
    bind(PositionSettings{settings, positionReceiver}, positionFromSettings, positions,
         &PositionManager::setConfiguration);
}
