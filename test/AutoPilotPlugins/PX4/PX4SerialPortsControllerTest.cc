#include "PX4SerialPortsControllerTest.h"

#include "AutoPilotPlugin.h"
#include "Fact.h"
#include "PX4SerialPortsController.h"
#include "ParameterManager.h"
#include "QmlObjectListModel.h"
#include "Vehicle.h"
#include "VehicleComponent.h"

namespace {

constexpr int kTelem1 = 101;
constexpr int kTelem2 = 102;
constexpr int kTelem3 = 103;
constexpr int kGps1 = 201;
constexpr int kRc = 300;
constexpr int kEthernet = 1000;

}  // namespace

PX4SerialPortRow* PX4SerialPortsControllerTest::_row(const PX4SerialPortsController& controller, int portId)
{
    for (int i = 0; i < controller.ports()->count(); i++) {
        PX4SerialPortRow* const row = controller.ports()->value<PX4SerialPortRow*>(i);
        if (row->portId() == portId) {
            return row;
        }
    }
    return nullptr;
}

QStringList PX4SerialPortsControllerTest::_optionNames(const PX4SerialPortRow* row)
{
    QStringList names;
    for (const QVariant& optionFact : row->optionFacts()) {
        names.append(optionFact.value<Fact*>()->name());
    }
    return names;
}

bool PX4SerialPortsControllerTest::_waitForRowsLoaded(const PX4SerialPortsController& controller)
{
    const auto rowsLoaded = [&controller]() {
        for (int i = 0; i < controller.ports()->count(); i++) {
            const PX4SerialPortRow* const row = controller.ports()->value<PX4SerialPortRow*>(i);
            if (row->baudLoading() || row->optionsLoading()) {
                return false;
            }
        }
        return true;
    };

    return UnitTest::waitForCondition(rowsLoaded, TestTimeout::mediumMs(), QStringLiteral("port rows loaded"));
}

bool PX4SerialPortsControllerTest::_waitForMockValue(const QString& paramName, int value)
{
    return UnitTest::waitForCondition(
        [this, &paramName, value]() {
            return mockLink()->paramValue(MAV_COMP_ID_AUTOPILOT1, paramName).toInt() == value;
        },
        TestTimeout::mediumMs(), QStringLiteral("%1 == %2 on vehicle").arg(paramName).arg(value));
}

void PX4SerialPortsControllerTest::_pageShownForVehicleWithPortFunctions()
{
    bool found = false;
    for (const QVariant& component : vehicle()->autopilotPlugin()->vehicleComponents()) {
        found = found || (component.value<VehicleComponent*>()->name() == QStringLiteral("Ports"));
    }
    QVERIFY(found);
}

void PX4SerialPortsControllerTest::_rowsShowVehicleAssignments()
{
    PX4SerialPortsController controller;
    QVERIFY(_waitForRowsLoaded(controller));

    const PX4SerialPortRow* const telem1 = _row(controller, kTelem1);
    QVERIFY(telem1);
    QCOMPARE(telem1->functionLabels().value(telem1->functionIndex()), QStringLiteral("MAVLink (instance 0)"));
    QVERIFY(telem1->baudFact());
    QCOMPARE(telem1->baudFact()->name(), QStringLiteral("SER_TEL1_BAUD"));
    QCOMPARE(_optionNames(telem1), (QStringList{QStringLiteral("MAV_0_MODE"), QStringLiteral("MAV_0_RATE"),
                                                QStringLiteral("MAV_0_FORWARD"), QStringLiteral("MAV_0_FLOW_CTRL")}));
    QVERIFY(telem1->conflictText().isEmpty());

    const PX4SerialPortRow* const gps1 = _row(controller, kGps1);
    QVERIFY(gps1);
    QCOMPARE(gps1->functionLabels().value(gps1->functionIndex()), QStringLiteral("Main GPS"));

    const PX4SerialPortRow* const telem2 = _row(controller, kTelem2);
    QVERIFY(telem2);
    QCOMPARE(telem2->functionIndex(), 0);

    const PX4SerialPortRow* const ethernet = _row(controller, kEthernet);
    QVERIFY(ethernet);
    QVERIFY(!ethernet->baudFact());

    QCOMPARE(controller.changedPortCount(), 0);
}

void PX4SerialPortsControllerTest::_unsentParamLoadingUntilReadEnds_data()
{
    QTest::addColumn<bool>("readAnswered");

    QTest::newRow("answered") << true;
    QTest::newRow("readFails") << false;
}

