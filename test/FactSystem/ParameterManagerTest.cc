#include "ParameterManagerTest.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>
#include <memory>

#include <QtCore/QDir>
#include <QtCore/QElapsedTimer>
#include <QtCore/QPointer>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtCore/QStandardPaths>
#include <QtCore/QTemporaryDir>
#include <QtTest/QSignalSpy>

#include "BulkRefreshJob.h"
#include "FTPManager.h"
#include "FileFixtures.h"
#include "LogManager.h"
#include "MAVLinkLib.h"
#include "MockLinkFTP.h"
#include "MultiVehicleManager.h"
#include "ParameterManager.h"
#include "ProcessFixtures.h"
#include "QGCApplication.h"
#include "QGCMath.h"
#include "QGCStateMachine.h"
#include "QmlObjectListModel.h"
#include "Vehicle.h"

// Call from tests that deliberately let PARAM_SET / PARAM_REQUEST_READ waits time out.
void ParameterManagerTest::_ignoreParamResponseTimeouts()
{
    ignoreLogMessage("Utilities.StateMachine.QGCStateMachine", QtWarningMsg,
                     QRegularExpression("Timeout \".*WaitForParamResponseState\""));
}

void ParameterManagerTest::_timeRequestsByHand(Vehicle* vehicle)
{
    QVERIFY(vehicle);
    if (!_scheduler) {
        _scheduler = std::make_unique<ManualScheduler>();
    }
    vehicle->parameterManager()->setScheduler(_scheduler.get());
}

void ParameterManagerTest::_timeNextVehicleByHand()
{
    _vehicleAddedConnection = connect(
        MultiVehicleManager::instance(), &MultiVehicleManager::vehicleAdded, this,
        [this](Vehicle* vehicle) { _timeRequestsByHand(vehicle); }, Qt::SingleShotConnection);
}

void ParameterManagerTest::_expireAckTimeout(uint32_t requestMessageId, int receivedCount)
{
    QVERIFY(_scheduler);
    QTRY_COMPARE_WITH_TIMEOUT(_mockLink->receivedMavlinkMessageCount(requestMessageId), receivedCount,
                              TestTimeout::mediumMs());
    // The timeout of the attempt runs on the scheduler of the test, and not by itself. One ack timeout and no more:
    // the timeout of the attempt that follows is not to expire with it.
    QCOMPARE_GT(_scheduler->pendingCount(), 0);
    QVERIFY(_scheduler->advanceBy(std::chrono::milliseconds(ParameterManager::kWaitForParamValueAckMs)));
}

void ParameterManagerTest::_expectIndexLoadFailureWarning()
{
    expectLogMessage("FactSystem.ParameterManager", QtWarningMsg,
                     QRegularExpression("could not be loaded after the maximum number of retries"));
}

// Starts a PX4 MockLink with the packed param file disabled so the PARAM_REQUEST_LIST stream path is exercised.
Vehicle *ParameterManagerTest::_connectAndWaitForVehicle(MockConfiguration::FailureMode_t failureMode)
{
    if (_mockLink) {
        qWarning() << "MockLink already connected";
        return nullptr;
    }
    // On a slow runner the PARAM_VALUE stream can delay a COMMAND_ACK past the ack timeout. A stray
    // UNSUPPORTED ack from the retried GIMBAL_MANAGER_INFORMATION probe (which MockLink rejects) can
    // then be credited to the post-connect AVAILABLE_MODES request, which is unrelated to these tests.
    ignoreLogMessage("Vehicle.StandardModes", QtWarningMsg, QRegularExpression("Failed to retrieve available modes"));
    _mockLink = MockLink::startPX4MockLink(MockConfiguration::OptionNone, failureMode);
    _mockLink->mockLinkFTP()->setParamPckEnabled(false);
    MultiVehicleManager* vehicleMgr = MultiVehicleManager::instance();
    QSignalSpy spyVehicle(vehicleMgr, &MultiVehicleManager::activeVehicleAvailableChanged);
    if (!UnitTest::waitForSignal(spyVehicle, TestTimeout::mediumMs(), QStringLiteral("activeVehicleAvailableChanged"))) {
        return nullptr;
    }
    return vehicleMgr->activeVehicle();
}

int ParameterManagerTest::_readCountForComponent(const QList<QPair<int, int>> &readLog, int componentId)
{
    return static_cast<int>(std::count_if(readLog.cbegin(), readLog.cend(), [componentId](const QPair<int, int> &entry) { return entry.first == componentId; }));
}

bool ParameterManagerTest::_waitForAppMessage(const QRegularExpression &pattern)
{
    return UnitTest::waitForCondition([&pattern]() {
        const auto messages = LogManager::capturedMessages();
        return std::any_of(messages.cbegin(), messages.cend(), [&pattern](const LogEntry &entry) {
            return (entry.category == QLatin1String("API.QGCApplication.AppMessage")) && pattern.match(entry.message).hasMatch();
        });
    }, TestTimeout::longMs(), QStringLiteral("app message"));
}

void ParameterManagerTest::cleanup()
{
    // Some tests create MockLink directly (not via _connectMockLink), so we need special handling.
    // For those cases, disconnect directly and wait for cleanup before calling base class.
    if (_mockLink && MultiVehicleManager::instance()->activeVehicle()) {
        _mockLink->disconnect();
        _mockLink = nullptr;
        QSignalSpy spy(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged);
        if (!UnitTest::waitForSignal(spy, TestTimeout::mediumMs(), QStringLiteral("activeVehicleChanged"))) {
            qWarning() << "ParameterManagerTest::cleanup: timeout waiting for vehicle disconnect";
        }
    }
    VehicleTestManualConnect::cleanup();
    disconnect(_vehicleAddedConnection);
    _scheduler.reset();
}

/// Test failure modes which should still lead to param load success
void ParameterManagerTest::_noFailureWorker(MockConfiguration::FailureMode_t failureMode)
{
    QVERIFY2(!_mockLink, "MockLink already connected");
    MultiVehicleManager* vehicleMgr = MultiVehicleManager::instance();
    QVERIFY(vehicleMgr);
    // Spy from vehicle creation: a fast load can complete before the vehicle becomes active
    QSignalSpy spyVehicle(vehicleMgr, &MultiVehicleManager::activeVehicleAvailableChanged);
    QSignalSpy spyParamsReady(vehicleMgr, &MultiVehicleManager::parameterReadyVehicleAvailableChanged);
    std::unique_ptr<QSignalSpy> spyProgress;
    const QMetaObject::Connection vehicleAddedConnection =
        connect(vehicleMgr, &MultiVehicleManager::vehicleAdded, this, [&spyProgress](Vehicle* addedVehicle) {
            spyProgress =
                std::make_unique<QSignalSpy>(addedVehicle->parameterManager(), &ParameterManager::loadProgressChanged);
        });
    const auto disconnectVehicleAdded = qScopeGuard([vehicleAddedConnection] { disconnect(vehicleAddedConnection); });
    _mockLink = MockLink::startPX4MockLink(MockConfiguration::OptionNone, failureMode);
    // Wait for the Vehicle to get created
    QVERIFY_SIGNAL_COUNT_WAIT(spyVehicle, 1, TestTimeout::mediumMs());
    QCOMPARE(spyVehicle.count(), 1);
    QList<QVariant> arguments = spyVehicle.takeFirst();
    QCOMPARE(arguments.count(), 1);
    QCOMPARE(arguments.at(0).toBool(), true);
    Vehicle* vehicle = vehicleMgr->activeVehicle();
    QVERIFY(vehicle);
    QVERIFY(spyProgress);
    // We should get progress bar updates during load. The first vehicle of a process takes several times as long
    // to get there as a later one
    QVERIFY_SIGNAL_COUNT_WAIT(*spyProgress, 1, TestTimeout::longMs());
    arguments = spyProgress->takeFirst();
    QCOMPARE(arguments.count(), 1);
    QVERIFY(arguments.at(0).toFloat() > 0.0f);
    // When param load is complete we get the param ready signal
    QVERIFY_SIGNAL_COUNT_WAIT(spyParamsReady, 1, TestTimeout::longMs());
    arguments = spyParamsReady.takeFirst();
    QCOMPARE(arguments.count(), 1);
    QCOMPARE(arguments.at(0).toBool(), true);
    // Progress should have been set back to 0
    arguments = spyProgress->takeLast();
    QCOMPARE(arguments.count(), 1);
    QCOMPARE(arguments.at(0).toFloat(), 0.0f);
}

void ParameterManagerTest::_noFailure()
{
    _noFailureWorker(MockConfiguration::FailNone);
}

void ParameterManagerTest::_requestListMissingParamSuccess()
{
    _noFailureWorker(MockConfiguration::FailMissingParamOnInitialRequest);
}

