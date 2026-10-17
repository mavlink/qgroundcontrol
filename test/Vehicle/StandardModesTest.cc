#include "StandardModesTest.h"

#include <QtCore/QRegularExpression>
#include <QtTest/QSignalSpy>

#include "MAVLinkLib.h"
#include "MockLinkWorker.h"
#include "QGCMAVLink.h"
#include "StandardModes.h"
#include "Vehicle.h"

void StandardModesTest::_monitorSequenceBumpTriggersRequery()
{
    // Initial connect has completed, so the initial AVAILABLE_MODES enumeration has settled.
    // Assert on the re-query itself rather than on flightModes(), which a FirmwarePlugin can
    // filter (e.g. custom builds hide modes that cannot be set by the user).
    const int baselineRequests = _mockLink->receivedRequestMessageCount(MAVLINK_MSG_ID_AVAILABLE_MODES);
    QVERIFY(baselineRequests > 0);

    // Bumping the sequence number changes the 1Hz AVAILABLE_MODES_MONITOR, which must cause
    // StandardModes to re-query the mode list.
    _mockLink->bumpAvailableModesMonitorSequence();

    QTRY_VERIFY_WITH_TIMEOUT(
        _mockLink->receivedRequestMessageCount(MAVLINK_MSG_ID_AVAILABLE_MODES) > baselineRequests,
        TestTimeout::longMs());
}

void StandardModesTest::_monitorWithDownloadedSequenceDoesNotRequery()
{
    QVERIFY(_mockLink);
    QVERIFY(_mockLink->_worker);

    // Deliver AVAILABLE_MODES_MONITOR from the test instead of the 1Hz telemetry.
    QVERIFY(QMetaObject::invokeMethod(_mockLink->_worker, &MockLinkWorker::stopWork, Qt::BlockingQueuedConnection));

    auto* connectedVehicle = vehicle();
    QVERIFY(connectedVehicle);

    auto* standardModes = connectedVehicle->findChild<StandardModes*>();
    QVERIFY(standardModes);

    // AVAILABLE_MODES now reports seq 1; MAVLink specifies that seq == 0 is ignored.
    _mockLink->bumpAvailableModesMonitorSequence();

    _mockLink->clearReceivedRequestMessageCounts();

    QSignalSpy completedSpy(standardModes, &StandardModes::requestCompleted);
    standardModes->request();

    QVERIFY(completedSpy.wait(TestTimeout::longMs()));

    const int enumerationRequests = _mockLink->receivedRequestMessageCount(MAVLINK_MSG_ID_AVAILABLE_MODES);

    QVERIFY(enumerationRequests >= 3);

    // A monitor with the downloaded seq must not start another download. If it does, the request below
    // restarts it and the enumeration takes more requests.
    _mockLink->clearReceivedRequestMessageCounts();
    standardModes->availableModesMonitorReceived(1);
    standardModes->request();

    QVERIFY(completedSpy.wait(TestTimeout::longMs()));

    QCOMPARE(_mockLink->receivedRequestMessageCount(MAVLINK_MSG_ID_AVAILABLE_MODES), enumerationRequests);
}

void StandardModesTest::_sequenceChangeDuringDownloadRestartsRequest()
{
    QVERIFY(_mockLink);
    QVERIFY(_mockLink->_worker);

    // Prevent AVAILABLE_MODES_MONITOR telemetry from causing the restart.
    QVERIFY(QMetaObject::invokeMethod(_mockLink->_worker, &MockLinkWorker::stopWork, Qt::BlockingQueuedConnection));

    auto* connectedVehicle = vehicle();
    QVERIFY(connectedVehicle);

    auto* standardModes = connectedVehicle->findChild<StandardModes*>();
    QVERIFY(standardModes);

    // Use a non-zero generation since MAVLink specifies that seq == 0 is ignored.
    _mockLink->bumpAvailableModesMonitorSequence();

    // First measure a normal enumeration.
    _mockLink->clearReceivedRequestMessageCounts();

    QSignalSpy baselineCompletedSpy(standardModes, &StandardModes::requestCompleted);
    standardModes->request();

    QVERIFY(baselineCompletedSpy.wait(TestTimeout::longMs()));

    const int baselineRequests = _mockLink->receivedRequestMessageCount(MAVLINK_MSG_ID_AVAILABLE_MODES);

    QVERIFY(baselineRequests >= 3);

    // Change generation before the third response is sent.
    _mockLink->clearReceivedRequestMessageCounts();
    _mockLink->setAvailableModesSequenceBumpAtIndex(3);

    QSignalSpy changedCompletedSpy(standardModes, &StandardModes::requestCompleted);
    standardModes->request();

    QVERIFY(changedCompletedSpy.wait(TestTimeout::longMs()));

    const int changedRequests = _mockLink->receivedRequestMessageCount(MAVLINK_MSG_ID_AVAILABLE_MODES);

    QVERIFY2(changedRequests > baselineRequests,
             qPrintable(QStringLiteral("AVAILABLE_MODES seq changed during download, but enumeration was not restarted "
                                       "(baseline=%1 changed=%2)")
                            .arg(baselineRequests)
                            .arg(changedRequests)));
}