void PX4SerialPortsControllerTest::_unsentParamLoadingUntilReadEnds()
{
    QFETCH(bool, readAnswered);

    // SER_TEL2_BAUD is not streamed because nothing runs on TELEM 2
    QVERIFY(!vehicle()->parameterManager()->parameterExists(ParameterManager::defaultComponentId,
                                                            QStringLiteral("SER_TEL2_BAUD")));
    if (!readAnswered) {
        mockLink()->setParamRequestReadFailureMode(MockLink::FailParamRequestReadParamError);
    }

    PX4SerialPortsController controller;
    const PX4SerialPortRow* const telem2 = _row(controller, kTelem2);
    QVERIFY(telem2);
    QVERIFY(telem2->baudLoading());
    QVERIFY(!telem2->baudFact());

    QVERIFY(_waitForRowsLoaded(controller));
    if (readAnswered) {
        QVERIFY(telem2->baudFact());
        QCOMPARE(telem2->baudFact()->rawValue().toInt(), 921600);
    } else {
        QVERIFY(!telem2->baudFact());
    }
    QVERIFY(!telem2->rebootRequired());
    QCOMPARE(controller.changedPortCount(), 0);
}

void PX4SerialPortsControllerTest::_assignWritesImmediatelyWithoutRebootNotice()
{
    PX4SerialPortsController controller;
    QVERIFY(_waitForRowsLoaded(controller));
    const PX4SerialPortRow* const telem2 = _row(controller, kTelem2);
    QVERIFY(telem2);
    const int mavlink1Index =
        static_cast<int>(telem2->functionLabels().indexOf(QStringLiteral("MAVLink (instance 1)")));
    QVERIFY(mavlink1Index > 0);

    // Strict log checking fails the test if the reboot notice is shown
    controller.setPortFunction(kTelem2, mavlink1Index);

    QVERIFY(_waitForMockValue(QStringLiteral("MAV_1_CONFIG"), kTelem2));
    QCOMPARE(telem2->functionIndex(), mavlink1Index);
    QVERIFY(telem2->rebootRequired());
    QCOMPARE(controller.changedPortCount(), 1);
    QVERIFY(vehicle()->rebootRequired());

    // MAV_1 options are only answered by name until MAVLink instance 1 runs
    QCOMPARE_TRUE_WAIT(_optionNames(telem2).count(), 4, TestTimeout::mediumMs());
    QCOMPARE(_optionNames(telem2).first(), QStringLiteral("MAV_1_MODE"));

    QVERIFY_TRUE_WAIT(!vehicle()->parameterManager()->pendingWrites(), TestTimeout::mediumMs());
}

void PX4SerialPortsControllerTest::_revertRestoresValuesFromPageOpen()
{
    PX4SerialPortsController controller;
    QVERIFY(_waitForRowsLoaded(controller));
    const PX4SerialPortRow* const telem1 = _row(controller, kTelem1);
    QVERIFY(telem1);
    const int mavlink1Index =
        static_cast<int>(telem1->functionLabels().indexOf(QStringLiteral("MAVLink (instance 1)")));
    QVERIFY(mavlink1Index > 0);

    controller.setPortFunction(kTelem1, mavlink1Index);
    QVERIFY(_waitForMockValue(QStringLiteral("MAV_0_CONFIG"), 0));
    QVERIFY(_waitForMockValue(QStringLiteral("MAV_1_CONFIG"), kTelem1));
    QCOMPARE(controller.changedPortCount(), 1);

    // Revert is ignored until the earlier writes are acknowledged
    QVERIFY_TRUE_WAIT(!vehicle()->parameterManager()->pendingWrites(), TestTimeout::mediumMs());
    controller.revertChanges();
    QVERIFY(_waitForMockValue(QStringLiteral("MAV_0_CONFIG"), kTelem1));
    QVERIFY(_waitForMockValue(QStringLiteral("MAV_1_CONFIG"), 0));
    QCOMPARE(controller.changedPortCount(), 0);
    QVERIFY(!telem1->rebootRequired());

    QVERIFY_TRUE_WAIT(!vehicle()->parameterManager()->pendingWrites(), TestTimeout::mediumMs());
}

