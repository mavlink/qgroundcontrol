#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "Fixtures/RAIIFixtures.h"
#include "GPSCorrectionManager.h"
#include "GPSCorrectionSettings.h"
#include "GPSManager.h"
#include "GpsTestHelpers.h"
#include "NTRIPSettings.h"
#include "SettingsManager.h"
#include "UnitTest.h"

class GPSManagerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _correctionState();
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

UT_REGISTER_TEST(GPSManagerTest, TestLabel::Unit)

#include "GPSManagerTest.moc"