// Test no response to param_request_list
void ParameterManagerTest::_requestListNoResponse()
{
    QVERIFY2(!_mockLink, "MockLink already connected");
    _timeNextVehicleByHand();
    _mockLink = MockLink::startPX4MockLink(MockConfiguration::OptionNone, MockConfiguration::FailParamNoResponseToRequestList);
    _mockLink->mockLinkFTP()->setParamPckEnabled(false);
    MultiVehicleManager* vehicleMgr = MultiVehicleManager::instance();
    QVERIFY(vehicleMgr);
    // Wait for the Vehicle to get created
    QSignalSpy spyVehicle(vehicleMgr, &MultiVehicleManager::activeVehicleAvailableChanged);
    QVERIFY_SIGNAL_WAIT(spyVehicle, TestTimeout::mediumMs());
    QCOMPARE(spyVehicle.count(), 1);
    QList<QVariant> arguments = spyVehicle.takeFirst();
    QCOMPARE(arguments.count(), 1);
    QCOMPARE(arguments.at(0).toBool(), true);
    Vehicle* vehicle = vehicleMgr->activeVehicle();
    QVERIFY(vehicle);
    QSignalSpy spyParamsReady(vehicleMgr, &MultiVehicleManager::parameterReadyVehicleAvailableChanged);
    QSignalSpy spyProgress(vehicle->parameterManager(), &ParameterManager::loadProgressChanged);
    QSignalSpy spyRequestFailed(vehicle->parameterManager(), &ParameterManager::initialParametersRequestFailed);
    expectAppMessage(QRegularExpression("did not respond to request for parameters"));
    expectLogMessage("FactSystem.ParameterManager", QtWarningMsg,
                     QRegularExpression("did not respond to request for parameters"));
    // ParameterManager sends the request again each time it is left unanswered, and gives up after its retries
    // without any progress bar updates or a parameter ready signal.
    int requestsSent = 0;
    while (spyRequestFailed.isEmpty()) {
        requestsSent++;
        QCOMPARE_LE(requestsSent, ParameterManager::kMaxInitialRequestListRetry + 1);
        QTRY_COMPARE_WITH_TIMEOUT(_mockLink->receivedMavlinkMessageCount(MAVLINK_MSG_ID_PARAM_REQUEST_LIST),
                                  requestsSent, TestTimeout::mediumMs());
        QCOMPARE_GT(_scheduler->pendingCount(), 0);
        QVERIFY(_scheduler->advanceBy(std::chrono::milliseconds(ParameterManager::kTestInitialRequestIntervalMs)));
    }
    // The download of the parameter file, which this vehicle refuses at once, was the first attempt
    QCOMPARE(requestsSent, ParameterManager::kMaxInitialRequestListRetry);
    QCOMPARE(spyProgress.count(), 0);
    QCOMPARE(spyParamsReady.count(), 0);
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
}

// ArduPilot streams parameters that nobody has asked for, with an index of 65535, and begins before it answers
// PARAM_REQUEST_LIST. One of them is not the answer: taken for it, it ended the wait for the list.
void ParameterManagerTest::_unrequestedParamBeforeListResponseIsDisregarded()
{
    _timeNextVehicleByHand();
    Vehicle* const vehicle = _connectAndWaitForVehicle(MockConfiguration::FailParamNoResponseToRequestList);
    QVERIFY(vehicle);
    ParameterManager* const paramManager = vehicle->parameterManager();
    QTRY_COMPARE_WITH_TIMEOUT(_mockLink->receivedMavlinkMessageCount(MAVLINK_MSG_ID_PARAM_REQUEST_LIST), 1,
                              TestTimeout::mediumMs());

    constexpr uint16_t unrequestedIndex = 65535;
    bool paramValueHandled = false;
    const QObject connectionContext;
    connect(vehicle, &Vehicle::mavlinkMessageReceived, &connectionContext,
            [&paramValueHandled](const mavlink_message_t& message) {
                if ((message.msgid == MAVLINK_MSG_ID_PARAM_VALUE) &&
                    (mavlink_msg_param_value_get_param_index(&message) == unrequestedIndex)) {
                    paramValueHandled = true;
                }
            });
    const char paramName[MAVLINK_MSG_PARAM_VALUE_FIELD_PARAM_ID_LEN] = "UNREQUESTED";
    mavlink_message_t message{};
    (void) mavlink_msg_param_value_pack_chan(static_cast<uint8_t>(_mockLink->vehicleId()), MAV_COMP_ID_AUTOPILOT1,
                                             _mockLink->outgoingMavlinkChannel(), &message, paramName, 1.0f,
                                             MAV_PARAM_TYPE_REAL32, 1, unrequestedIndex);
    _mockLink->respondWithMavlinkMessage(message);
    // Vehicle hands a message to its parameter manager before it announces it
    QTRY_VERIFY_WITH_TIMEOUT(paramValueHandled, TestTimeout::mediumMs());
    QVERIFY(!paramManager->parameterExists(MAV_COMP_ID_AUTOPILOT1, QString::fromLatin1(paramName)));

    // The list is still waited for, and requested again when it does not come
    QVERIFY(_scheduler->advanceBy(std::chrono::milliseconds(ParameterManager::kTestInitialRequestIntervalMs)));
    QTRY_COMPARE_WITH_TIMEOUT(_mockLink->receivedMavlinkMessageCount(MAVLINK_MSG_ID_PARAM_REQUEST_LIST), 2,
                              TestTimeout::mediumMs());
}

// MockLink will fail to send a param on initial request, it will also fail to send it on subsequent
// param_read requests. The packed file is disabled so the stream path is exercised.
// The initial-load re-read loop is the only retry authority: exactly one PARAM_REQUEST_READ per retry cycle,
// and no PARAM_REQUEST_READ state machine timeouts.
void ParameterManagerTest::_requestListMissingParamFail()
{
    QVERIFY2(!_mockLink, "MockLink already connected");
    _mockLink = MockLink::startPX4MockLink(MockConfiguration::OptionNone, MockConfiguration::FailMissingParamOnAllRequests);
    _mockLink->mockLinkFTP()->setParamPckEnabled(false);
    MultiVehicleManager* vehicleMgr = MultiVehicleManager::instance();
    QVERIFY(vehicleMgr);
    // Wait for the Vehicle to get created
    QSignalSpy spyVehicle(vehicleMgr, &MultiVehicleManager::activeVehicleAvailableChanged);
    QVERIFY_SIGNAL_WAIT(spyVehicle, TestTimeout::mediumMs());
    QCOMPARE(spyVehicle.count(), 1);
    QList<QVariant> arguments = spyVehicle.takeFirst();
    QCOMPARE(arguments.count(), 1);
    QCOMPARE(arguments.at(0).toBool(), true);
    Vehicle* vehicle = vehicleMgr->activeVehicle();
    QVERIFY(vehicle);
    QSignalSpy spyParamsReady(vehicleMgr, &MultiVehicleManager::parameterReadyVehicleAvailableChanged);
    QSignalSpy spyProgress(vehicle->parameterManager(), &ParameterManager::loadProgressChanged);
    // We will get progress bar updates, since it will fail after getting partially through the request
    QVERIFY_SIGNAL_WAIT(spyProgress, TestTimeout::mediumMs());
    arguments = spyProgress.takeFirst();
    QCOMPARE(arguments.count(), 1);
    QVERIFY(arguments.at(0).toFloat() > 0.0f);
    expectAppMessage(QRegularExpression("was unable to retrieve the full set of parameters from vehicle"));
    _expectIndexLoadFailureWarning();
    // We should get a parameters ready signal, but Vehicle should indicate missing params
    QVERIFY_SIGNAL_WAIT(spyParamsReady, TestTimeout::longMs());
    QCOMPARE(vehicle->parameterManager()->missingParameters(), true);
    QCOMPARE(_mockLink->paramRequestReadIndexLog().count(), ParameterManager::kMaxInitialLoadRetrySingleParam);
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
}

// The autopilot serves all its params but a second component (simulated DroneCAN node) never serves one of
// its params. Only default component failures may block the UI, so missingParameters must stay false.
void ParameterManagerTest::_requestListMissingParamNonDefaultComponentFail()
{
    QVERIFY2(!_mockLink, "MockLink already connected");
    _mockLink = MockLink::startPX4MockLink(MockConfiguration::OptionNone, MockConfiguration::FailMissingParamOnAllRequestsNonDefaultComponent);
    _mockLink->mockLinkFTP()->setParamPckEnabled(false);
    MultiVehicleManager* vehicleMgr = MultiVehicleManager::instance();
    QVERIFY(vehicleMgr);
    QSignalSpy spyVehicle(vehicleMgr, &MultiVehicleManager::activeVehicleAvailableChanged);
    QVERIFY_SIGNAL_WAIT(spyVehicle, TestTimeout::mediumMs());
    Vehicle* vehicle = vehicleMgr->activeVehicle();
    QVERIFY(vehicle);
    QSignalSpy spyParamsReady(vehicleMgr, &MultiVehicleManager::parameterReadyVehicleAvailableChanged);
    expectAppMessage(QRegularExpression("was unable to retrieve the full set of parameters from component"));
    _expectIndexLoadFailureWarning();
    QVERIFY_SIGNAL_WAIT(spyParamsReady, TestTimeout::longMs());
    QCOMPARE(vehicle->parameterManager()->missingParameters(), false);
    QVERIFY(_waitForAppMessage(QRegularExpression("from component")));
    // A single dead index is below the unresponsive-component threshold: per-index retries only
    QCOMPARE(_readCountForComponent(_mockLink->paramRequestReadIndexLog(), 125), ParameterManager::kMaxInitialLoadRetrySingleParam);
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
}

// The second component streams 2 of its 14 params and never answers a read. Once enough re-reads are in flight
// and it stays silent for consecutive cycles it must be given up on as a whole, not one index at a time.
void ParameterManagerTest::_requestListNonDefaultComponentDead()
{
    Vehicle* vehicle = _connectAndWaitForVehicle(MockConfiguration::FailNonDefaultComponentDead);
    QVERIFY(vehicle);
    MultiVehicleManager* vehicleMgr = MultiVehicleManager::instance();
    QSignalSpy spyParamsReady(vehicleMgr, &MultiVehicleManager::parameterReadyVehicleAvailableChanged);
    expectLogMessage("FactSystem.ParameterManager", QtWarningMsg, QRegularExpression("unresponsive"));
    expectAppMessage(QRegularExpression("was unable to retrieve the full set of parameters from component"));
    _expectIndexLoadFailureWarning();
    QVERIFY_SIGNAL_WAIT(spyParamsReady, TestTimeout::longMs());
    QCOMPARE(vehicle->parameterManager()->missingParameters(), false);
    QVERIFY(_waitForAppMessage(QRegularExpression("from component")));
    const int readCount = _readCountForComponent(_mockLink->paramRequestReadIndexLog(), 125);
    QVERIFY2(readCount <= ParameterManager::kUnresponsiveSilentCycles * ParameterManager::kIndexBatchMaxOutstanding,
             qPrintable(QStringLiteral("%1 reads sent to dead component").arg(readCount)));
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
}