void PX4SerialPortsControllerTest::_conflictReportedUntilResolved()
{
    PX4SerialPortsController controller;
    QVERIFY(_waitForRowsLoaded(controller));
    const PX4SerialPortRow* const gps1 = _row(controller, kGps1);
    QVERIFY(gps1);

    Fact* const gps2Config = vehicle()->parameterManager()->getParameter(ParameterManager::defaultComponentId,
                                                                         QStringLiteral("GPS_2_CONFIG"));
    QVERIFY(gps2Config);
    gps2Config->setRawValue(kGps1);
    QVERIFY(_waitForMockValue(QStringLiteral("GPS_2_CONFIG"), kGps1));
    QVERIFY(gps1->conflictText().contains(QStringLiteral("Main GPS")));
    QVERIFY(gps1->conflictText().contains(QStringLiteral("Secondary GPS")));
    QVERIFY(gps1->optionFacts().isEmpty());

    controller.setPortFunction(kGps1, static_cast<int>(gps1->functionLabels().indexOf(QStringLiteral("Main GPS"))));
    QVERIFY(_waitForMockValue(QStringLiteral("GPS_2_CONFIG"), 0));
    QVERIFY(gps1->conflictText().isEmpty());
    QCOMPARE(gps1->functionLabels().value(gps1->functionIndex()), QStringLiteral("Main GPS"));

    QVERIFY_TRUE_WAIT(!vehicle()->parameterManager()->pendingWrites(), TestTimeout::mediumMs());
}

void PX4SerialPortsControllerTest::_sharedPortRunsSeveralFunctions()
{
    PX4SerialPortsController controller;
    QVERIFY(_waitForRowsLoaded(controller));
    const PX4SerialPortRow* const ethernet = _row(controller, kEthernet);
    QVERIFY(ethernet);
    QVERIFY(ethernet->shared());
    const int mavlink1Index =
        static_cast<int>(ethernet->functionLabels().indexOf(QStringLiteral("MAVLink (instance 1)")));
    const int ddsIndex = static_cast<int>(ethernet->functionLabels().indexOf(QStringLiteral("UXRCE-DDS Client")));
    QVERIFY(mavlink1Index > 0);
    QVERIFY(ddsIndex > 0);

    controller.setPortFunctionEnabled(kEthernet, mavlink1Index, true);
    controller.setPortFunctionEnabled(kEthernet, ddsIndex, true);
    QVERIFY(_waitForMockValue(QStringLiteral("MAV_1_CONFIG"), kEthernet));
    QVERIFY(_waitForMockValue(QStringLiteral("UXRCE_DDS_CFG"), kEthernet));
    QVERIFY(ethernet->functionEnabled().value(mavlink1Index).toBool());
    QVERIFY(ethernet->functionEnabled().value(ddsIndex).toBool());
    QVERIFY(ethernet->conflictText().isEmpty());

    // MAV_1's network options are only answered by name until MAVLink instance 1 runs
    QVERIFY(_waitForRowsLoaded(controller));
    QVERIFY(_optionNames(ethernet).contains(QStringLiteral("MAV_1_UDP_PRT")));

    controller.setPortFunctionEnabled(kEthernet, mavlink1Index, false);
    QVERIFY(_waitForMockValue(QStringLiteral("MAV_1_CONFIG"), 0));
    QCOMPARE(mockLink()->paramValue(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("UXRCE_DDS_CFG")).toInt(), kEthernet);

    QVERIFY_TRUE_WAIT(!vehicle()->parameterManager()->pendingWrites(), TestTimeout::mediumMs());
}

void PX4SerialPortsControllerTest::_changedOptionCountedOnOwningPort_data()
{
    QTest::addColumn<int>("finalPortId");

    // MAV_1_CONFIG is back to its value at page open, but MAV_1_RATE still changed on TELEM 2
    QTest::newRow("functionRemoved") << 0;
    // The rate moves with MAVLink 1 to TELEM 3, so TELEM 2 no longer differs from page open
    QTest::newRow("functionMoved") << kTelem3;
}

