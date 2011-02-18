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
    QCOMPARE(uas->getUASID(), 100);
}

void UASUnitTest::getUASName_test()
{
  // Test that the name is build as MAV + ID
  QCOMPARE(uas->getUASName(), "MAV " + QString::number(UASID));

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
}

void UASUnitTest::getSystemType_test()
{
    QCOMPARE(uas->getSystemType(), 13);
}

void UASUnitTest::getAirframe_test()
{
    QCOMPARE(uas->getAirframe(), 0);

    uas->setAirframe(25);
    QVERIFY(uas->getAirframe() == 25);
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

    qDebug()<<"disconnect SIGNAL waypointListChanged";
}

void UASUnitTest::getWaypoint_test()
{
    Waypoint* wp = new Waypoint(0,5.6,0,0,0,false, false, 0,0, MAV_FRAME_GLOBAL, MAV_ACTION_NAVIGATE);

    uas->getWaypointManager()->addWaypoint(wp, true);

    QVector<Waypoint*> wpList = uas->getWaypointManager()->getWaypointList();

    QCOMPARE(wpList.count(), 1);
    QCOMPARE(static_cast<quint16>(0), static_cast<Waypoint*>(wpList.at(0))->getId());

    wp = new Waypoint(0, 5.6, 2, 3);
    uas->getWaypointManager()->addWaypoint(wp, true);
    Waypoint* wp2 = static_cast<Waypoint*>(wpList.at(0));

    QCOMPARE(wp->getX(), wp2->getX());
    QCOMPARE(wp->getFrame(), MAV_FRAME_GLOBAL);
    QCOMPARE(wp->getFrame(), wp2->getFrame());
}

void UASUnitTest::signalWayPoint_test()
{
    QSignalSpy spy(uas->getWaypointManager(), SIGNAL(waypointListChanged()));

    Waypoint* wp = new Waypoint(0,5.6,0,0,0,false, false, 0,0, MAV_FRAME_GLOBAL, MAV_ACTION_NAVIGATE);
    uas->getWaypointManager()->addWaypoint(wp, true);

    QCOMPARE(spy.count(), 1); // 1 listChanged for add wayPoint
    uas->getWaypointManager()->removeWaypoint(0);
    QCOMPARE(spy.count(), 2); // 2 listChanged for remove wayPoint

    QSignalSpy spyDestroyed(uas->getWaypointManager(), SIGNAL(destroyed()));
    QVERIFY(spyDestroyed.isValid());
    QCOMPARE( spyDestroyed.count(), 0 );

    delete uas;// delete(destroyed) uas for validating
    QCOMPARE(spyDestroyed.count(), 1);// count destroyed uas should are 1

    uas = new UAS(mav,UASID);
    QSignalSpy spy2(uas->getWaypointManager(), SIGNAL(waypointListChanged()));
    QCOMPARE(spy2.count(), 0);

    uas->getWaypointManager()->addWaypoint(wp, true);
    QCOMPARE(spy2.count(), 1);

    uas->getWaypointManager()->clearWaypointList();
    QVector<Waypoint*> wpList = uas->getWaypointManager()->getWaypointList();
    QCOMPARE(wpList.count(), 1);
}

void UASUnitTest::signalUASLink_test()
{
    QSignalSpy spy(uas, SIGNAL(modeChanged(int,QString,QString)));
    uas->setMode(2);
    QCOMPARE(spy.count(), 0);// not solve for UAS not receiving message from UAS

    QSignalSpy spyS(LinkManager::instance(), SIGNAL(newLink(LinkInterface*)));
    SerialLink* link = new SerialLink();
    LinkManager::instance()->add(link);
    LinkManager::instance()->addProtocol(link, mav);
    QCOMPARE(spyS.count(), 1);

    LinkManager::instance()->add(link);
    LinkManager::instance()->addProtocol(link, mav);
    QCOMPARE(spyS.count(), 1);// not add SerialLink, exist in list

    SerialLink* link2 = new SerialLink();
    LinkManager::instance()->add(link2);
    LinkManager::instance()->addProtocol(link2, mav);
    QCOMPARE(spyS.count(), 2);// add SerialLink, not exist in list

    QList<LinkInterface*> links = LinkManager::instance()->getLinks();
    foreach(LinkInterface* link, links)
    {
        qDebug()<< link->getName();
        qDebug()<< QString::number(link->getId());
        qDebug()<< QString::number(link->getNominalDataRate());
        QVERIFY(link != NULL);
        uas->addLink(link);
    }

    SerialLink* ff = static_cast<SerialLink*>(uas->getLinks()->at(0));

    QCOMPARE(ff->isConnected(), false);

    QCOMPARE(ff->isRunning(), false);

    QCOMPARE(ff->isFinished(), false);

    QCOMPARE(links.count(), uas->getLinks()->count());
    QCOMPARE(uas->getLinks()->count(), 2);

    LinkInterface* ff99 = static_cast<LinkInterface*>(links.at(1));
    LinkManager::instance()->removeLink(ff99);

    QCOMPARE(LinkManager::instance()->getLinks().count(), 1);
    QCOMPARE(uas->getLinks()->count(), 2);


    QCOMPARE(static_cast<LinkInterface*>(LinkManager::instance()->getLinks().at(0))->getId(),
             static_cast<LinkInterface*>(uas->getLinks()->at(0))->getId());

    link = new SerialLink();
    LinkManager::instance()->add(link);
    LinkManager::instance()->addProtocol(link, mav);
    QCOMPARE(spyS.count(), 3);
}

void UASUnitTest::signalIdUASLink_test()
{
    SerialLink* myLink = new SerialLink();
    myLink->setPortName("COM 17");
    LinkManager::instance()->add(myLink);
    LinkManager::instance()->addProtocol(myLink, mav);

    myLink = new SerialLink();
    myLink->setPortName("COM 18");
    LinkManager::instance()->add(myLink);
    LinkManager::instance()->addProtocol(myLink, mav);

    QCOMPARE(LinkManager::instance()->getLinks().count(), 4);

    QList<LinkInterface*> links = LinkManager::instance()->getLinks();

    LinkInterface* a = static_cast<LinkInterface*>(links.at(2));
    LinkInterface* b = static_cast<LinkInterface*>(links.at(3));

    QCOMPARE(a->getName(), QString("serial port COM 17"));
    QCOMPARE(b->getName(), QString("serial port COM 18"));
}
