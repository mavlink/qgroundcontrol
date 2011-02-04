#include "UASUnitTest.h"

UASUnitTest::UASUnitTest()
{
}

void UASUnitTest::initTestCase()
{
    mav= new MAVLinkProtocol();
    link = new SerialLink();
    uas=new UAS(mav,UASID);
}

void UASUnitTest::cleanupTestCase()
{
    delete uas;
    delete mav;

}

void UASUnitTest::getUASID_test()
{
    // Test a default ID of zero is assigned
    UAS* uas2 = new UAS(mav);
    QCOMPARE(uas2->getUASID(), 0);
    delete uas2;

    // Test that the chosen ID was assigned at construction
    QCOMPARE(uas->getUASID(), UASID);

    // Make sure that no other ID was sert
    QEXPECT_FAIL("", "When you set an ID it does not use the default ID of 0", Continue);
    QCOMPARE(uas->getUASID(), 0);
}

void UASUnitTest::getUASName_test()
{
    // Test that the name is build as MAV + ID
    QCOMPARE(uas->getUASName(), "MAV 0" + QString::number(UASID));

}

void UASUnitTest::getUpTime_test()
{
    UAS* uas2 = new UAS(mav);
    // Test that the uptime starts at zero to a
    // precision of seconds
    QCOMPARE(floor(uas2->getUptime()/1000.0), 0.0);

    // Sleep for three seconds
    QTest::qSleep(3000);

    // Test that the up time is computed correctly to a
    // precision of seconds
    QCOMPARE(floor(uas2->getUptime()/1000.0), 3.0);

    delete uas2;
}

void UASUnitTest::getCommunicationStatus_test()
{
    // Verify that upon construction the Comm status is disconnected
    QCOMPARE(uas->getCommunicationStatus(), static_cast<int>(UASInterface::COMM_DISCONNECTED));
}

void UASUnitTest::filterVoltage_test()
{
    float verificar=uas->filterVoltage(0.4f);
    // Verify that upon construction the Comm status is disconnected
    QCOMPARE(verificar, 8.52f);
}
void UASUnitTest:: getAutopilotType_test()
{
    int type = uas->getAutopilotType();
    // Verify that upon construction the autopilot is set to -1
    QCOMPARE(type, -1);
}
void UASUnitTest::setAutopilotType_test()
{
    uas->setAutopilotType(2);
    // Verify that the autopilot is set
    QCOMPARE(uas->getAutopilotType(), 2);
}

void UASUnitTest::getStatusForCode_test()
{
    QString state, desc;
    state = "";
    desc = "";

    uas->getStatusForCode(MAV_STATE_UNINIT, state, desc);
    QVERIFY(state == "UNINIT");

    uas->getStatusForCode(MAV_STATE_UNINIT, state, desc);
    QVERIFY(state == "UNINIT");

    uas->getStatusForCode(MAV_STATE_BOOT, state, desc);
    QVERIFY(state == "BOOT");

    uas->getStatusForCode(MAV_STATE_CALIBRATING, state, desc);
    QVERIFY(state == "CALIBRATING");

    uas->getStatusForCode(MAV_STATE_ACTIVE, state, desc);
    QVERIFY(state == "ACTIVE");

    uas->getStatusForCode(MAV_STATE_STANDBY, state, desc);
    QVERIFY(state == "STANDBY");

    uas->getStatusForCode(MAV_STATE_CRITICAL, state, desc);
    QVERIFY(state == "CRITICAL");

    uas->getStatusForCode(MAV_STATE_EMERGENCY, state, desc);
    QVERIFY(state == "EMERGENCY");

    uas->getStatusForCode(MAV_STATE_POWEROFF, state, desc);
    QVERIFY(state == "SHUTDOWN");

    uas->getStatusForCode(5325, state, desc);
    QVERIFY(state == "UNKNOWN");
}


void UASUnitTest::getLocalX_test()
{
    QCOMPARE(uas->getLocalX(), 0.0);
}
void UASUnitTest::getLocalY_test()
{
    QCOMPARE(uas->getLocalY(), 0.0);
}
void UASUnitTest::getLocalZ_test()
{
    QCOMPARE(uas->getLocalZ(), 0.0);
}
void UASUnitTest::getLatitude_test()
{  QCOMPARE(uas->getLatitude(), 0.0);
}
void UASUnitTest::getLongitude_test()
{
    QCOMPARE(uas->getLongitude(), 0.0);
}
void UASUnitTest::getAltitude_test()
{
    QCOMPARE(uas->getAltitude(), 0.0);
}
void UASUnitTest::getRoll_test()
{
    QCOMPARE(uas->getRoll(), 0.0);
}
void UASUnitTest::getPitch_test()
{
    QCOMPARE(uas->getPitch(), 0.0);
}
void UASUnitTest::getYaw_test()
{
    QCOMPARE(uas->getYaw(), 0.0);
}
void UASUnitTest::attitudeLimitsZero_test()
{
    mavlink_message_t msg;
    mavlink_attitude_t att;

    // Set zero values and encode them
    att.roll  = 0.0f;
    att.pitch = 0.0f;
    att.yaw   = 0.0f;
    mavlink_msg_attitude_encode(uas->getUASID(), MAV_COMP_ID_IMU, &msg, &att);
    // Let UAS decode message
    uas->receiveMessage(link, msg);
    // Check result
    QCOMPARE(uas->getRoll(), 0.0);
    QCOMPARE(uas->getPitch(), 0.0);
    QCOMPARE(uas->getYaw(), 0.0);
}

