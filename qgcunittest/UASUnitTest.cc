#include "UASUnitTest.h"

UASUnitTest::UASUnitTest()
{
}

void UASUnitTest::initTestCase()
{
  mav= new MAVLinkProtocol();
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

    // Make sure that ID >= 0
    QCOMPARE(uas->getUASID(), -1);


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
    int verificar=uas->getAutopilotType();
  // Verify that upon construction the autopilot is set to -1
  QCOMPARE(verificar, -1);
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

void UASUnitTest::getSelected_test()
{
    QCOMPARE(uas->getSelected(), false);

    //QCOMPARE(uas->getSelected(), true);
}

void UASUnitTest::getSystemType_test()
{
    //QCOMPARE(uas->getSystemType(), -1);

    QEXPECT_FAIL("", "uas->getSystemType(), 0", Continue);
    QCOMPARE(uas->getSystemType(), 0);

    QEXPECT_FAIL("", "uas->getSystemType(), 1", Continue);
    QCOMPARE(uas->getSystemType(), 1);

    int systemType = uas->getSystemType();
    QCOMPARE(uas->getSystemType(), systemType);
}

void UASUnitTest::getAirframe_test()
{
    //QCOMPARE(uas->getAirframe(), -1);

    QCOMPARE(uas->getAirframe(), 0);

    uas->setAirframe(25);
    QCOMPARE(uas->getAirframe(), 1);

    QVERIFY(uas->getAirframe() == 25);
}

void UASUnitTest::getLinks_test()
{
    // Compare that the links count equal to 0
    QCOMPARE(uas->getLinks()->count(), 0);

    QList<LinkInterface*> links = LinkManager::instance()->getLinks();
    // Compare that the links in LinkManager count equal to 0
    QCOMPARE(links.count(), 0);

    LinkInterface* l;
    uas->getLinks()->append(l);

    // Compare that the links in LinkManager count equal to 1
    QCOMPARE(uas->getLinks()->count(), 1);

    QList<LinkInterface*> links2 = LinkManager::instance()->getLinks();

    // Compare that the links count equals after update add link in uas
    QCOMPARE(uas->getLinks()->count(), links2.count()+1);

    // Compare that the link l is equal to link[0] from links in uas
    QCOMPARE(l, static_cast<LinkInterface*>(uas->getLinks()->at(0)));

    // Compare that the link l is equal to link[0] from links in uas through count links
    QCOMPARE(l, static_cast<LinkInterface*>(uas->getLinks()->at(uas->getLinks()->count()-1)));

    uas->addLink(l);
    QCOMPARE(uas->getLinks()->count(), 1);

    uas->removeLink(0);// dynamic_cast<QObject*>(l));
    QCOMPARE(uas->getLinks()->count(), 0);
}

void UASUnitTest::getWaypointList_test()
{
    QVector<Waypoint*> kk = uas->getWaypointManager()->getWaypointList();
    QCOMPARE(kk.count(), 0);

    Waypoint* wp = new Waypoint(0,0,0,0,0,false, false, 0,0, MAV_FRAME_GLOBAL, MAV_ACTION_NAVIGATE);
    uas->getWaypointManager()->addWaypoint(wp, true);

    kk = uas->getWaypointManager()->getWaypointList();
    QCOMPARE(kk.count(), 1);

    wp = new Waypoint();
    uas->getWaypointManager()->addWaypoint(wp, false);

    kk = uas->getWaypointManager()->getWaypointList();
    QCOMPARE(kk.count(), 2);

    uas->getWaypointManager()->removeWaypoint(1);
    kk = uas->getWaypointManager()->getWaypointList();
    QCOMPARE(kk.count(), 1);

    uas->getWaypointManager()->removeWaypoint(0);
    kk = uas->getWaypointManager()->getWaypointList();
    QCOMPARE(kk.count(), 0);

    wp = new Waypoint();
    uas->getWaypointManager()->addWaypoint(wp, true);

    wp = new Waypoint();
    uas->getWaypointManager()->addWaypoint(wp, false);

    // Fail clearWaypointList
    //uas->getWaypointManager()->clearWaypointList();
    //kk = uas->getWaypointManager()->getWaypointList();
    //QCOMPARE(kk.count(), 1);
}

void UASUnitTest::battery_test()
{
    QCOMPARE(uas->getCommunicationStatus(), 0);
}