void PX4SerialPortsControllerTest::_changedOptionCountedOnOwningPort()
{
    QFETCH(int, finalPortId);

    PX4SerialPortsController controller;
    QVERIFY(_waitForRowsLoaded(controller));
    const PX4SerialPortRow* const telem2 = _row(controller, kTelem2);
    const PX4SerialPortRow* const telem3 = _row(controller, kTelem3);
    QVERIFY(telem2);
    QVERIFY(telem3);
    const int mavlink1Index =
        static_cast<int>(telem2->functionLabels().indexOf(QStringLiteral("MAVLink (instance 1)")));
    QVERIFY(mavlink1Index > 0);

    controller.setPortFunction(kTelem2, mavlink1Index);
    QVERIFY(_waitForMockValue(QStringLiteral("MAV_1_CONFIG"), kTelem2));
    QVERIFY(_waitForRowsLoaded(controller));
    Fact* const mav1Rate =
        vehicle()->parameterManager()->getParameter(ParameterManager::defaultComponentId, QStringLiteral("MAV_1_RATE"));
    QVERIFY(mav1Rate);
    mav1Rate->setRawValue(5000);
    QVERIFY(_waitForMockValue(QStringLiteral("MAV_1_RATE"), 5000));

    if (finalPortId == 0) {
        controller.setPortFunction(kTelem2, 0);
    } else {
        controller.setPortFunction(
            finalPortId, static_cast<int>(telem3->functionLabels().indexOf(QStringLiteral("MAVLink (instance 1)"))));
    }
    QVERIFY(_waitForMockValue(QStringLiteral("MAV_1_CONFIG"), finalPortId));
    QCOMPARE(telem2->rebootRequired(), finalPortId == 0);
    QCOMPARE(telem3->rebootRequired(), finalPortId == kTelem3);
    QCOMPARE(controller.changedPortCount(), 1);

    QVERIFY(_waitForRowsLoaded(controller));
    QVERIFY_TRUE_WAIT(!vehicle()->parameterManager()->pendingWrites(), TestTimeout::mediumMs());
}

void PX4SerialPortsControllerTest::_rcInputDriverStaysOnDefaultPort()
{
    PX4SerialPortsController controller;
    QVERIFY(_waitForRowsLoaded(controller));
    const PX4SerialPortRow* const rc = _row(controller, kRc);
    QVERIFY(rc);

    // Disabled still runs RC Input Driver, on the board's default RC port
    Fact* const rcPortConfig = vehicle()->parameterManager()->getParameter(ParameterManager::defaultComponentId,
                                                                           QStringLiteral("RC_PORT_CONFIG"));
    QVERIFY(rcPortConfig);
    rcPortConfig->setRawValue(0);
    QVERIFY(_waitForMockValue(QStringLiteral("RC_PORT_CONFIG"), 0));
    QCOMPARE(rc->functionLabels().value(rc->functionIndex()), QStringLiteral("RC Input Driver"));
    QVERIFY(!rc->functionLabels().contains(QStringLiteral("None")));

    const int mavlink1Index = static_cast<int>(rc->functionLabels().indexOf(QStringLiteral("MAVLink (instance 1)")));
    QVERIFY(mavlink1Index > 0);
    controller.setPortFunction(kRc, mavlink1Index);
    QVERIFY(_waitForMockValue(QStringLiteral("MAV_1_CONFIG"), kRc));
    QCOMPARE(mockLink()->paramValue(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("RC_PORT_CONFIG")).toInt(), 0);
    QVERIFY(rc->conflictText().contains(QStringLiteral("RC Input Driver always uses this port")));

    // Moving RC Input Driver to another port frees the RC port, so it can be cleared again
    const PX4SerialPortRow* const telem2 = _row(controller, kTelem2);
    QVERIFY(telem2);
    controller.setPortFunction(kTelem2,
                               static_cast<int>(telem2->functionLabels().indexOf(QStringLiteral("RC Input Driver"))));
    QVERIFY(_waitForMockValue(QStringLiteral("RC_PORT_CONFIG"), kTelem2));
    QVERIFY(rc->functionLabels().contains(QStringLiteral("None")));
    QVERIFY(rc->conflictText().isEmpty());

    QVERIFY(_waitForRowsLoaded(controller));
    QVERIFY_TRUE_WAIT(!vehicle()->parameterManager()->pendingWrites(), TestTimeout::mediumMs());
}

void PX4SerialPortsControllerTest::_rebootNoticeReturnsWhenPageCloses()
{
    {
        PX4SerialPortsController controller;
        QVERIFY(_waitForRowsLoaded(controller));
    }

    Fact* const mav1Config = vehicle()->parameterManager()->getParameter(ParameterManager::defaultComponentId,
                                                                         QStringLiteral("MAV_1_CONFIG"));
    QVERIFY(mav1Config);
    expectAppMessage(QRegularExpression(QStringLiteral("Vehicle reboot required for changes to take effect")));
    mav1Config->setRawValue(kTelem2);
    verifyExpectedLogMessage();

    QVERIFY(_waitForMockValue(QStringLiteral("MAV_1_CONFIG"), kTelem2));
    QVERIFY_TRUE_WAIT(!vehicle()->parameterManager()->pendingWrites(), TestTimeout::mediumMs());
}

UT_REGISTER_TEST(PX4SerialPortsControllerTest, TestLabel::Integration, TestLabel::Vehicle)
