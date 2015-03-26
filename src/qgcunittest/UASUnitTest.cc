#include "UASUnitTest.h"
#include <stdio.h>
#include <QObject>

UT_REGISTER_TEST(UASUnitTest)

UASUnitTest::UASUnitTest()
{
}
//This function is called after every test
void UASUnitTest::init()
{
    UnitTest::init();
    
    _mavlink = new MAVLinkProtocol();
    _uas = new UAS(_mavlink, UASID);
    _uas->deleteSettings();
}
//this function is called after every test
void UASUnitTest::cleanup()
{
    delete _uas;
    _uas = NULL;

    delete _mavlink;
    _mavlink = NULL;
    
    UnitTest::cleanup();
}

void UASUnitTest::getUASID_test()
{
    // Test a default ID of zero is assigned
    UAS* uas2 = new UAS(_mavlink);
    QCOMPARE(uas2->getUASID(), 0);
    delete uas2;

    // Test that the chosen ID was assigned at construction
    QCOMPARE(_uas->getUASID(), UASID);

    // Make sure that no other ID was set
    QEXPECT_FAIL("", "When you set an ID it does not use the default ID of 0", Continue);
    QCOMPARE(_uas->getUASID(), 0);

    // Make sure that ID >= 0
    QCOMPARE(_uas->getUASID(), 100);

}

void UASUnitTest::getUASName_test()
{
  // Test that the name is build as MAV + ID
  QCOMPARE(_uas->getUASName(), "MAV " + QString::number(UASID));

}