// Same component, but it answers the second read of every param. A lossy component must not be declared
// unresponsive: every param loads, no failure message.
void ParameterManagerTest::_requestListNonDefaultComponentLossy()
{
    Vehicle* vehicle = _connectAndWaitForVehicle(MockConfiguration::FailNonDefaultComponentLossy);
    QVERIFY(vehicle);
    MultiVehicleManager* vehicleMgr = MultiVehicleManager::instance();
    QSignalSpy spyParamsReady(vehicleMgr, &MultiVehicleManager::parameterReadyVehicleAvailableChanged);
    QVERIFY_SIGNAL_WAIT(spyParamsReady, TestTimeout::longMs());
    QCOMPARE(vehicle->parameterManager()->missingParameters(), false);
    QTRY_COMPARE_WITH_TIMEOUT(vehicle->parameterManager()->parameterNames(125).count(), 14, TestTimeout::longMs());
}

// Ready is gated on the autopilot alone. The second component streamed first (MAV_COMP_ID_ALL) and is still
// re-reading its lost params when the autopilot finishes; it must not hold the UI and must keep loading afterwards.
void ParameterManagerTest::_requestListNonDefaultComponentDoesNotGateReady()
{
    Vehicle* vehicle = _connectAndWaitForVehicle(MockConfiguration::FailNonDefaultComponentLossy);
    QVERIFY(vehicle);
    MultiVehicleManager* vehicleMgr = MultiVehicleManager::instance();
    QSignalSpy spyParamsReady(vehicleMgr, &MultiVehicleManager::parameterReadyVehicleAvailableChanged);
    QVERIFY_SIGNAL_WAIT(spyParamsReady, TestTimeout::longMs());
    QCOMPARE(_readCountForComponent(_mockLink->paramRequestReadIndexLog(), 125), 0);
    QVERIFY(vehicle->parameterManager()->parameterNames(125).count() < 14);
    // The bar is initial-load UI; the background load of the second component must not bring it back
    QSignalSpy spyProgress(vehicle->parameterManager(), &ParameterManager::loadProgressChanged);
    QTRY_COMPARE_WITH_TIMEOUT(vehicle->parameterManager()->parameterNames(125).count(), 14, TestTimeout::longMs());
    QCOMPARE(vehicle->parameterManager()->missingParameters(), false);
    QCOMPARE(spyProgress.count(), 0);
}

// A user refresh-all on a PX4 vehicle takes the FTP path, which never touches the stream-progress bookkeeping.
// Once the packed file is parsed the bar must stay down even though another component is still loading via
// the stream path.
void ParameterManagerTest::_refreshAllViaFtpKeepsProgressDownForBackgroundComponent()
{
    QVERIFY2(!_mockLink, "MockLink already connected");
    MultiVehicleManager* vehicleMgr = MultiVehicleManager::instance();
    QSignalSpy spyVehicle(vehicleMgr, &MultiVehicleManager::activeVehicleAvailableChanged);
    // Spy before connecting: the FTP load can complete before the vehicle becomes active
    QSignalSpy spyParamsReady(vehicleMgr, &MultiVehicleManager::parameterReadyVehicleAvailableChanged);
    _mockLink =
        MockLink::startPX4MockLink(MockConfiguration::OptionNone, MockConfiguration::FailNonDefaultComponentLossy);
    QVERIFY_SIGNAL_COUNT_WAIT(spyVehicle, 1, TestTimeout::mediumMs());
    Vehicle* vehicle = vehicleMgr->activeVehicle();
    QVERIFY(vehicle);
    QVERIFY_SIGNAL_COUNT_WAIT(spyParamsReady, 1, TestTimeout::longMs());
    ParameterManager* const paramManager = vehicle->parameterManager();
    QVERIFY(paramManager);

    // Initial load went via FTP, which only covers the autopilot. Kick 125 into a slow lossy stream load.
    paramManager->refreshAllParameters(125);
    QTRY_VERIFY_WITH_TIMEOUT(paramManager->parameterNames(125).count() >= 2, TestTimeout::mediumMs());

    // Full refresh goes via FTP and finishes long before 125 does
    QSignalSpy spyProgress(paramManager, &ParameterManager::loadProgressChanged);
    paramManager->refreshAllParameters();
    auto firstZeroIndex = [&spyProgress]() -> int {
        for (int i = 0; i < spyProgress.count(); i++) {
            if (spyProgress.at(i).at(0).toFloat() == 0.0f) {
                return i;
            }
        }
        return -1;
    };
    QTRY_VERIFY_WITH_TIMEOUT(firstZeroIndex() != -1, TestTimeout::longMs());
    const int readsAtFtpDone = _readCountForComponent(_mockLink->paramRequestReadIndexLog(), 125);

    // 125 must still be re-reading after the FTP parse, and none of that may bring the bar back
    QTRY_COMPARE_WITH_TIMEOUT(paramManager->parameterNames(125).count(), 14, TestTimeout::longMs());
    QVERIFY(_readCountForComponent(_mockLink->paramRequestReadIndexLog(), 125) > readsAtFtpDone);
    QCOMPARE(spyProgress.count(), firstZeroIndex() + 1);
}

// A post-ready stream refresh whose last missing index is given up on gets no further PARAM_VALUE. The bar must
// still return to 0 off the timeout path rather than staying stuck at the last streamed value.
void ParameterManagerTest::_refreshAllViaStreamClearsProgressAfterGiveUp()
{
    Vehicle* vehicle = _connectAndWaitForVehicle(MockConfiguration::FailMissingParamOnAllRequests);
    QVERIFY(vehicle);
    MultiVehicleManager* vehicleMgr = MultiVehicleManager::instance();
    QSignalSpy spyParamsReady(vehicleMgr, &MultiVehicleManager::parameterReadyVehicleAvailableChanged);
    expectAppMessage(QRegularExpression("was unable to retrieve the full set of parameters from vehicle"));
    _expectIndexLoadFailureWarning();
    QVERIFY_SIGNAL_WAIT(spyParamsReady, TestTimeout::longMs());
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
    ParameterManager* const paramManager = vehicle->parameterManager();
    QVERIFY(paramManager);
    QCOMPARE(paramManager->loadProgress(), 0.0);

    QSignalSpy spyProgress(paramManager, &ParameterManager::loadProgressChanged);
    paramManager->refreshAllParameters();
    QTRY_VERIFY_WITH_TIMEOUT(spyProgress.count() > 0 && spyProgress.first().at(0).toFloat() > 0.0f, TestTimeout::mediumMs());
    QTRY_COMPARE_WITH_TIMEOUT(paramManager->loadProgress(), 0.0, TestTimeout::longMs());
    QCOMPARE(spyProgress.last().at(0).toFloat(), 0.0f);
}

// Both components are missing param index 1 after the initial stream. The autopilot's index 1 is dead, the second
// component's index 1 answers re-reads. The re-read queue must track (component, index) pairs: the second component's
// index 1 must be requested in the first re-read cycle, not starved until the autopilot's index 1 is given up.
void ParameterManagerTest::_requestListSharedIndexAcrossComponents()
{
    QVERIFY2(!_mockLink, "MockLink already connected");
    _mockLink = MockLink::startPX4MockLink(MockConfiguration::OptionNone, MockConfiguration::FailMissingParamSharedIndexAcrossComponents);
    _mockLink->mockLinkFTP()->setParamPckEnabled(false);
    MultiVehicleManager* vehicleMgr = MultiVehicleManager::instance();
    QVERIFY(vehicleMgr);
    QSignalSpy spyVehicle(vehicleMgr, &MultiVehicleManager::activeVehicleAvailableChanged);
    QVERIFY_SIGNAL_WAIT(spyVehicle, TestTimeout::mediumMs());
    Vehicle* vehicle = vehicleMgr->activeVehicle();
    QVERIFY(vehicle);
    QSignalSpy spyParamsReady(vehicleMgr, &MultiVehicleManager::parameterReadyVehicleAvailableChanged);
    expectAppMessage(QRegularExpression("was unable to retrieve the full set of parameters from vehicle"));
    _expectIndexLoadFailureWarning();
    QVERIFY_SIGNAL_WAIT(spyParamsReady, TestTimeout::longMs());
    QCOMPARE(vehicle->parameterManager()->missingParameters(), true);
    QVERIFY(vehicle->parameterManager()->parameterExists(125, QStringLiteral("BATT_MONITOR")));

    constexpr int sharedIndex = 1;
    const QList<QPair<int, int>> readLog = _mockLink->paramRequestReadIndexLog();
    const qsizetype firstOtherComponentRead = readLog.indexOf(qMakePair(125, sharedIndex));
    const qsizetype lastAutopilotRead = readLog.lastIndexOf(qMakePair(static_cast<int>(MAV_COMP_ID_AUTOPILOT1), sharedIndex));
    QVERIFY(firstOtherComponentRead != -1);
    QVERIFY(lastAutopilotRead != -1);
    QVERIFY2(firstOtherComponentRead < lastAutopilotRead,
             qPrintable(QStringLiteral("Component 125 index %1 first requested at position %2, after autopilot index %1 last requested at %3")
                            .arg(sharedIndex).arg(firstOtherComponentRead).arg(lastAutopilotRead)));
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
}