void StandardModesTest::_repeatedSequenceChangesStillComplete()
{
    QVERIFY(_mockLink);
    QVERIFY(_mockLink->_worker);

    // Prevent AVAILABLE_MODES_MONITOR telemetry from independently restarting the request.
    QVERIFY(QMetaObject::invokeMethod(_mockLink->_worker, &MockLinkWorker::stopWork, Qt::BlockingQueuedConnection));

    auto* connectedVehicle = vehicle();
    QVERIFY(connectedVehicle);

    auto* standardModes = connectedVehicle->findChild<StandardModes*>();
    QVERIFY(standardModes);

    // MAVLink specifies that seq == 0 is ignored.
    _mockLink->bumpAvailableModesMonitorSequence();

    // Force the generation to change at index 3 on every enumeration.
    _mockLink->setAvailableModesSequenceBumpAtIndex(3, true);

    _mockLink->clearReceivedRequestMessageCounts();

    QSignalSpy completedSpy(standardModes, &StandardModes::requestCompleted);
    QSignalSpy modesUpdatedSpy(standardModes, &StandardModes::modesUpdated);

    expectLogMessage(
        "Vehicle.StandardModes", QtWarningMsg,
        QRegularExpression(QStringLiteral("AVAILABLE_MODES sequence kept changing; giving up after 3 restarts")));

    standardModes->request();

    const bool completedWhileChurning = completedSpy.wait(TestTimeout::shortMs());

    verifyExpectedLogMessage();

    const int requestCount = _mockLink->receivedRequestMessageCount(MAVLINK_MSG_ID_AVAILABLE_MODES);

    // Always disable persistent churn before asserting so a RED failure cannot
    // leave requests continuously restarting in subsequent tests.
    _mockLink->setAvailableModesSequenceBumpAtIndex(0, false);

    if (!completedWhileChurning) {
        completedSpy.wait(TestTimeout::longMs());
    }

    QVERIFY2(completedWhileChurning, "Repeated AVAILABLE_MODES sequence changes prevented requestCompleted");

    QVERIFY2(requestCount > 3 && requestCount < 100,
             qPrintable(QStringLiteral("Repeated sequence changes were not bounded as expected "
                                       "(requestCount=%1)")
                            .arg(requestCount)));

    QCOMPARE(modesUpdatedSpy.count(), 0);
}

void StandardModesTest::_singleModeDoesNotDependOnPeriodicTelemetry()
{
    QVERIFY(_mockLink);
    QVERIFY(_mockLink->_worker);
    QVERIFY(QMetaObject::invokeMethod(_mockLink->_worker, &MockLinkWorker::stopWork, Qt::BlockingQueuedConnection));
    auto* connectedVehicle = vehicle();
    QVERIFY(connectedVehicle);
    _singleModeReceived = false;
    _singleModeValid = false;
    connectedVehicle->requestMessage(
        [](void* context, MAV_RESULT result, VehicleTypes::RequestMessageResultHandlerFailureCode_t failure,
           const mavlink_message_t& message) {
            auto* test = static_cast<StandardModesTest*>(context);
            mavlink_available_modes_t mode{};
            if (message.msgid == MAVLINK_MSG_ID_AVAILABLE_MODES) {
                mavlink_msg_available_modes_decode(&message, &mode);
            }
            test->_singleModeValid = result == MAV_RESULT_ACCEPTED &&
                                     failure == VehicleTypes::RequestMessageNoFailure &&
                                     message.msgid == MAVLINK_MSG_ID_AVAILABLE_MODES && mode.mode_index == 1;
            test->_singleModeReceived = true;
        },
        this, MAV_COMP_ID_AUTOPILOT1, MAVLINK_MSG_ID_AVAILABLE_MODES, 1);
    QTRY_VERIFY_WITH_TIMEOUT(_singleModeReceived, TestTimeout::shortMs());
    QVERIFY(_singleModeValid);
}

void StandardModesTest::_duplicateDeliveryDoesNotDuplicateModes()
{
    _mockLink->setDuplicateResponses(true);

    QSignalSpy flightModesSpy(_vehicle, &Vehicle::flightModesChanged);
    _mockLink->bumpAvailableModesMonitorSequence();
    QVERIFY(flightModesSpy.wait(TestTimeout::longMs()));

    // ensureUniqueModeNames renames duplicate entries to "<name> (N)"
    static const QRegularExpression renamedDuplicate(QStringLiteral(" \\(\\d+\\)$"));
    for (const QString& mode : _vehicle->flightModes()) {
        QVERIFY2(!renamedDuplicate.match(mode).hasMatch(), qPrintable(mode));
    }
    QVERIFY2(!renamedDuplicate.match(_vehicle->flightMode()).hasMatch(), qPrintable(_vehicle->flightMode()));
}

UT_REGISTER_TEST(StandardModesTest, TestLabel::Integration, TestLabel::Vehicle)