void UASUnitTest::attitudeLimitsPi_test()
{
    mavlink_message_t msg;
    mavlink_attitude_t att;
    // Set PI values and encode them
    att.roll  = M_PI;
    att.pitch = M_PI;
    att.yaw   = M_PI;
    mavlink_msg_attitude_encode(uas->getUASID(), MAV_COMP_ID_IMU, &msg, &att);
    // Let UAS decode message
    uas->receiveMessage(link, msg);
    // Check result
    QVERIFY((uas->getRoll() < M_PI+0.000001) && (uas->getRoll() > M_PI+-0.000001));
    QVERIFY((uas->getPitch() < M_PI+0.000001) && (uas->getPitch() > M_PI+-0.000001));
    QVERIFY((uas->getYaw() < M_PI+0.000001) && (uas->getYaw() > M_PI+-0.000001));
}

void UASUnitTest::attitudeLimitsMinusPi_test()
{
    mavlink_message_t msg;
    mavlink_attitude_t att;
    // Set -PI values and encode them
    att.roll  = -M_PI;
    att.pitch = -M_PI;
    att.yaw   = -M_PI;
    mavlink_msg_attitude_encode(uas->getUASID(), MAV_COMP_ID_IMU, &msg, &att);
    // Let UAS decode message
    uas->receiveMessage(link, msg);
    // Check result
    QVERIFY((uas->getRoll() > -M_PI-0.000001) && (uas->getRoll() < -M_PI+0.000001));
    QVERIFY((uas->getPitch() > -M_PI-0.000001) && (uas->getPitch() < -M_PI+0.000001));
    QVERIFY((uas->getYaw() > -M_PI-0.000001) && (uas->getYaw() < -M_PI+0.000001));
}

void UASUnitTest::attitudeLimitsTwoPi_test()
{
    mavlink_message_t msg;
    mavlink_attitude_t att;
    // Set off-limit values and check
    // correct wrapping
    // Set 2PI values and encode them
    att.roll  = 2*M_PI;
    att.pitch = 2*M_PI;
    att.yaw   = 2*M_PI;
    mavlink_msg_attitude_encode(uas->getUASID(), MAV_COMP_ID_IMU, &msg, &att);
    // Let UAS decode message
    uas->receiveMessage(link, msg);
    // Check result
    QVERIFY((uas->getRoll() < 0.000001) && (uas->getRoll() > -0.000001));
    QVERIFY((uas->getPitch() < 0.000001) && (uas->getPitch() > -0.000001));
    QVERIFY((uas->getYaw() < 0.000001) && (uas->getYaw() > -0.000001));
}

void UASUnitTest::attitudeLimitsMinusTwoPi_test()
{
    mavlink_message_t msg;
    mavlink_attitude_t att;
    // Set -2PI values and encode them
    att.roll  = -2*M_PI;
    att.pitch = -2*M_PI;
    att.yaw   = -2*M_PI;
    mavlink_msg_attitude_encode(uas->getUASID(), MAV_COMP_ID_IMU, &msg, &att);
    // Let UAS decode message
    uas->receiveMessage(link, msg);
    // Check result
    QVERIFY((uas->getRoll() < 0.000001) && (uas->getRoll() > -0.000001));
    QVERIFY((uas->getPitch() < 0.000001) && (uas->getPitch() > -0.000001));
    QVERIFY((uas->getYaw() < 0.000001) && (uas->getYaw() > -0.000001));
}

void UASUnitTest::attitudeLimitsTwoPiOne_test()
{
    mavlink_message_t msg;
    mavlink_attitude_t att;
    // Set over 2 PI values and encode them
    att.roll  = 2*M_PI+1.0f;
    att.pitch = 2*M_PI+1.0f;
    att.yaw   = 2*M_PI+1.0f;
    mavlink_msg_attitude_encode(uas->getUASID(), MAV_COMP_ID_IMU, &msg, &att);
    // Let UAS decode message
    uas->receiveMessage(link, msg);
    // Check result
    QVERIFY((uas->getRoll() < 1.000001) && (uas->getRoll() > 0.999999));
    QVERIFY((uas->getPitch() < 1.000001) && (uas->getPitch() > 0.999999));
    QVERIFY((uas->getYaw() < 1.000001) && (uas->getYaw() > 0.999999));
}

void UASUnitTest::attitudeLimitsMinusTwoPiOne_test()
{
    mavlink_message_t msg;
    mavlink_attitude_t att;
    // Set 3PI values and encode them
    att.roll  = -2*M_PI-1.0f;
    att.pitch = -2*M_PI-1.0f;
    att.yaw   = -2*M_PI-1.0f;
    mavlink_msg_attitude_encode(uas->getUASID(), MAV_COMP_ID_IMU, &msg, &att);
    // Let UAS decode message
    uas->receiveMessage(link, msg);
    // Check result
    QVERIFY((uas->getRoll() > -1.000001) && (uas->getRoll() < -0.999999));
    QVERIFY((uas->getPitch() > -1.000001) && (uas->getPitch() < -0.999999));
    QVERIFY((uas->getYaw() > -1.000001) && (uas->getYaw() < -0.999999));
}