void ParameterManagerTest::_paramWriteNoAckRetry()
{
    _ignoreParamResponseTimeouts();
    // BAT1_V_CHARGED requires a vehicle reboot, so writing it pops the reboot-required notice
    expectAppMessage(QRegularExpression("Vehicle reboot required for changes to take effect"));
    _setParamWithFailureMode(MockLink::FailParamSetFirstAttemptNoAck, true /* expectSuccess */,
                             QStringLiteral("BAT1_V_CHARGED"), MAV_AUTOPILOT_PX4);
    verifyExpectedLogMessage();
}

void ParameterManagerTest::_paramWriteNoAckPermanent()
{
    _ignoreParamResponseTimeouts();
    // Expectations verify in FIFO order: reboot notice first (fires at local
    // setRawValue), then the write-failed message (fires after retries exhaust)
    expectAppMessage(QRegularExpression("Vehicle reboot required for changes to take effect"));
    expectAppMessage(QRegularExpression("Parameter write failed"));
    _setParamWithFailureMode(MockLink::FailParamSetNoAck, false /* expectSuccess */,
                             QStringLiteral("BAT1_V_CHARGED"), MAV_AUTOPILOT_PX4);
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
}

void ParameterManagerTest::_paramWriteUInt8()
{
    _setParamWithFailureMode(MockLink::FailParamSetNone, true /* expectSuccess */,
                             QStringLiteral("TEST_UINT8"), MAV_AUTOPILOT_GENERIC);
}

void ParameterManagerTest::_paramWriteUInt16()
{
    _setParamWithFailureMode(MockLink::FailParamSetNone, true /* expectSuccess */,
                             QStringLiteral("TEST_UINT16"), MAV_AUTOPILOT_GENERIC);
}

void ParameterManagerTest::_paramReadFirstAttemptNoResponseRetry_data()
{
    QTest::addColumn<bool>("timedByTest");

    QTest::newRow("ack timeout on the scheduler of the test") << true;
    QTest::newRow("ack timeout on the scheduler of the manager, which it is given back") << false;
}

