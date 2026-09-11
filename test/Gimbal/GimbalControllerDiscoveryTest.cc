#include "GimbalControllerDiscoveryTest.h"

#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "Gimbal.h"
#include "GimbalController.h"
#include "MockConfiguration.h"
#include "MockLink.h"
#include "MultiVehicleManager.h"
#include "QmlObjectListModel.h"
#include "UnitTest.h"
#include "Vehicle.h"

namespace {

constexpr uint8_t kAutopilotAttachedDeviceId = 1;
constexpr uint8_t kUnknownGimbalCompId = MAV_COMP_ID_GIMBAL2;
constexpr uint8_t kOutOfRangeDeviceId = 200;
constexpr int kManagerInfoHeartbeatRetries = 6;  // GimbalController::PotentialGimbalManager
constexpr int kStatusIntervalRetries = 6;        // Gimbal::_requestStatusRetries
constexpr int kStatusIntervalFallbackUs = 5000000;

}  // namespace

void GimbalControllerDiscoveryTest::_startGimbalMockLink(const std::function<void(MockConfiguration*)>& configure,
                                                         const std::function<void(MockLinkGimbal*)>& prepareGimbal)
{
    QVERIFY2(!_mockLink, "MockLink already connected");

    QSignalSpy spyVehicle(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged);
    QVERIFY(spyVehicle.isValid());

    MockConfiguration* const config = new MockConfiguration(QStringLiteral("Gimbal discovery MockLink"));
    config->setFirmwareType(MAV_AUTOPILOT_PX4);
    config->setVehicleType(MAV_TYPE_QUADROTOR);
    config->setEnableGimbal(true);
    if (configure) {
        configure(config);
    }

    _mockLink = MockLink::startMockLink(config);
    QVERIFY(_mockLink);
    (void) connect(_mockLink, &QObject::destroyed, this, [this]() { _mockLink = nullptr; });
    QVERIFY(mockGimbal());
    if (prepareGimbal) {
        prepareGimbal(mockGimbal());
    }

    QVERIFY2(UnitTest::waitForSignal(spyVehicle, TestTimeout::longMs(), QStringLiteral("activeVehicleChanged")),
             "Timeout waiting for vehicle connection");
    _vehicle = MultiVehicleManager::instance()->activeVehicle();
    QVERIFY(_vehicle);
    QVERIFY_TRUE_WAIT(_vehicle->isInitialConnectComplete(), TestTimeout::longMs());
}

GimbalController* GimbalControllerDiscoveryTest::gimbalController() const
{
    return _vehicle ? _vehicle->gimbalController() : nullptr;
}

Gimbal* GimbalControllerDiscoveryTest::activeGimbal() const
{
    GimbalController* const controller = gimbalController();
    return controller ? controller->activeGimbal() : nullptr;
}

MockLinkGimbal* GimbalControllerDiscoveryTest::mockGimbal() const
{
    return _mockLink ? _mockLink->mockLinkGimbal() : nullptr;
}

void GimbalControllerDiscoveryTest::_testAutopilotAttachedGimbal()
{
    // Device ids 1-6 denote a gimbal without its own MAVLink component; all traffic comes from the manager
    _startGimbalMockLink([](MockConfiguration* config) { config->setGimbalDeviceId(kAutopilotAttachedDeviceId); },
                         nullptr);
    if (QTest::currentTestFailed()) {
        return;
    }

    QVERIFY_TRUE_WAIT(activeGimbal() != nullptr, TestTimeout::longMs());
    Gimbal* const gimbal = activeGimbal();
    QCOMPARE(gimbal->deviceId()->rawValue().toUInt(), static_cast<uint>(kAutopilotAttachedDeviceId));
    QCOMPARE(gimbal->managerCompid()->rawValue().toUInt(), static_cast<uint>(MAV_COMP_ID_AUTOPILOT1));
    QCOMPARE(_vehicle->getFactGroup(
                 QStringLiteral("gimbal%1%2").arg(MAV_COMP_ID_AUTOPILOT1).arg(kAutopilotAttachedDeviceId)),
             gimbal);

    // Attitude addressed by device id field flows through
    mockGimbal()->setAttitudeDeg(0.0f, 12.0f, 0.0f);
    mockGimbal()->sendGimbalDeviceAttitudeStatusNow();
    QVERIFY_TRUE_WAIT(qAbs(gimbal->absolutePitch()->rawValue().toFloat() - 12.0f) < 0.1f, TestTimeout::mediumMs());

    // Commands target the sub-device id, not a component id
    gimbalController()->acquireGimbalControl();
    QVERIFY_TRUE_WAIT(gimbal->gimbalHaveControl(), TestTimeout::longMs());
    const int count = mockGimbal()->lastPitchYawCommand().count;
    gimbalController()->sendPitchBodyYaw(-5.0f, 5.0f, false);
    QVERIFY_TRUE_WAIT(mockGimbal()->lastPitchYawCommand().count > count, TestTimeout::mediumMs());
    QCOMPARE(mockGimbal()->lastPitchYawCommand().deviceId, kAutopilotAttachedDeviceId);
}

