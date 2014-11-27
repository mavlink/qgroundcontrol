#include "UASUnitTest.h"
#include <stdio.h>
#include <QObject>
#include <QThread>

UASUnitTest::UASUnitTest()
{
}
//This function is called after every test
void UASUnitTest::init()
{
    mav = new MAVLinkProtocol();
    uas = new UAS(mav, QThread::currentThread(), UASID);
    uas->deleteSettings();
}
//this function is called after every test
void UASUnitTest::cleanup()
{
    delete uas;
    uas = NULL;

    delete mav;
    mav = NULL;
}

void UASUnitTest::getUASID_test()
{
    // Test a default ID of zero is assigned
    UAS* uas2 = new UAS(mav, QThread::currentThread());
    QCOMPARE(uas2->getUASID(), 0);
    delete uas2;

    // Test that the chosen ID was assigned at construction
    QCOMPARE(uas->getUASID(), UASID);

    // Make sure that no other ID was set
    QEXPECT_FAIL("", "When you set an ID it does not use the default ID of 0", Continue);
    QCOMPARE(uas->getUASID(), 0);

    // Make sure that ID >= 0
    QCOMPARE(uas->getUASID(), 100);

}

void UASUnitTest::getUASName_test()
{
  // Test that the name is build as MAV + ID
  QCOMPARE(uas->getUASName(), "MAV " + QString::number(UASID));

}