void ParameterManagerTest::_paramReadFirstAttemptNoResponseRetry()
{
    QFETCH(bool, timedByTest);

    _ignoreParamResponseTimeouts();
    QVERIFY2(!_mockLink, "MockLink already connected");
    _connectMockLink();
    QVERIFY(_mockLink);
    QVERIFY(_vehicle);
    ParameterManager* const paramManager = _vehicle->parameterManager();
    QVERIFY(paramManager);
    _timeRequestsByHand(_vehicle);
    if (!timedByTest) {
        paramManager->setScheduler(nullptr);
    }
    _mockLink->setParamRequestReadFailureMode(MockLink::FailParamRequestReadFirstAttemptNoResponse);
    Fact* const fact = paramManager->getParameter(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("BAT1_V_CHARGED"));
    QVERIFY(fact);
    QSignalSpy vehicleUpdatedSpy(fact, &Fact::vehicleUpdated);
    QSignalSpy paramReadSuccessSpy(paramManager, &ParameterManager::_paramRequestReadSuccess);
    QVERIFY(vehicleUpdatedSpy.isValid());
    QVERIFY(paramReadSuccessSpy.isValid());
    _mockLink->clearReceivedMavlinkMessageCounts();
    paramManager->refreshParameter(MAV_COMP_ID_AUTOPILOT1, fact->name());
    if (timedByTest) {
        _expireAckTimeout(MAVLINK_MSG_ID_PARAM_REQUEST_READ, 1);
    } else {
        // The attempt ends by itself: the scheduler of the test has nothing of it to time
        QTRY_COMPARE_WITH_TIMEOUT(_mockLink->receivedMavlinkMessageCount(MAVLINK_MSG_ID_PARAM_REQUEST_READ), 1,
                                  TestTimeout::mediumMs());
        QCOMPARE(_scheduler->pendingCount(), 0);
    }
    QVERIFY_SIGNAL_COUNT_WAIT(paramReadSuccessSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(paramReadSuccessSpy.count(), 1);
    QCOMPARE(vehicleUpdatedSpy.count(), 1);
    _disconnectMockLink();
}

void ParameterManagerTest::_paramReadNoResponse()
{
    _ignoreParamResponseTimeouts();
    QVERIFY2(!_mockLink, "MockLink already connected");
    _connectMockLink();
    QVERIFY(_mockLink);
    QVERIFY(_vehicle);
    ParameterManager* const paramManager = _vehicle->parameterManager();
    QVERIFY(paramManager);
    Fact* const fact = paramManager->getParameter(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("BAT1_V_CHARGED"));
    QVERIFY(fact);
    QSignalSpy vehicleUpdatedSpy(fact, &Fact::vehicleUpdated);
    QSignalSpy paramReadFailureSpy(paramManager, &ParameterManager::_paramRequestReadFailure);
    QVERIFY(vehicleUpdatedSpy.isValid());
    QVERIFY(paramReadFailureSpy.isValid());
    _timeRequestsByHand(_vehicle);
    _mockLink->setParamRequestReadFailureMode(MockLink::FailParamRequestReadNoResponse);
    expectAppMessage(QRegularExpression("Parameter read failed"));
    _mockLink->clearReceivedMavlinkMessageCounts();
    paramManager->refreshParameter(MAV_COMP_ID_AUTOPILOT1, fact->name());
    for (int attempt = 1; attempt <= ParameterManager::kParamRequestReadRetryCount + 1; attempt++) {
        _expireAckTimeout(MAVLINK_MSG_ID_PARAM_REQUEST_READ, attempt);
    }
    // The read fails with its last timeout
    QCOMPARE(paramReadFailureSpy.count(), 1);
    QCOMPARE(vehicleUpdatedSpy.count(), 0);
    verifyExpectedLogMessage();
    _disconnectMockLink();
}

void ParameterManagerTest::_requestStateMachineDeletedWhenFinished_data()
{
    QTest::addColumn<bool>("write");
    QTest::addColumn<bool>("vehicleResponds");

    QTest::newRow("write acked") << true << true;
    QTest::newRow("write retries exhausted") << true << false;
    QTest::newRow("read answered") << false << true;
    QTest::newRow("read retries exhausted") << false << false;
}

void ParameterManagerTest::_requestStateMachineDeletedWhenFinished()
{
    QFETCH(bool, write);
    QFETCH(bool, vehicleResponds);

    if (!vehicleResponds) {
        _ignoreParamResponseTimeouts();
    }
    QVERIFY2(!_mockLink, "MockLink already connected");
    _connectMockLink();
    QVERIFY(_mockLink);
    QVERIFY(_vehicle);
    ParameterManager* const paramManager = _vehicle->parameterManager();
    QVERIFY(paramManager);
    Fact* const fact = paramManager->getParameter(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("BAT1_V_CHARGED"));
    QVERIFY(fact);
    QVERIFY(paramManager->findChildren<QGCStateMachine*>().isEmpty());
    _timeRequestsByHand(_vehicle);
    _mockLink->clearReceivedMavlinkMessageCounts();

    if (write) {
        _mockLink->setParamSetFailureMode(vehicleResponds ? MockLink::FailParamSetNone : MockLink::FailParamSetNoAck);
        expectAppMessage(QRegularExpression("Vehicle reboot required for changes to take effect"));
        if (!vehicleResponds) {
            expectAppMessage(QRegularExpression("Parameter write failed"));
        }
        fact->setRawValue(fact->rawValue().toDouble() + 0.1);
    } else {
        _mockLink->setParamRequestReadFailureMode(vehicleResponds ? MockLink::FailParamRequestReadNone
                                                                  : MockLink::FailParamRequestReadNoResponse);
        paramManager->refreshParameter(MAV_COMP_ID_AUTOPILOT1, fact->name(), false /* notifyFailure */);
    }
    QVERIFY(!paramManager->findChildren<QGCStateMachine*>().isEmpty());
    if (!vehicleResponds) {
        const uint32_t requestMessageId = write ? MAVLINK_MSG_ID_PARAM_SET : MAVLINK_MSG_ID_PARAM_REQUEST_READ;
        const int attempts =
            (write ? ParameterManager::kParamSetRetryCount : ParameterManager::kParamRequestReadRetryCount) + 1;
        for (int attempt = 1; attempt <= attempts; attempt++) {
            _expireAckTimeout(requestMessageId, attempt);
        }
    }

    // A failed write is followed by a refresh of the parameter, which runs a machine of its own
    QVERIFY_TRUE_WAIT(paramManager->findChildren<QGCStateMachine*>().isEmpty(), TestTimeout::mediumMs());
    if (write) {
        verifyExpectedLogMessage();
        if (!vehicleResponds) {
            verifyExpectedLogMessage();
        }
    }
    _mockLink->setParamSetFailureMode(MockLink::FailParamSetNone);
    _mockLink->setParamRequestReadFailureMode(MockLink::FailParamRequestReadNone);
    _disconnectMockLink();
}

void ParameterManagerTest::_paramWriteParamError()
{
    // Expectations verify in FIFO order: reboot notice first (fires at local
    // setRawValue), then the write-failed message (fires on the PARAM_ERROR ack)
    expectAppMessage(QRegularExpression("Vehicle reboot required for changes to take effect"));
    expectAppMessage(QRegularExpression("Parameter write failed"));
    _setParamWithFailureMode(MockLink::FailParamSetParamError, false /* expectSuccess */,
                             QStringLiteral("BAT1_V_CHARGED"), MAV_AUTOPILOT_PX4);
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
}

void ParameterManagerTest::_paramReadParamError_data()
{
    QTest::addColumn<bool>("notifyFailure");

    QTest::newRow("notify") << true;
    QTest::newRow("silent") << false;
}

void ParameterManagerTest::_paramReadParamError()
{
    QFETCH(bool, notifyFailure);

    QVERIFY2(!_mockLink, "MockLink already connected");
    _connectMockLink();
    QVERIFY(_mockLink);
    QVERIFY(_vehicle);
    ParameterManager* const paramManager = _vehicle->parameterManager();
    QVERIFY(paramManager);
    _mockLink->setParamRequestReadFailureMode(MockLink::FailParamRequestReadParamError);
    Fact* const fact = paramManager->getParameter(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("BAT1_V_CHARGED"));
    QVERIFY(fact);
    QSignalSpy vehicleUpdatedSpy(fact, &Fact::vehicleUpdated);
    QSignalSpy paramReadFailureSpy(paramManager, &ParameterManager::_paramRequestReadFailure);
    QVERIFY(vehicleUpdatedSpy.isValid());
    QVERIFY(paramReadFailureSpy.isValid());
    if (notifyFailure) {
        expectAppMessage(QRegularExpression("Parameter read failed"));
    }
    paramManager->refreshParameter(MAV_COMP_ID_AUTOPILOT1, fact->name(), notifyFailure);
    // PARAM_ERROR should cause immediate failure - no retries needed, so wait just one ack interval plus buffer
    const int maxWaitTimeMs = ParameterManager::kWaitForParamValueAckMs + TestTimeout::shortMs();
    QVERIFY_SIGNAL_WAIT(paramReadFailureSpy, maxWaitTimeMs);
    QCOMPARE(paramReadFailureSpy.count(), 1);
    QCOMPARE(vehicleUpdatedSpy.count(), 0);
    if (notifyFailure) {
        verifyExpectedLogMessage();
    }
    _disconnectMockLink();
}

void ParameterManagerTest::_setParamWithFailureMode(MockLink::ParamSetFailureMode_t failureMode, bool expectSuccess,
                                                     const QString &paramName, MAV_AUTOPILOT autopilot)
{
    QVERIFY2(!_mockLink, "MockLink already connected");
    if (autopilot == MAV_AUTOPILOT_GENERIC) {
        // Generic mock link has no metadata source; this warning is expected for generic autopilot
        ignoreLogMessage("ComponentInformation.RequestMetaDataTypeStateMachine", QtWarningMsg,
                         QRegularExpression("failed to load metadata"));
    }
    // Bring up a clean mock vehicle for each run
    _connectMockLink(autopilot);
    QVERIFY(_mockLink);
    QVERIFY(_vehicle);
    _mockLink->setParamSetFailureMode(failureMode);
    MultiVehicleManager* vehicleMgr = MultiVehicleManager::instance();
    QVERIFY(vehicleMgr);
    ParameterManager* const paramManager = _vehicle->parameterManager();
    QVERIFY(paramManager);
    _timeRequestsByHand(_vehicle);
    // The attempts that the vehicle leaves unanswered
    int unansweredAttempts = 0;
    if (failureMode == MockLink::FailParamSetNoAck) {
        unansweredAttempts = ParameterManager::kParamSetRetryCount + 1;
    } else if (failureMode == MockLink::FailParamSetFirstAttemptNoAck) {
        unansweredAttempts = 1;
    }
    QVERIFY(!_vehicle->parameterManager()->pendingWrites());
    Fact* const fact = paramManager->getParameter(MAV_COMP_ID_AUTOPILOT1, paramName);
    QVERIFY(fact);
    QSignalSpy rawValueChangedSpy(fact, &Fact::rawValueChanged);
    const QVariant originalValue = fact->rawValue();
    const double originalDouble = originalValue.toDouble();
    const FactMetaData* const metaData = fact->metaData();
    const double minValue = (metaData && metaData->rawMin().isValid()) ? metaData->rawMin().toDouble()
                                                                       : -std::numeric_limits<double>::infinity();
    const double maxValue = (metaData && metaData->rawMax().isValid()) ? metaData->rawMax().toDouble()
                                                                       : std::numeric_limits<double>::infinity();
    const double step = fact->type() == FactMetaData::valueTypeFloat ? 0.1 : 1.0;
    auto adjustedValue = [&](double candidate) -> double {
        if (candidate > maxValue) {
            candidate = originalDouble - step;
        }
        if (candidate < minValue) {
            candidate = originalDouble + step;
        }
        if (qFuzzyCompare(candidate + 1.0, originalDouble + 1.0)) {
            candidate = originalDouble + (step * 2.0);
        }
        if (candidate > maxValue) {
            candidate = originalDouble - (step * 2.0);
        }
        if (candidate < minValue) {
            candidate = originalDouble;
        }
        return candidate;
    };
    const double newValueDouble = adjustedValue(originalDouble + step);
    QVERIFY(!qFuzzyCompare(newValueDouble + 1.0, originalDouble + 1.0));
    const QVariant newValue(newValueDouble);
    QSignalSpy pendingSpy(paramManager, &ParameterManager::pendingWritesChanged);
    QVERIFY(pendingSpy.isValid());
    QSignalSpy paramSetSuccessSpy(paramManager, &ParameterManager::_paramSetSuccess);
    QVERIFY(paramSetSuccessSpy.isValid());
    QSignalSpy paramSetFailureSpy(paramManager, &ParameterManager::_paramSetFailure);
    QVERIFY(paramSetFailureSpy.isValid());
    QVERIFY(!_vehicle->rebootRequired());
    _mockLink->clearReceivedMavlinkMessageCounts();
    fact->setRawValue(newValue);
    QCOMPARE(_vehicle->rebootRequired(), fact->vehicleRebootRequired());
    for (int attempt = 1; attempt <= unansweredAttempts; attempt++) {
        _expireAckTimeout(MAVLINK_MSG_ID_PARAM_SET, attempt);
    }
    // We should see pendingWrites go to true and then back to false
    bool sawPendingTrue = false;
    bool sawPendingFalse = false;
    int processedCount = 0;
    const auto processPendingSignals = [&]() {
        while (processedCount < pendingSpy.count()) {
            const QList<QVariant> arguments = pendingSpy.at(processedCount++);
            QCOMPARE(arguments.count(), 1);
            const bool isPending = arguments.at(0).toBool();
            if (isPending) {
                sawPendingTrue = true;
            } else {
                sawPendingFalse = true;
            }
        }
    };

    processPendingSignals();
    QElapsedTimer waitTimer;
    waitTimer.start();
    while ((!sawPendingTrue || !sawPendingFalse) && waitTimer.elapsed() < TestTimeout::longMs()) {
        const int remainingMs = TestTimeout::longMs() - static_cast<int>(waitTimer.elapsed());
        if (remainingMs <= 0) {
            break;
        }

        const int nextExpectedCount = processedCount + 1;
        if (!UnitTest::waitForSignalCount(pendingSpy, nextExpectedCount, remainingMs,
                                          QStringLiteral("pendingWritesChanged"))) {
            break;
        }

        processPendingSignals();
    }
    QVERIFY2(sawPendingTrue, "Expected pendingWritesChanged(true) signal");
    QVERIFY2(sawPendingFalse, "Expected pendingWritesChanged(false) signal");
    // We should get two rawValueChanged signals if unsuccessful (one for the set, one for the refresh)
    // We should get one rawValueChanged signal if successful (just the set)
    QVERIFY_SIGNAL_COUNT_WAIT(
        rawValueChangedSpy, 1,
        ParameterManager::kWaitForParamValueAckMs * ParameterManager::kParamSetRetryCount + TestTimeout::shortMs());
    const int maxSetResultWaitMs = (ParameterManager::kWaitForParamValueAckMs * ParameterManager::kParamSetRetryCount)
                                   + TestTimeout::mediumMs();
    if (expectSuccess) {
        QVERIFY_SIGNAL_COUNT_WAIT(paramSetSuccessSpy, 1, maxSetResultWaitMs);
        QCOMPARE(rawValueChangedSpy.count(), 1);
        QCOMPARE(paramSetSuccessSpy.count(), 1);
        QCOMPARE(paramSetFailureSpy.count(), 0);
    } else {
        QVERIFY_SIGNAL_COUNT_WAIT(paramSetFailureSpy, 1, maxSetResultWaitMs);
        QVERIFY(rawValueChangedSpy.count() == 1 || rawValueChangedSpy.count() == 2);
        QCOMPARE(paramSetSuccessSpy.count(), 0);
        QCOMPARE(paramSetFailureSpy.count(), 1);
    }
    // The write is pending once, from its first attempt to its result, however often PARAM_SET is sent
    QCOMPARE(pendingSpy.count(), 2);
    // The first signal is the change we made, so we start checking from there
    QVERIFY(QGC::fuzzyCompare(rawValueChangedSpy[0][0].toDouble(), newValueDouble, 0.00001));
    if (!expectSuccess) {
        // If the write failed the second signal should be the value reverting to original
        if (rawValueChangedSpy.count() == 1) {
            // Wait a bit longer for the refresh to come in
            QVERIFY_SIGNAL_COUNT_WAIT(rawValueChangedSpy, 2, TestTimeout::shortMs());
        }
        QCOMPARE(rawValueChangedSpy.count(), 2);
        QVERIFY(QGC::fuzzyCompare(rawValueChangedSpy[1][0].toDouble(), originalDouble, 0.00001));
        // We should have also alerted the user of the failure
        // Note that we can't easily check for the user notify state here due to the
        // complexity of the state machine and timing of the signals.
    }
    _mockLink->setParamSetFailureMode(MockLink::FailParamSetNone);
    _disconnectMockLink();
}

void ParameterManagerTest::_rebootRequiredNotice_data()
{
    QTest::addColumn<int>("suppressCalls");
    QTest::addColumn<int>("unsuppressCalls");
    QTest::addColumn<bool>("expectNotice");

    QTest::newRow("notify") << 0 << 0 << true;
    QTest::newRow("suppressed") << 1 << 0 << false;
    QTest::newRow("nestedStillSuppressed") << 2 << 1 << false;
    QTest::newRow("balancedNotifies") << 2 << 2 << true;
    QTest::newRow("extraUnsuppressIgnored") << 0 << 1 << true;
}

void ParameterManagerTest::_rebootRequiredNotice()
{
    QFETCH(int, suppressCalls);
    QFETCH(int, unsuppressCalls);
    QFETCH(bool, expectNotice);

    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);
    ParameterManager* const paramManager = _vehicle->parameterManager();
    QVERIFY(paramManager);
    Fact* const fact = paramManager->getParameter(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("BAT1_V_CHARGED"));
    QVERIFY(fact);
    QVERIFY(fact->vehicleRebootRequired());
    QVERIFY(!_vehicle->rebootRequired());
    QSignalSpy rebootRequiredSpy(_vehicle, &Vehicle::rebootRequiredChanged);
    QVERIFY(rebootRequiredSpy.isValid());
    QSignalSpy pendingSpy(paramManager, &ParameterManager::pendingWritesChanged);
    QVERIFY(pendingSpy.isValid());

    for (int i = 0; i < suppressCalls; i++) {
        paramManager->suppressRebootMessage();
    }
    for (int i = 0; i < unsuppressCalls; i++) {
        const bool unbalanced = i >= suppressCalls;
        if (unbalanced) {
            expectLogMessage("FactSystem.ParameterManager", QtWarningMsg,
                             QRegularExpression("unsuppressRebootMessage without matching suppressRebootMessage"));
        }
        paramManager->unsuppressRebootMessage();
        if (unbalanced) {
            verifyExpectedLogMessage();
        }
    }

    const QRegularExpression notice("Vehicle reboot required for changes to take effect");
    const double originalValue = fact->rawValue().toDouble();
    const auto writeValue = [&](double value, bool notifies) {
        if (notifies) {
            expectAppMessage(notice);
        }
        fact->setRawValue(value);
        if (notifies) {
            verifyExpectedLogMessage();
        }
    };

    writeValue(originalValue + 0.1, expectNotice);

    // A change right after the first one is debounced
    writeValue(originalValue + 0.2, false);

    // Once the debounce window has passed, a later change notifies again
    qgcApp()->resetRebootMessageDebounce();
    writeValue(originalValue + 0.3, expectNotice);

    for (int i = unsuppressCalls; i < suppressCalls; i++) {
        paramManager->unsuppressRebootMessage();
    }

    QVERIFY(_vehicle->rebootRequired());
    QCOMPARE(rebootRequiredSpy.count(), 1);
    QTRY_VERIFY_WITH_TIMEOUT(!pendingSpy.isEmpty() && !pendingSpy.last().at(0).toBool(), TestTimeout::longMs());
    _disconnectMockLink();
}