void GimbalControllerDiscoveryTest::_testInvalidMessagesIgnored()
{
    _startGimbalMockLink(nullptr, nullptr);
    if (QTest::currentTestFailed()) {
        return;
    }
    QVERIFY_TRUE_WAIT(activeGimbal() != nullptr, TestTimeout::longMs());
    Gimbal* const gimbal = activeGimbal();
    MockLinkGimbal* const mock = mockGimbal();

    // Manager information with device id 0 is a protocol violation and is logged as such
    expectLogMessage("Gimbal.GimbalController", QtWarningMsg,
                     QRegularExpression(QStringLiteral("invalid gimbal device")));
    mock->sendGimbalManagerInformationWithDeviceId(0);
    // Status with device id 0 and attitude from an unknown component / with an out-of-range device id are dropped
    QSignalSpy pitchSpy(gimbal->absolutePitch(), &Fact::rawValueChanged);
    mock->sendGimbalManagerStatusWithDeviceId(0);
    mock->setAttitudeDeg(0.0f, 33.0f, 0.0f);
    mock->sendGimbalDeviceAttitudeStatusFrom(kUnknownGimbalCompId, 0);
    mock->sendGimbalDeviceAttitudeStatusFrom(MAV_COMP_ID_GIMBAL, kOutOfRangeDeviceId);

    // A following valid message proves the invalid ones were processed (and dropped) before it
    mock->setAttitudeDeg(0.0f, 34.0f, 0.0f);
    mock->sendGimbalDeviceAttitudeStatusNow();
    QVERIFY_TRUE_WAIT(qAbs(gimbal->absolutePitch()->rawValue().toFloat() - 34.0f) < 0.1f, TestTimeout::mediumMs());
    verifyExpectedLogMessage();

    for (const QList<QVariant>& args : pitchSpy) {
        QVERIFY2(qAbs(args.first().toFloat() - 33.0f) > 0.1f, "attitude from rejected message reached the gimbal");
    }
    QCOMPARE(gimbalController()->gimbals()->count(), 1);
    QCOMPARE(activeGimbal(), gimbal);
    QCOMPARE(gimbal->deviceId()->rawValue().toUInt(), static_cast<uint>(MAV_COMP_ID_GIMBAL));
}

void GimbalControllerDiscoveryTest::_testManagerInformationUnavailable_data()
{
    QTest::addColumn<MockLinkGimbal::InformationResponse>("response");

    QTest::newRow("silent") << MockLinkGimbal::InformationResponse::Silent;
    QTest::newRow("nack") << MockLinkGimbal::InformationResponse::Nack;
}

