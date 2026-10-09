#include "GPSSettingsBindings.h"

#include <chrono>

#include <QtCore/QSet>

#include "Fact.h"
#include "GPSCorrectionSettings.h"
#include "NTRIPSettings.h"
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
    configuration.udpInput.enabled = read(settings->rtcmUdpInputEnabled()).toBool();
    configuration.udpInput.port = read(settings->rtcmUdpInputPort()).value<quint16>();
    configuration.udpOutput.enabled = read(settings->rtcmUdpOutputEnabled()).toBool();
    configuration.udpOutput.address = read(settings->rtcmUdpOutputAddress()).toString().trimmed();
    configuration.udpOutput.port = read(settings->rtcmUdpOutputPort()).value<quint16>();
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
    configuration.filter.whitelist = read(settings->ntripWhitelist()).toString();
    configuration.gga.source =
        static_cast<NTRIPGgaReporter::PositionSource>(read(settings->ntripGgaPositionSource()).toInt());
    configuration.gga.interval = std::chrono::seconds(read(settings->ntripGgaIntervalSec()).toUInt());
    return configuration;
}

/// Every base setting is read, whichever mode is selected, so each is watched.
GPSBaseStationConfig baseStationFromSettings(RTKSettings* settings, FactReader& read)
{
    using Mode = BaseModeDefinition::Mode;
    const auto mode = static_cast<Mode>(read(settings->useFixedBasePosition()).toInt());
    const GPSBaseStationConfig::Fixed fixed{
        .position = {.latitudeDegrees = read(settings->fixedBasePositionLatitude()).toDouble(),
                     .longitudeDegrees = read(settings->fixedBasePositionLongitude()).toDouble(),
                     .altitudeMeters = read(settings->fixedBasePositionAltitude()).toDouble()},
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
        default:
            qCWarning(GPSSettingsBindingsLog) << "Invalid base mode; using survey-in";
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
    configuration.baseReceiverManufacturer = read(settings->baseReceiverManufacturers()).toInt();
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

PositionConfig positionFromSettings(RTKSettings* settings, FactReader& read)
{
    return {.sourceMode = static_cast<PositionManager::SourceMode>(read(settings->gcsPositionSource()).toInt())};
}

/// The log names whether these are set, never their values: credentials and the base station's location.
bool redacted(const Fact* fact)
{
    static const QSet<QString> names{
        QString::fromLatin1(NTRIPSettings::ntripPasswordName),
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

template <typename Settings, typename Configuration>
QList<Fact*> readFacts(Settings* settings, Configuration (*build)(Settings*, FactReader&))
{
    FactReader reader;
    (void) build(settings, reader);
    return reader.facts();
}

/// Applies the built configuration to @a target now and whenever a Fact it reads changes, logging each value.
template <typename Settings, typename Configuration, typename Target>
void bind(Settings* settings, Configuration (*build)(Settings*, FactReader&), Target* target,
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

QList<Fact*> GPSSettingsBindings::_boundFacts(RTKSettings* settings)
{
    return readFacts(settings, receiverFromSettings) + readFacts(settings, positionFromSettings);
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
    // Queued: the manager emits these inside its own operation (retrying, trusting a certificate), and storing the
    // Fact applies a new configuration to it, which must wait until that returns.
    QObject::connect(
        ntrip, &NTRIPManager::enableRequested, ntrip,
        [settings]() { settings->ntripServerConnectEnabled()->setRawValue(true); }, Qt::QueuedConnection);
    QObject::connect(
        ntrip, &NTRIPManager::certificatePinChanged, ntrip,
        [settings](const QString& pin) { settings->ntripPinnedCertificate()->setRawValue(pin); }, Qt::QueuedConnection);
    bind(settings, ntripFromSettings, ntrip, &NTRIPManager::setConfiguration);
}

void GPSSettingsBindings::bindReceiver(RTKSettings* settings, GPSReceiver* receiver)
{
    if (!settings || !receiver) {
        return;
    }
    bind(settings, receiverFromSettings, receiver, &GPSReceiver::setConfiguration);
}

void GPSSettingsBindings::bindPosition(RTKSettings* settings, PositionManager* positions)
{
    if (!settings || !positions) {
        return;
    }
    bind(settings, positionFromSettings, positions, &PositionManager::setConfiguration);
}
