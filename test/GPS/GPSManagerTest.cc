#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "Fixtures/RAIIFixtures.h"
#include "GPSCorrectionManager.h"
#include "GPSCorrectionSettings.h"
#include "GPSManager.h"
#include "GPSSettingsBindings.h"
#include "GpsTestHelpers.h"
#include "NTRIPManager.h"
#include "NTRIPSettings.h"
#include "SettingsManager.h"
#include "UnitTest.h"

class GPSManagerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _correctionState();
    void _ntripSettingsBinding();
};

void GPSManagerTest::_correctionState()
{
    using State = GPSManager::CorrectionState;
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->gpsCorrectionSettings();
    saved.setFactValue(settings->rtcmUdpInputEnabled(), false);
    saved.setFactValue(SettingsManager::instance()->ntripSettings()->ntripServerConnectEnabled(), false);
    GPSManager manager;
    QSignalSpy changes(&manager, &GPSManager::correctionStateChanged);
    QCOMPARE(manager.correctionState(), State::Inactive);

    // An enabled source that has delivered nothing is waiting.
    settings->rtcmUdpInputEnabled()->setRawValue(true);
    QCOMPARE(manager.correctionState(), State::Waiting);
    settings->rtcmUdpInputEnabled()->setRawValue(false);
    QCOMPARE(manager.correctionState(), State::Inactive);

    auto source = manager.corrections()->registerSource(GPSCorrectionSource::Ntrip, QStringLiteral("caster"));
    manager.corrections()->acceptIngress(source.token().event(GpsTestHelpers::buildRtcmFrame(1005, 20),
                                                              GPSCorrectionFrame::monotonicNowMs(), 1005, true));
    QTRY_COMPARE_WITH_TIMEOUT(manager.correctionState(), State::Fresh, TestTimeout::mediumMs());
    source.reset();
    QTRY_COMPARE_WITH_TIMEOUT(manager.correctionState(), State::Inactive, TestTimeout::mediumMs());
    QCOMPARE(changes.count(), 4);
}

void GPSManagerTest::_ntripSettingsBinding()
{
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->ntripSettings();
    saved.setFactValue(settings->ntripServerConnectEnabled(), false);

    const NTRIPConfiguration expected{.connection = {.host = QStringLiteral("caster.example.com"),
                                                     .port = 443,
                                                     .username = QStringLiteral("user"),
                                                     .password = QStringLiteral("pass"),
                                                     .mountpoint = QStringLiteral("MOUNT"),
                                                     .useTls = true,
                                                     .allowSelfSignedCerts = true},
                                      .filter = {.whitelist = QStringLiteral("1005,1077")}};
    saved.setFactValue(settings->ntripServerHostAddress(), expected.connection.host);
    saved.setFactValue(settings->ntripServerPort(), expected.connection.port);
    saved.setFactValue(settings->ntripUsername(), expected.connection.username);
    saved.setFactValue(settings->ntripPassword(), expected.connection.password);
    saved.setFactValue(settings->ntripMountpoint(), expected.connection.mountpoint);
    saved.setFactValue(settings->ntripUseTls(), expected.connection.useTls);
    saved.setFactValue(settings->ntripAllowSelfSignedCerts(), expected.connection.allowSelfSignedCerts);
    saved.setFactValue(settings->ntripWhitelist(), expected.filter.whitelist);
    saved.setFactValue(settings->ntripGgaPositionSource(),
                       static_cast<int>(NTRIPGgaProvider::PositionSource::GCSPosition));
    saved.setFactValue(settings->ntripGgaIntervalSec(), 7);

    const NTRIPManager::Configuration configuration = GPSSettingsBindings::ntripConfiguration(settings);
    QVERIFY(!configuration.enabled);
    QCOMPARE(configuration.stream, expected);
    QCOMPARE(configuration.gga.source, NTRIPGgaProvider::PositionSource::GCSPosition);
    QCOMPARE(configuration.gga.interval, std::chrono::milliseconds(7000));

    NTRIPManager manager;
    GPSSettingsBindings::bindNtrip(settings, &manager);
    QCOMPARE(manager.configuration(), configuration);
    settings->ntripServerConnectEnabled()->setRawValue(true);
    QVERIFY(manager.configuration().enabled);
}

UT_REGISTER_TEST(GPSManagerTest, TestLabel::Unit)

#include "GPSManagerTest.moc"