void ParameterManagerTest::_FTPnoFailure()
{
    // Test APM FTP-based parameter download (param.pck).
    // FailParamNoResponseToRequestList forces the FTP path by blocking PARAM_REQUEST_LIST.
    QVERIFY2(!_mockLink, "MockLink already connected");

    MultiVehicleManager* vehicleMgr = MultiVehicleManager::instance();
    QVERIFY(vehicleMgr);

    QSignalSpy spyVehicle(vehicleMgr, &MultiVehicleManager::activeVehicleAvailableChanged);
    QSignalSpy spyParamsReady(vehicleMgr, &MultiVehicleManager::parameterReadyVehicleAvailableChanged);
    _mockLink = MockLink::startAPMArduPlaneMockLink(MockConfiguration::OptionNone,
                                                    MockConfiguration::FailParamNoResponseToRequestList);
    QTRY_VERIFY_WITH_TIMEOUT(!spyVehicle.isEmpty(), TestTimeout::mediumMs());
    QCOMPARE(spyVehicle.count(), 1);
    QCOMPARE(spyVehicle.first().at(0).toBool(), true);

    Vehicle* vehicle = vehicleMgr->activeVehicle();
    QVERIFY(vehicle);

    QTRY_VERIFY_WITH_TIMEOUT(!spyParamsReady.isEmpty(), TestTimeout::longMs());
    QCOMPARE(spyParamsReady.takeFirst().at(0).toBool(), true);

    // Verify FTP was used and PARAM_REQUEST_LIST was not
    QVERIFY2(_mockLink->receivedMavlinkMessageCount(MAVLINK_MSG_ID_FILE_TRANSFER_PROTOCOL) > 0, "FTP messages should have been sent");
    QCOMPARE(_mockLink->receivedMavlinkMessageCount(MAVLINK_MSG_ID_PARAM_REQUEST_LIST), 0);

    // Verify parameters were loaded with correct values from param.pck
    ParameterManager* paramManager = vehicle->parameterManager();
    QVERIFY(paramManager);
    QVERIFY(paramManager->parametersReady());
    QVERIFY(paramManager->parameterExists(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("BATT_LOW_VOLT")));
    QVERIFY(paramManager->parameterExists(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("RC1_MIN")));
    QVERIFY(paramManager->parameterExists(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("BATT2_MONITOR")));

    Fact* battLowVoltFact = paramManager->getParameter(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("BATT_LOW_VOLT"));
    Fact* rc1MinFact      = paramManager->getParameter(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("RC1_MIN"));
    Fact* batt2MonFact    = paramManager->getParameter(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("BATT2_MONITOR"));

    QCOMPARE(battLowVoltFact->rawValue().toFloat(), 0.0f);
    QCOMPARE(rc1MinFact->rawValue().toInt(), 1000);
    QCOMPARE(batt2MonFact->rawValue().toInt(), 4);
}

void ParameterManagerTest::_FTPRequestsHeartbeatComponentParams_data()
{
    QTest::addColumn<MAV_AUTOPILOT>("firmwareType");

    QTest::newRow("ArduPilot") << MAV_AUTOPILOT_ARDUPILOTMEGA;
    QTest::newRow("PX4") << MAV_AUTOPILOT_PX4;
}

// param.pck only carries the autopilot's params. Other components that heartbeat must still be asked over the
// parameter protocol, without making the autopilot stream the params it just sent in the file.
void ParameterManagerTest::_FTPRequestsHeartbeatComponentParams()
{
    QFETCH(MAV_AUTOPILOT, firmwareType);
    QVERIFY2(!_mockLink, "MockLink already connected");

    MultiVehicleManager* vehicleMgr = MultiVehicleManager::instance();
    QSignalSpy spyVehicle(vehicleMgr, &MultiVehicleManager::activeVehicleAvailableChanged);
    QSignalSpy spyParamsReady(vehicleMgr, &MultiVehicleManager::parameterReadyVehicleAvailableChanged);
    _mockLink = (firmwareType == MAV_AUTOPILOT_PX4)
                    ? MockLink::startPX4MockLink(MockConfiguration::OptionCompanionParams)
                    : MockLink::startAPMArduPlaneMockLink(MockConfiguration::OptionCompanionParams);
    QVERIFY_SIGNAL_COUNT_WAIT(spyVehicle, 1, TestTimeout::mediumMs());
    QVERIFY_SIGNAL_COUNT_WAIT(spyParamsReady, 1, TestTimeout::longMs());
    QVERIFY(_mockLink->receivedMavlinkMessageCount(MAVLINK_MSG_ID_FILE_TRANSFER_PROTOCOL) > 0);

    Vehicle* const vehicle = vehicleMgr->activeVehicle();
    QVERIFY(vehicle);
    ParameterManager* const paramManager = vehicle->parameterManager();
    QVERIFY(paramManager);
    QTRY_COMPARE_WITH_TIMEOUT(paramManager->parameterNames(MAV_COMP_ID_ONBOARD_COMPUTER).count(), 3,
                              TestTimeout::longMs());

    const QList<int> requestListTargets = _mockLink->paramRequestListTargetLog();
    QVERIFY(!requestListTargets.contains(MAV_COMP_ID_AUTOPILOT1));
    QVERIFY(!requestListTargets.contains(MAV_COMP_ID_ALL));
}

// The param.pck download may wait behind another transfer of the vehicle. A wait longer than the request-list timer
// is no failed request: the download keeps its one retry over FTP, rather than going on with the parameter stream.
void ParameterManagerTest::_FTPWaitInQueueKeepsRetry()
{
    QVERIFY2(!_mockLink, "MockLink already connected");

    // Queued as the vehicle appears, before the parameter manager asks for param.pck: a large file, slowly
    MultiVehicleManager* const vehicleMgr = MultiVehicleManager::instance();
    QObject owner;
    QPointer<FTPRequest> blocker;
    (void) connect(vehicleMgr, &MultiVehicleManager::vehicleAdded, &owner, [&owner, &blocker](Vehicle* vehicle) {
        if (vehicle && !blocker) {
            blocker = vehicle->ftpManager()->download(
                &owner, MAV_COMP_ID_AUTOPILOT1,
                QStringLiteral("%1%2").arg(MockLinkFTP::sizeFilenamePrefix).arg(1024 * 1024),
                QStandardPaths::writableLocation(QStandardPaths::TempLocation), {},
                [](const QString&, const QString&) {});
        }
    });
    // The mock's blocking burst delay also starves unrelated commands sent to it during the download
    ignoreLogMessage("Vehicle.StandardModes", QtWarningMsg, QRegularExpression("Failed to retrieve available modes"));

    // The parameter stream never answers, so a fall back to it would never load the parameters
    QSignalSpy spyParamsReady(vehicleMgr, &MultiVehicleManager::parameterReadyVehicleAvailableChanged);
    _mockLink = MockLink::startAPMArduPlaneMockLink(MockConfiguration::OptionNone,
                                                    MockConfiguration::FailParamNoResponseToRequestList);
    MockLinkFTP* const mockFtp = _mockLink->mockLinkFTP();
    mockFtp->setBurstReadDelayMs(50);
    QVERIFY_TRUE_WAIT(
        blocker && vehicleMgr->activeVehicle() && (vehicleMgr->activeVehicle()->ftpManager()->requestCount() == 2),
        TestTimeout::longMs());
    FTPManager* const ftpManager = vehicleMgr->activeVehicle()->ftpManager();

    // Twice the request-list timer in the queue. A timer that ran while the download waited would have used up the
    // retry, which needs this many milliseconds to show.
    QVERIFY_NO_SIGNAL_WAIT(spyParamsReady, 2 * ParameterManager::kTestInitialRequestIntervalMs);

    // The first download of param.pck fails, the retry succeeds
    mockFtp->setErrorMode(MockLinkFTP::errModeNakResponse);
    mockFtp->setBurstReadDelayMs(0);
    blocker->cancel();
    QVERIFY_TRUE_WAIT(ftpManager->requestCount() == 0, TestTimeout::mediumMs());
    mockFtp->setErrorMode(MockLinkFTP::errModeNone);

    QVERIFY_SIGNAL_COUNT_WAIT(spyParamsReady, 1, TestTimeout::longMs());
    QVERIFY(spyParamsReady.first().at(0).toBool());
    QCOMPARE(_mockLink->receivedMavlinkMessageCount(MAVLINK_MSG_ID_PARAM_REQUEST_LIST), 0);
}