void GimbalControllerDiscoveryTest::_testManagerInformationUnavailable()
{
    QFETCH(MockLinkGimbal::InformationResponse, response);

    if (response == MockLinkGimbal::InformationResponse::Silent) {
        // Every unanswered request exhausts the command queue's resends
        ignoreLogMessage(
            "Vehicle.MavCommandQueue", QtWarningMsg,
            QRegularExpression(QStringLiteral("Giving up sending command after max retries: MAV_CMD_REQUEST_MESSAGE "
                                              "message: GIMBAL_MANAGER_INFORMATION")));
    }

    _startGimbalMockLink(nullptr, [response](MockLinkGimbal* mock) { mock->setInformationResponse(response); });
    if (QTest::currentTestFailed()) {
        return;
    }

    // Requests are issued per heartbeat while the retry budget lasts. Each unanswered request also carries the
    // command layer's own resend attempts, so only the upper bound and the eventual stop are deterministic.
    constexpr int kCommandResends = 3;  // Vehicle command queue attempts per request
    const auto requestCount = [this]() {
        return _mockLink->receivedRequestMessageCount(MAVLINK_MSG_ID_GIMBAL_MANAGER_INFORMATION);
    };

    QVERIFY_TRUE_WAIT(requestCount() >= 1, TestTimeout::longMs());
    int lastCount = requestCount();
    while (QTest::qWaitFor([&]() { return requestCount() > lastCount; }, TestTimeout::mediumMs())) {
        lastCount = requestCount();
    }
    QVERIFY2(lastCount <= kManagerInfoHeartbeatRetries * kCommandResends,
             qPrintable(QStringLiteral("%1 requests sent").arg(lastCount)));
    if (response == MockLinkGimbal::InformationResponse::Nack) {
        QCOMPARE(lastCount, kManagerInfoHeartbeatRetries);  // immediate NACK: no command-layer resends
    }
    QVERIFY(!activeGimbal());
    QCOMPARE(gimbalController()->gimbals()->count(), 0);
}

void GimbalControllerDiscoveryTest::_testStatusBeforeInformation()
{
    // Unsolicited GIMBAL_MANAGER_STATUS before any GIMBAL_MANAGER_INFORMATION must seed the gimbal, not be dropped,
    // and discovery must still complete once information finally arrives
    ignoreLogMessage("Vehicle.MavCommandQueue", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Giving up sending command after max retries: "
                                                       "MAV_CMD_REQUEST_MESSAGE message: GIMBAL_MANAGER_INFORMATION")));

    _startGimbalMockLink(nullptr, [](MockLinkGimbal* mock) {
        mock->setInformationResponse(MockLinkGimbal::InformationResponse::Silent);
    });
    if (QTest::currentTestFailed()) {
        return;
    }
    MockLinkGimbal* const mock = mockGimbal();

    // Attitude for a device QGC has never heard of is dropped; status is what creates the potential gimbal.
    // Both are queued on the link ahead of any information reply, so QGC processes them first.
    mock->sendGimbalDeviceAttitudeStatusNow();
    mock->sendGimbalManagerStatusNow();
    QVERIFY(!activeGimbal());
    mock->setInformationResponse(MockLinkGimbal::InformationResponse::Accept);

    QVERIFY_TRUE_WAIT(activeGimbal() != nullptr, TestTimeout::longMs());
    Gimbal* const gimbal = activeGimbal();
    QCOMPARE(gimbalController()->gimbals()->count(), 1);
    QCOMPARE(gimbal->deviceId()->rawValue().toUInt(), static_cast<uint>(MAV_COMP_ID_GIMBAL));
    QCOMPARE(gimbal->managerCompid()->rawValue().toUInt(), static_cast<uint>(MAV_COMP_ID_AUTOPILOT1));

    mock->setAttitudeDeg(0.0f, 21.0f, 0.0f);
    mock->sendGimbalDeviceAttitudeStatusNow();
    QVERIFY_TRUE_WAIT(qAbs(gimbal->absolutePitch()->rawValue().toFloat() - 21.0f) < 0.1f, TestTimeout::mediumMs());
}

void GimbalControllerDiscoveryTest::_testStatusIntervalRetryFallback()
{
    // First four interval requests are ignored; QGC must fall back to an explicit 0.2 Hz interval and still complete
    constexpr int kIgnoredRequests = 4;
    _startGimbalMockLink(nullptr,
                         [](MockLinkGimbal* mock) { mock->setIgnoreStatusIntervalRequests(kIgnoredRequests); });
    if (QTest::currentTestFailed()) {
        return;
    }

    QVERIFY_TRUE_WAIT(activeGimbal() != nullptr, TestTimeout::longMs());

    const QList<int> requests = mockGimbal()->statusIntervalRequests();
    QVERIFY2(requests.count() > kIgnoredRequests,
             qPrintable(QStringLiteral("only %1 interval requests").arg(requests.count())));
    QVERIFY(requests.count() <= kStatusIntervalRetries);
    for (int i = 0; i < kIgnoredRequests; ++i) {
        QCOMPARE(requests[i], 0);  // default rate
    }
    QCOMPARE(requests[kIgnoredRequests], kStatusIntervalFallbackUs);
}

UT_REGISTER_TEST(GimbalControllerDiscoveryTest, TestLabel::Integration, TestLabel::Vehicle)