void UASUnitTest::getUpTime_test()
{
    UAS* uas2 = new UAS(mav, QThread::currentThread());
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

    // We allow the voltage returned to be within a small delta
    const float allowedDelta = 0.05f;
    const float desiredVoltage = 7.36f;
    QVERIFY(verificar > (desiredVoltage - allowedDelta) && verificar < (desiredVoltage + allowedDelta));
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

//verify that the correct status is returned if a certain statue is given to uas
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
{
    QCOMPARE(uas->getLatitude(), 0.0);
}
void UASUnitTest::getLongitude_test()
{
    QCOMPARE(uas->getLongitude(), 0.0);
}
void UASUnitTest::getAltitudeAMSL_test()
{
    QCOMPARE(uas->getAltitudeAMSL(), 0.0);
}
void UASUnitTest::getAltitudeRelative_test()
{
    QCOMPARE(uas->getAltitudeRelative(), 0.0);
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

void UASUnitTest::getSelected_test()
{
    QCOMPARE(uas->getSelected(), false);
}

void UASUnitTest::getSystemType_test()
{    //check that system type is set to MAV_TYPE_GENERIC when initialized
    QCOMPARE(uas->getSystemType(), 0);
    uas->setSystemType(13);
    QCOMPARE(uas->getSystemType(), 13);
}

void UASUnitTest::getAirframe_test()
{
    //when uas is constructed, airframe is set to QGC_AIRFRAME_GENERIC
    QVERIFY(uas->getAirframe() == UASInterface::QGC_AIRFRAME_GENERIC);
}

void UASUnitTest::setAirframe_test()
{
    //check at construction, that airframe=0 (GENERIC)
    QVERIFY(uas->getAirframe() == UASInterface::QGC_AIRFRAME_GENERIC);

    //check that set airframe works
    uas->setAirframe(UASInterface::QGC_AIRFRAME_HEXCOPTER);
    QVERIFY(uas->getAirframe() == UASInterface::QGC_AIRFRAME_HEXCOPTER);

    //check that setAirframe will not assign a number to airframe, that is 
    //not defined in the enum 
    uas->setAirframe(UASInterface::QGC_AIRFRAME_END_OF_ENUM);
    QVERIFY(uas->getAirframe() == UASInterface::QGC_AIRFRAME_HEXCOPTER);
}

void UASUnitTest::getWaypointList_test()
{
    QList<Waypoint*> kk = uas->getWaypointManager()->getWaypointEditableList();
    QCOMPARE(kk.count(), 0);

    Waypoint* wp = new Waypoint(0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,false, false, MAV_FRAME_GLOBAL, MAV_CMD_MISSION_START, "blah");
    uas->getWaypointManager()->addWaypointEditable(wp, true);

    kk = uas->getWaypointManager()->getWaypointEditableList();
    QCOMPARE(kk.count(), 1);

    wp = new Waypoint();
    uas->getWaypointManager()->addWaypointEditable(wp, false);

    kk = uas->getWaypointManager()->getWaypointEditableList();
    QCOMPARE(kk.count(), 2);

    uas->getWaypointManager()->removeWaypoint(1);
    kk = uas->getWaypointManager()->getWaypointEditableList();
    QCOMPARE(kk.count(), 1);

    uas->getWaypointManager()->removeWaypoint(0);
    kk = uas->getWaypointManager()->getWaypointEditableList();
    QCOMPARE(kk.count(), 0);

    qDebug()<<"disconnect SIGNAL waypointListChanged";

}

void UASUnitTest::getWaypoint_test()
{
    Waypoint* wp = new Waypoint(0,5.6,2.0,3.0,0.0,0.0,0.0,0.0,false, false, MAV_FRAME_GLOBAL, MAV_CMD_MISSION_START, "blah");

    uas->getWaypointManager()->addWaypointEditable(wp, true);

    QList<Waypoint*> wpList = uas->getWaypointManager()->getWaypointEditableList();

    QCOMPARE(wpList.count(), 1);
    QCOMPARE(static_cast<quint16>(0), static_cast<Waypoint*>(wpList.at(0))->getId());

    Waypoint*  wp3 = new Waypoint(1, 5.6, 2.0, 3.0);
    uas->getWaypointManager()->addWaypointEditable(wp3, true);
    wpList = uas->getWaypointManager()->getWaypointEditableList();
    Waypoint* wp2 = static_cast<Waypoint*>(wpList.at(0));

    QCOMPARE(wpList.count(), 2);
    QCOMPARE(wp3->getX(), wp2->getX());
    QCOMPARE(wp3->getY(), wp2->getY());
    QCOMPARE(wp3->getZ(), wp2->getZ());
    QCOMPARE(wpList.at(1)->getId(), static_cast<quint16>(1));
    QCOMPARE(wp3->getFrame(), MAV_FRAME_GLOBAL);
    QCOMPARE(wp3->getFrame(), wp2->getFrame());

    delete wp3;
    delete wp;
}

void UASUnitTest::signalWayPoint_test()
{
    QSignalSpy spy(uas->getWaypointManager(), SIGNAL(waypointEditableListChanged()));

    Waypoint* wp = new Waypoint(0,1.0,0.0,0.0,0.0,0.0,0.0,0.0,false, false, MAV_FRAME_GLOBAL, MAV_CMD_MISSION_START, "blah");
    uas->getWaypointManager()->addWaypointEditable(wp, true);

    QCOMPARE(spy.count(), 1); // 1 listChanged for add wayPoint
    uas->getWaypointManager()->removeWaypoint(0);
    QCOMPARE(spy.count(), 2); // 2 listChanged for remove wayPoint

    QSignalSpy spyDestroyed(uas->getWaypointManager(), SIGNAL(destroyed()));
    QVERIFY(spyDestroyed.isValid());
    QCOMPARE( spyDestroyed.count(), 0 );

    delete uas;// delete(destroyed) uas for validating
    uas = NULL;
    QCOMPARE(spyDestroyed.count(), 1);// count destroyed uas should are 1
    uas = new UAS(mav, QThread::currentThread(), UASID);
    QSignalSpy spy2(uas->getWaypointManager(), SIGNAL(waypointEditableListChanged()));
    QCOMPARE(spy2.count(), 0);
    Waypoint* wp2 = new Waypoint(0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,false, false, MAV_FRAME_GLOBAL, MAV_CMD_MISSION_START, "blah");

    uas->getWaypointManager()->addWaypointEditable(wp2, true);
    QCOMPARE(spy2.count(), 1);

    uas->getWaypointManager()->clearWaypointList();
    QList<Waypoint*> wpList = uas->getWaypointManager()->getWaypointEditableList();
    QCOMPARE(wpList.count(), 1);
    delete uas;
    uas = NULL;
    delete wp2;
}