void UASUnitTest::getUpTime_test()
{
    UAS* uas2 = new UAS(_mavlink);
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

void UASUnitTest::filterVoltage_test()
{
    float verificar=_uas->filterVoltage(0.4f);

    // We allow the voltage returned to be within a small delta
    const float allowedDelta = 0.05f;
    const float desiredVoltage = 7.36f;
    QVERIFY(verificar > (desiredVoltage - allowedDelta) && verificar < (desiredVoltage + allowedDelta));
}

void UASUnitTest:: getAutopilotType_test()
{
    int type = _uas->getAutopilotType();
    // Verify that upon construction the autopilot is set to -1
    QCOMPARE(type, -1);
}

void UASUnitTest::setAutopilotType_test()
{
    _uas->setAutopilotType(2);
    // Verify that the autopilot is set
    QCOMPARE(_uas->getAutopilotType(), 2);
}

//verify that the correct status is returned if a certain statue is given to _uas
void UASUnitTest::getStatusForCode_test()
{
    QString state, desc;
    state = "";
    desc = "";

    _uas->getStatusForCode(MAV_STATE_UNINIT, state, desc);
    QVERIFY(state == "UNINIT");

    _uas->getStatusForCode(MAV_STATE_UNINIT, state, desc);
    QVERIFY(state == "UNINIT");

    _uas->getStatusForCode(MAV_STATE_BOOT, state, desc);
    QVERIFY(state == "BOOT");

    _uas->getStatusForCode(MAV_STATE_CALIBRATING, state, desc);
    QVERIFY(state == "CALIBRATING");

    _uas->getStatusForCode(MAV_STATE_ACTIVE, state, desc);
    QVERIFY(state == "ACTIVE");

    _uas->getStatusForCode(MAV_STATE_STANDBY, state, desc);
    QVERIFY(state == "STANDBY");

    _uas->getStatusForCode(MAV_STATE_CRITICAL, state, desc);
    QVERIFY(state == "CRITICAL");

    _uas->getStatusForCode(MAV_STATE_EMERGENCY, state, desc);
    QVERIFY(state == "EMERGENCY");

    _uas->getStatusForCode(MAV_STATE_POWEROFF, state, desc);
    QVERIFY(state == "SHUTDOWN");

    _uas->getStatusForCode(5325, state, desc);
    QVERIFY(state == "UNKNOWN");
}

void UASUnitTest::getLocalX_test()
{
   QCOMPARE(_uas->getLocalX(), 0.0);
}
void UASUnitTest::getLocalY_test()
{
    QCOMPARE(_uas->getLocalY(), 0.0);
}
void UASUnitTest::getLocalZ_test()
{
    QCOMPARE(_uas->getLocalZ(), 0.0);
}
void UASUnitTest::getLatitude_test()
{
    QCOMPARE(_uas->getLatitude(), 0.0);
}
void UASUnitTest::getLongitude_test()
{
    QCOMPARE(_uas->getLongitude(), 0.0);
}
void UASUnitTest::getAltitudeAMSL_test()
{
    QCOMPARE(_uas->getAltitudeAMSL(), 0.0);
}
void UASUnitTest::getAltitudeRelative_test()
{
    QCOMPARE(_uas->getAltitudeRelative(), 0.0);
}
void UASUnitTest::getRoll_test()
{
    QCOMPARE(_uas->getRoll(), 0.0);
}
void UASUnitTest::getPitch_test()
{
    QCOMPARE(_uas->getPitch(), 0.0);
}
void UASUnitTest::getYaw_test()
{
    QCOMPARE(_uas->getYaw(), 0.0);
}

void UASUnitTest::getSelected_test()
{
    QCOMPARE(_uas->getSelected(), false);
}

void UASUnitTest::getSystemType_test()
{    //check that system type is set to MAV_TYPE_GENERIC when initialized
    QCOMPARE(_uas->getSystemType(), 0);
    _uas->setSystemType(13);
    QCOMPARE(_uas->getSystemType(), 13);
}

void UASUnitTest::getAirframe_test()
{
    //when _uas is constructed, airframe is set to QGC_AIRFRAME_GENERIC
    QVERIFY(_uas->getAirframe() == UASInterface::QGC_AIRFRAME_GENERIC);
}

void UASUnitTest::setAirframe_test()
{
    //check at construction, that airframe=0 (GENERIC)
    QVERIFY(_uas->getAirframe() == UASInterface::QGC_AIRFRAME_GENERIC);

    //check that set airframe works
    _uas->setAirframe(UASInterface::QGC_AIRFRAME_HEXCOPTER);
    QVERIFY(_uas->getAirframe() == UASInterface::QGC_AIRFRAME_HEXCOPTER);

    //check that setAirframe will not assign a number to airframe, that is 
    //not defined in the enum 
    _uas->setAirframe(UASInterface::QGC_AIRFRAME_END_OF_ENUM);
    QVERIFY(_uas->getAirframe() == UASInterface::QGC_AIRFRAME_HEXCOPTER);
}

void UASUnitTest::getWaypointList_test()
{
    QList<Waypoint*> kk = _uas->getWaypointManager()->getWaypointEditableList();
    QCOMPARE(kk.count(), 0);

    Waypoint* wp = new Waypoint(0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,false, false, MAV_FRAME_GLOBAL, MAV_CMD_MISSION_START, "blah");
    _uas->getWaypointManager()->addWaypointEditable(wp, true);

    kk = _uas->getWaypointManager()->getWaypointEditableList();
    QCOMPARE(kk.count(), 1);

    wp = new Waypoint();
    _uas->getWaypointManager()->addWaypointEditable(wp, false);

    kk = _uas->getWaypointManager()->getWaypointEditableList();
    QCOMPARE(kk.count(), 2);

    _uas->getWaypointManager()->removeWaypoint(1);
    kk = _uas->getWaypointManager()->getWaypointEditableList();
    QCOMPARE(kk.count(), 1);

    _uas->getWaypointManager()->removeWaypoint(0);
    kk = _uas->getWaypointManager()->getWaypointEditableList();
    QCOMPARE(kk.count(), 0);

    qDebug()<<"disconnect SIGNAL waypointListChanged";

}

void UASUnitTest::getWaypoint_test()
{
    Waypoint* wp = new Waypoint(0,5.6,2.0,3.0,0.0,0.0,0.0,0.0,false, false, MAV_FRAME_GLOBAL, MAV_CMD_MISSION_START, "blah");

    _uas->getWaypointManager()->addWaypointEditable(wp, true);

    QList<Waypoint*> wpList = _uas->getWaypointManager()->getWaypointEditableList();

    QCOMPARE(wpList.count(), 1);
    QCOMPARE(static_cast<quint16>(0), static_cast<Waypoint*>(wpList.at(0))->getId());

    Waypoint*  wp3 = new Waypoint(1, 5.6, 2.0, 3.0);
    _uas->getWaypointManager()->addWaypointEditable(wp3, true);
    wpList = _uas->getWaypointManager()->getWaypointEditableList();
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
    QSignalSpy spy(_uas->getWaypointManager(), SIGNAL(waypointEditableListChanged()));

    Waypoint* wp = new Waypoint(0,1.0,0.0,0.0,0.0,0.0,0.0,0.0,false, false, MAV_FRAME_GLOBAL, MAV_CMD_MISSION_START, "blah");
    _uas->getWaypointManager()->addWaypointEditable(wp, true);

    QCOMPARE(spy.count(), 1); // 1 listChanged for add wayPoint
    _uas->getWaypointManager()->removeWaypoint(0);
    QCOMPARE(spy.count(), 2); // 2 listChanged for remove wayPoint

    QSignalSpy spyDestroyed(_uas->getWaypointManager(), SIGNAL(destroyed()));
    QVERIFY(spyDestroyed.isValid());
    QCOMPARE( spyDestroyed.count(), 0 );

    delete _uas;// delete(destroyed) _uas for validating
    _uas = NULL;
    QCOMPARE(spyDestroyed.count(), 1);// count destroyed _uas should are 1
    _uas = new UAS(_mavlink, UASID);
    QSignalSpy spy2(_uas->getWaypointManager(), SIGNAL(waypointEditableListChanged()));
    QCOMPARE(spy2.count(), 0);
    Waypoint* wp2 = new Waypoint(0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,false, false, MAV_FRAME_GLOBAL, MAV_CMD_MISSION_START, "blah");

    _uas->getWaypointManager()->addWaypointEditable(wp2, true);
    QCOMPARE(spy2.count(), 1);

    _uas->getWaypointManager()->clearWaypointList();
    QList<Waypoint*> wpList = _uas->getWaypointManager()->getWaypointEditableList();
    QCOMPARE(wpList.count(), 1);
    delete _uas;
    _uas = NULL;
    delete wp2;
}