// Each vehicle has a queue of its own, so the parameter file downloads of two vehicles run at once. Each is written
// into a directory of the vehicle's own: the end of one, here its vehicle going away during its download, leaves the
// file of the other alone.
void ParameterManagerTest::_FTPDownloadsOfTwoVehiclesStayApart()
{
    QVERIFY2(!_mockLink, "MockLink already connected");
    MultiVehicleManager* const vehicleMgr = MultiVehicleManager::instance();
    const QDir tempDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation));
    const auto downloadsLeft = [&tempDir]() {
        return tempDir.entryList({QStringLiteral("qgc_params_*")}, QDir::Dirs | QDir::NoDotAndDotDot);
    };
    const QString paramFilePath = QStringLiteral("@PARAM/param.pck?withdefaults=1");

    // The first vehicle sends a parameter file of 4 MiB, a burst every 50 ms: its download runs for minutes, and is
    // ended only by its vehicle going away
    QTemporaryDir servedDir;
    QVERIFY(servedDir.isValid());
    const QString largeFile = servedDir.filePath(QStringLiteral("param.pck"));
    QVERIFY(TestFixtures::writeFile(largeFile, QByteArray(4 * 1024 * 1024, '\0')));
    QSignalSpy spyVehicles(vehicleMgr, &MultiVehicleManager::vehicleAdded);
    MockLink* const leavingLink = MockLink::startAPMArduPlaneMockLink();
    leavingLink->mockLinkFTP()->setServedFile(paramFilePath, largeFile);
    leavingLink->mockLinkFTP()->setBurstReadDelayMs(50);
    QVERIFY_SIGNAL_COUNT_WAIT(spyVehicles, 1, TestTimeout::mediumMs());
    const QPointer<Vehicle> leavingVehicle = spyVehicles.takeFirst().at(0).value<Vehicle*>();
    QVERIFY(leavingVehicle);
    QVERIFY_TRUE_WAIT(leavingLink->mockLinkFTP()->lastBurstReadRequestSize() >= 0, TestTimeout::longMs());

    // The parameter stream of the second vehicle never answers, so only its file can load its parameters. Its file is
    // sent slowly too, so that its download spans the end of the first vehicle (about two seconds)
    expectAppMessage(QRegularExpression(QStringLiteral("Connected to Vehicle [0-9]+")));
    _mockLink = MockLink::startAPMArduPlaneMockLink(MockConfiguration::OptionNone,
                                                    MockConfiguration::FailParamNoResponseToRequestList);
    _mockLink->setInt32ParamValue(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("RC1_MIN"), 1111);
    _mockLink->mockLinkFTP()->setBurstReadDelayMs(80);
    QVERIFY_SIGNAL_COUNT_WAIT(spyVehicles, 1, TestTimeout::mediumMs());
    Vehicle* const vehicle = spyVehicles.first().at(0).value<Vehicle*>();
    QVERIFY(vehicle);
    QVERIFY_TRUE_WAIT(_mockLink->mockLinkFTP()->lastBurstReadRequestSize() >= 0, TestTimeout::longMs());
    verifyExpectedLogMessage();

    // Both downloads have their file open
    QVERIFY(leavingVehicle);
    QCOMPARE(leavingVehicle->ftpManager()->requestCount(), 1);
    QVERIFY(!leavingVehicle->parameterManager()->parametersReady());
    QCOMPARE(vehicle->ftpManager()->requestCount(), 1);

    leavingLink->disconnect();
    QVERIFY_TRUE_WAIT(!leavingVehicle, TestTimeout::mediumMs());
    // The download of the second vehicle has outlasted the first vehicle
    ParameterManager* const parameterManager = vehicle->parameterManager();
    QVERIFY(!parameterManager->parametersReady());
    QCOMPARE(vehicle->ftpManager()->requestCount(), 1);
    _mockLink->mockLinkFTP()->setBurstReadDelayMs(0);

    QVERIFY_TRUE_WAIT(parameterManager->parametersReady() ||
                          (_mockLink->receivedMavlinkMessageCount(MAVLINK_MSG_ID_PARAM_REQUEST_LIST) > 0),
                      TestTimeout::longMs());
    QCOMPARE(_mockLink->receivedMavlinkMessageCount(MAVLINK_MSG_ID_PARAM_REQUEST_LIST), 0);
    QCOMPARE(parameterManager->getParameter(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("RC1_MIN"))->rawValue().toInt(),
             1111);

    // The parsed file is removed at once, the directory with its vehicle
    QCOMPARE(downloadsLeft().count(), 1);
    QVERIFY(QDir(tempDir.filePath(downloadsLeft().constFirst())).isEmpty());
    _mockLink->disconnect();
    _mockLink = nullptr;
    QVERIFY_TRUE_WAIT(downloadsLeft().isEmpty(), TestTimeout::mediumMs());
}

// Without a directory to download the parameter file into, the parameters load over the parameter stream, whose
// values the manager drops while it tries FTP
void ParameterManagerTest::_FTPWithoutDownloadDirectoryLoadsOverStream()
{
    QVERIFY2(!_mockLink, "MockLink already connected");

    // A temporary location below a file: no directory can be made there, also by root
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString notADirectory = tempDir.filePath(QStringLiteral("file"));
    QVERIFY(TestFixtures::writeFile(notADirectory, QByteArray()));

    // Unix reads TMPDIR for the temporary location, Windows TMP and then TEMP
    const TestFixtures::EnvVarFixture restoreTmpdir("TMPDIR");
    const TestFixtures::EnvVarFixture restoreTmp("TMP");
    const TestFixtures::EnvVarFixture restoreTemp("TEMP");
    for (const char* name : {"TMPDIR", "TMP", "TEMP"}) {
        qputenv(name, QFile::encodeName(QDir::toNativeSeparators(notADirectory + QStringLiteral("/tmp"))));
    }
    QVERIFY2(
        QStandardPaths::writableLocation(QStandardPaths::TempLocation).startsWith(notADirectory, Qt::CaseInsensitive),
        qPrintable(QStandardPaths::writableLocation(QStandardPaths::TempLocation)));

    MultiVehicleManager* const vehicleMgr = MultiVehicleManager::instance();
    QSignalSpy spyParamsReady(vehicleMgr, &MultiVehicleManager::parameterReadyVehicleAvailableChanged);
    expectLogMessage("FactSystem.ParameterManager", QtWarningMsg,
                     QRegularExpression("Cannot create a download directory, loading parameters without FTP"));
    _mockLink = MockLink::startAPMArduPlaneMockLink();
    QVERIFY_SIGNAL_COUNT_WAIT(spyParamsReady, 1, TestTimeout::longMs());
    QVERIFY(spyParamsReady.first().at(0).toBool());
    verifyExpectedLogMessage();
    QCOMPARE_GT(_mockLink->receivedMavlinkMessageCount(MAVLINK_MSG_ID_PARAM_REQUEST_LIST), 0);
    QVERIFY(vehicleMgr->activeVehicle());
    QVERIFY(vehicleMgr->activeVehicle()->parameterManager()->parameterExists(MAV_COMP_ID_AUTOPILOT1,
                                                                             QStringLiteral("RC1_MIN")));
}

void ParameterManagerTest::_FTPChangeParam()
{
    // Test that parameter set works after APM FTP param download
    QVERIFY2(!_mockLink, "MockLink already connected");

    MultiVehicleManager* vehicleMgr = MultiVehicleManager::instance();
    QVERIFY(vehicleMgr);

    QSignalSpy spyVehicle(vehicleMgr, &MultiVehicleManager::activeVehicleAvailableChanged);
    QSignalSpy spyParamsReady(vehicleMgr, &MultiVehicleManager::parameterReadyVehicleAvailableChanged);
    _mockLink = MockLink::startAPMArduPlaneMockLink(MockConfiguration::OptionNone,
                                                    MockConfiguration::FailParamNoResponseToRequestList);
    QTRY_VERIFY_WITH_TIMEOUT(!spyVehicle.isEmpty(), TestTimeout::mediumMs());
    QCOMPARE(spyVehicle.takeFirst().at(0).toBool(), true);

    Vehicle* vehicle = vehicleMgr->activeVehicle();
    QVERIFY(vehicle);

    QTRY_VERIFY_WITH_TIMEOUT(!spyParamsReady.isEmpty(), TestTimeout::longMs());
    QCOMPARE(spyParamsReady.takeFirst().at(0).toBool(), true);

    ParameterManager* paramManager = vehicle->parameterManager();
    QVERIFY(paramManager);

    // Change a float parameter and verify it round-trips
    Fact* fact = paramManager->getParameter(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("BATT_LOW_VOLT"));
    QVERIFY(fact);
    const QVariant originalValue = fact->rawValue();
    const float testValue = originalValue.toFloat() + 1.5f;

    QSignalSpy spyValueChanged(fact, &Fact::vehicleUpdated);
    fact->setRawValue(QVariant(testValue));
    QVERIFY_SIGNAL_WAIT(spyValueChanged, TestTimeout::mediumMs());
    QCOMPARE(fact->rawValue().toFloat(), testValue);
}

