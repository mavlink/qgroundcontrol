#pragma once

#include <optional>

#include <QtCore/QVariant>

#include "Fact.h"
#include "Fixtures/RAIIFixtures.h"
#include "GPSManager.h"
#include "GPSReceiver.h"
#include "GPSReceiverFactGroup.h"
#include "RTKSettings.h"
#include "Receiver/Support/GPSReceiverTestSupport.h"
#include "Receiver/Support/ScriptedReceiverWorker.h"
#include "SettingsManager.h"

/// Saved receiver settings and the application's scripted receiver, shared by the GPS panel and indicator suites.
namespace GPSTest {

/// The application's receiver settings for a manufacturer, restored when the fixture ends.
struct RTKSettingsFixture
{
    TestFixtures::SettingsFixture saved;
    RTKSettings* settings = SettingsManager::instance()->rtkSettings();
    Fact* autoConnect = settings->autoConnect();

    explicit RTKSettingsFixture(int manufacturer)
    {
        const bool passive = manufacturer == gpsReceiverManufacturerForType(GPSType::passive);
        saved.setFactValue(autoConnect, false);
        saved.setFactValue(settings->receiverRole(), passive ? RTKSettings::Passive : RTKSettings::ConfiguredBase);
        saved.setFactValue(settings->forwardReceiverRtcm(), true);
        saved.setFactValue(settings->baseReceiverManufacturers(),
                           passive ? settings->baseReceiverManufacturers()->rawValue() : QVariant(manufacturer));
        saved.setFactValue(settings->udpPort(), settings->udpPort()->rawValue());
        saved.setFactValue(settings->useFixedBasePosition(), 0);
        saved.setFactValue(settings->serialDevice(), QStringLiteral("/test/receiver"));
        saved.setFactValue(settings->serialBaudRate(), 115200);
        saved.setFactValue(settings->fixedBasePositionLatitude(), 0);
        saved.setFactValue(settings->fixedBasePositionLongitude(), 0);
        saved.setFactValue(settings->fixedBasePositionAltitude(), 0);
        saved.setFactValue(settings->fixedBasePositionAccuracy(), 0);
        saved.setFactValue(settings->compactRtcmCorrections(), false);
        saved.setFactValue(settings->connectionType(), RTKSettings::Serial);
        // Builds without serial links connect the saved serial selection over TCP.
        saved.setFactValue(settings->tcpHost(), QStringLiteral("127.0.0.1"));
        saved.setFactValue(settings->tcpPort(), 2102);
    }
};

/// The surveyed base position the application's receiver reports.
inline constexpr GPSEllipsoidPosition kSurveyedPosition{
    .latitudeDegrees = 47.123456789, .longitudeDegrees = 8.987654321, .altitudeMeters = 512.25f};

/// The application's receiver, which GPSManager saves the base position of, as a scripted u-blox base over TCP. Its
/// status Facts publish at once instead of at the fact group's update rate.
class AppReceiver
{
public:
    AppReceiver()
    {
        _receiver->setWorkerFactory(_workers.workerFactory());
        _receiver->facts()->setLiveUpdates(true);
    }

    ~AppReceiver()
    {
        _receiver->disconnectReceiver();
        _workers.finishRetired();
        _receiver->setWorkerFactory({});
        _receiver->setConfiguration(_unbound);
        _receiver->facts()->setLiveUpdates(false);
    }

    AppReceiver(const AppReceiver&) = delete;
    AppReceiver& operator=(const AppReceiver&) = delete;

    GPSReceiverFactGroup* facts() const { return _receiver->facts(); }

    /// Connects the receiver as a survey-in base, which reports ready.
    [[nodiscard]] bool connect()
    {
        if (!connectOverTcp(*_receiver)) {
            return false;
        }
        _workers.current()->ready();
        return true;
    }

    void disconnect() { _receiver->disconnectReceiver(); }

    /// Reports a valid survey at kSurveyedPosition; an unknown accuracy is unavailable.
    void survey(std::optional<double> accuracy, bool active = false)
    {
        _workers.current()->survey(
            {.position = kSurveyedPosition, .meanAccuracyMeters = accuracy, .valid = true, .active = active});
    }

private:
    GPSReceiver* const _receiver = GPSManager::instance()->receiver();
    const GPSReceiver::Configuration _unbound = _receiver->configuration();
    ScriptedReceiverWorkerFactory _workers;
};

}  // namespace GPSTest
