#include "StandardModesTest.h"

#include <QtCore/QRegularExpression>
#include <QtTest/QSignalSpy>

#include "MAVLinkLib.h"
#include "MockLinkWorker.h"
#include "QGCMAVLink.h"
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
