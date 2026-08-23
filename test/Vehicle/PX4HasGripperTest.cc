#include "PX4HasGripperTest.h"

#include "Fact.h"
#include "ParameterManager.h"
#include "QGCMAVLink.h"
#include "Vehicle.h"

void PX4HasGripperTest::_gripperDetectedFromGripperType()
{
    QVERIFY(vehicle());

    // PX4MockLink.params has PD_GRIPPER_TYPE=0 and no PD_GRIPPER_EN, matching PX4 v1.17+
    QVERIFY(vehicle()->hasGripper());

    // PD_GRIPPER_TYPE=-1 (Undefined) means gripper disabled
    Fact* gripperType = vehicle()->parameterManager()->getParameter(ParameterManager::defaultComponentId,
                                                                    QStringLiteral("PD_GRIPPER_TYPE"));
    QVERIFY(gripperType);
    gripperType->setRawValue(-1);
    QVERIFY(!vehicle()->hasGripper());

    // Wait for the write to fully complete. Otherwise the async PARAM_SET fires during link
    // teardown, logging unexpected warnings which fail the test in strict log mode.
    QTRY_COMPARE_WITH_TIMEOUT(mockLink()->paramValue(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("PD_GRIPPER_TYPE")).toInt(),
                              -1, TestTimeout::mediumMs());
    QVERIFY_TRUE_WAIT(!vehicle()->parameterManager()->pendingWrites(), TestTimeout::mediumMs());
}

UT_REGISTER_TEST(PX4HasGripperTest, TestLabel::Integration, TestLabel::Vehicle)