UT_REGISTER_TEST(ParameterManagerTest, TestLabel::Integration, TestLabel::Vehicle, TestLabel::Serial)

// ---------------------------------------------------------------------------
// bulkRefresh tests
// ---------------------------------------------------------------------------

// Two exact param names — both should resolve and succeed on round 0.
void ParameterManagerTest::_bulkRefreshExactNamesAllSucceed()
{
    _connectMockLink();
    QVERIFY(_vehicle);
    ParameterManager* const paramManager = _vehicle->parameterManager();
    QVERIFY(paramManager);

    QSignalSpy successSpy(paramManager, &ParameterManager::_paramRequestReadSuccess);
    QSignalSpy failureSpy(paramManager, &ParameterManager::_paramRequestReadFailure);
    QVERIFY(successSpy.isValid());
    QVERIFY(failureSpy.isValid());

    paramManager->bulkRefresh(MAV_COMP_ID_AUTOPILOT1,
                               {QStringLiteral("BAT1_V_CHARGED"), QStringLiteral("BAT1_N_CELLS")});

    const int maxWaitMs = ParameterManager::kWaitForParamValueAckMs
                          * (ParameterManager::kParamRequestReadRetryCount + 1)
                          + TestTimeout::shortMs();
    QVERIFY_SIGNAL_COUNT_WAIT(successSpy, 2, maxWaitMs);
    QCOMPARE(failureSpy.count(), 0);

    QStringList succeededNames;
    for (int i = 0; i < successSpy.count(); ++i) {
        succeededNames << successSpy.at(i).at(1).toString();
    }
    QVERIFY(succeededNames.contains(QStringLiteral("BAT1_V_CHARGED")));
    QVERIFY(succeededNames.contains(QStringLiteral("BAT1_N_CELLS")));

    _disconnectMockLink();
}

// A wildcard prefix "BAT1_*" should expand to all BAT1_ parameters and all should succeed.
void ParameterManagerTest::_bulkRefreshPrefixExpansion()
{
    _connectMockLink();
    QVERIFY(_vehicle);
    ParameterManager* const paramManager = _vehicle->parameterManager();
    QVERIFY(paramManager);

    // Count BAT1_ params actually present on the mock vehicle.
    int bat1Count = 0;
    for (const QString &name : paramManager->parameterNames(MAV_COMP_ID_AUTOPILOT1)) {
        if (name.startsWith(QStringLiteral("BAT1_"))) {
            ++bat1Count;
        }
    }
    QVERIFY2(bat1Count > 0, "Mock link must have at least one BAT1_ parameter");

    QSignalSpy successSpy(paramManager, &ParameterManager::_paramRequestReadSuccess);
    QSignalSpy failureSpy(paramManager, &ParameterManager::_paramRequestReadFailure);
    QVERIFY(successSpy.isValid());
    QVERIFY(failureSpy.isValid());

    paramManager->bulkRefresh(MAV_COMP_ID_AUTOPILOT1, {QStringLiteral("BAT1_*")});

    const int maxWaitMs = ParameterManager::kWaitForParamValueAckMs
                          * (ParameterManager::kParamRequestReadRetryCount + 1)
                          + TestTimeout::shortMs();
    QVERIFY_SIGNAL_COUNT_WAIT(successSpy, bat1Count, maxWaitMs);
    QCOMPARE(failureSpy.count(), 0);
    QCOMPARE(successSpy.count(), bat1Count);

    _disconnectMockLink();
}

// Passing only non-existent names should resolve to an empty set — no requests are sent.
void ParameterManagerTest::_bulkRefreshUnknownNameSkipped()
{
    _connectMockLink();
    QVERIFY(_vehicle);
    ParameterManager* const paramManager = _vehicle->parameterManager();
    QVERIFY(paramManager);

    QSignalSpy successSpy(paramManager, &ParameterManager::_paramRequestReadSuccess);
    QSignalSpy failureSpy(paramManager, &ParameterManager::_paramRequestReadFailure);
    QVERIFY(successSpy.isValid());
    QVERIFY(failureSpy.isValid());

    expectLogMessage("FactSystem.ParameterManager", QtWarningMsg, QRegularExpression("bulkRefresh: unknown param name \\(skipped\\):.*ZZZZ_DOES_NOT_EXIST"));
    paramManager->bulkRefresh(MAV_COMP_ID_AUTOPILOT1, {QStringLiteral("ZZZZ_DOES_NOT_EXIST")});
    verifyExpectedLogMessage();

    // bulkRefresh starts its job synchronously, so no job means no request can follow
    QVERIFY(paramManager->findChildren<BulkRefreshJob*>(Qt::FindDirectChildrenOnly).isEmpty());
    QCOMPARE(successSpy.count(), 0);
    QCOMPARE(failureSpy.count(), 0);

    _disconnectMockLink();
}

// Round 0 fails (no response from MockLink), round 1 succeeds after failure mode is cleared.
void ParameterManagerTest::_bulkRefreshRetrySucceeds()
{
    _ignoreParamResponseTimeouts();
    _connectMockLink();
    QVERIFY(_mockLink);
    QVERIFY(_vehicle);
    ParameterManager* const paramManager = _vehicle->parameterManager();
    QVERIFY(paramManager);

    QSignalSpy successSpy(paramManager, &ParameterManager::_paramRequestReadSuccess);
    QSignalSpy failureSpy(paramManager, &ParameterManager::_paramRequestReadFailure);
    QVERIFY(successSpy.isValid());
    QVERIFY(failureSpy.isValid());

    // Round 0: MockLink drops all responses — per-param SM exhausts retries → _paramRequestReadFailure
    _timeRequestsByHand(_vehicle);
    _mockLink->setParamRequestReadFailureMode(MockLink::FailParamRequestReadNoResponse);
    _mockLink->clearReceivedMavlinkMessageCounts();
    paramManager->bulkRefresh(MAV_COMP_ID_AUTOPILOT1, {QStringLiteral("BAT1_V_CHARGED")});
    for (int attempt = 1; attempt <= ParameterManager::kParamRequestReadRetryCount + 1; attempt++) {
        _expireAckTimeout(MAVLINK_MSG_ID_PARAM_REQUEST_READ, attempt);
    }
    QCOMPARE(failureSpy.count(), 1);

    // Allow BulkRefreshJob's retry round, which it starts after its delay, to succeed
    _mockLink->setParamRequestReadFailureMode(MockLink::FailParamRequestReadNone);
    QCOMPARE(_scheduler->pendingCount(), 1);
    QVERIFY(_scheduler->advanceBy(std::chrono::milliseconds(BulkRefreshJob::kRetryBaseDelayMs)));
    QVERIFY_SIGNAL_COUNT_WAIT(successSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(successSpy.count(), 1);
    QCOMPARE(successSpy.at(0).at(1).toString(), QStringLiteral("BAT1_V_CHARGED"));

    _disconnectMockLink();
}

// All kMaxRetryRounds+1 rounds fail — BulkRefreshJob gives up without a success signal.
void ParameterManagerTest::_bulkRefreshAllRetriesExhausted()
{
    _ignoreParamResponseTimeouts();
    _connectMockLink();
    QVERIFY(_mockLink);
    QVERIFY(_vehicle);
    ParameterManager* const paramManager = _vehicle->parameterManager();
    QVERIFY(paramManager);

    QSignalSpy successSpy(paramManager, &ParameterManager::_paramRequestReadSuccess);
    QSignalSpy failureSpy(paramManager, &ParameterManager::_paramRequestReadFailure);
    QVERIFY(successSpy.isValid());
    QVERIFY(failureSpy.isValid());

    _timeRequestsByHand(_vehicle);
    _mockLink->setParamRequestReadFailureMode(MockLink::FailParamRequestReadNoResponse);
    expectAppMessage(QRegularExpression("Parameter refresh failed"));
    _mockLink->clearReceivedMavlinkMessageCounts();
    paramManager->bulkRefresh(MAV_COMP_ID_AUTOPILOT1, {QStringLiteral("BAT1_V_CHARGED")});

    // Each round exhausts the per-param SM retries. Between the rounds the job waits for kRetryBaseDelayMs, doubled
    // each round.
    const int attemptsPerRound = ParameterManager::kParamRequestReadRetryCount + 1;
    for (int round = 0; round <= BulkRefreshJob::kMaxRetryRounds; round++) {
        for (int attempt = 1; attempt <= attemptsPerRound; attempt++) {
            _expireAckTimeout(MAVLINK_MSG_ID_PARAM_REQUEST_READ, round * attemptsPerRound + attempt);
        }
        QCOMPARE(failureSpy.count(), round + 1);
        if (round < BulkRefreshJob::kMaxRetryRounds) {
            QCOMPARE(_scheduler->pendingCount(), 1);
            QVERIFY(_scheduler->advanceBy(std::chrono::milliseconds(BulkRefreshJob::kRetryBaseDelayMs << round)));
        }
    }
    QCOMPARE(successSpy.count(), 0);
    verifyExpectedLogMessage();

    _disconnectMockLink();
}
