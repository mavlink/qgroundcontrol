#include "UASUnitTest.h"
#include <stdio.h>
#include <QObject>
UASUnitTest::UASUnitTest()
{
}
//This function is called after every test
void UASUnitTest::init()
{
    mav = new MAVLinkProtocol();
    uas = new UAS(mav, UASID);
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
    UAS* uas2 = new UAS(mav);
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
{    //check that system type is set to MAV_TYPE_GENERIC when initialized
    QCOMPARE(uas->getSystemType(), 0);
    uas->setSystemType(13);
    QCOMPARE(uas->getSystemType(), 13);
}

void UASUnitTest::getAirframe_test()
{
    //when uas is constructed, airframe is set to QGC_AIRFRAME_GENERIC which is 0
    QCOMPARE(uas->getAirframe(), 0);
}

void UASUnitTest::setAirframe_test()
{
    //check at construction, that airframe=0 (GENERIC)
    QVERIFY(uas->getAirframe() == 0);

    //check that set airframe works
    uas->setAirframe(11);
    QVERIFY(uas->getAirframe() == 11);

    //check that setAirframe will not assign a number to airframe, that is 
    //not defined in the enum 
    uas->setAirframe(12);
    QVERIFY(uas->getAirframe() == 11);
}

void UASUnitTest::getWaypointList_test()
{
    QVector<Waypoint*> kk = uas->getWaypointManager()->getWaypointEditableList();
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

    QVector<Waypoint*> wpList = uas->getWaypointManager()->getWaypointEditableList();

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
    uas = new UAS(mav,UASID);
    QSignalSpy spy2(uas->getWaypointManager(), SIGNAL(waypointEditableListChanged()));
    QCOMPARE(spy2.count(), 0);
    Waypoint* wp2 = new Waypoint(0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,false, false, MAV_FRAME_GLOBAL, MAV_CMD_MISSION_START, "blah");

    uas->getWaypointManager()->addWaypointEditable(wp2, true);
    QCOMPARE(spy2.count(), 1);

    uas->getWaypointManager()->clearWaypointList();
    QVector<Waypoint*> wpList = uas->getWaypointManager()->getWaypointEditableList();
    QCOMPARE(wpList.count(), 1);
    delete uas;
    uas = NULL;
    delete wp2;
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
    delete link2;

    QCOMPARE(LinkManager::instance()->getLinks().count(), 1);
    QCOMPARE(uas->getLinks()->count(), 2);

    QCOMPARE(static_cast<LinkInterface*>(LinkManager::instance()->getLinks().at(0))->getId(),
             static_cast<LinkInterface*>(uas->getLinks()->at(0))->getId());

    SerialLink* link3 = new SerialLink();
    LinkManager::instance()->add(link3);
    LinkManager::instance()->addProtocol(link3, mav);
    QCOMPARE(spyS.count(), 3);
    QCOMPARE(LinkManager::instance()->getLinks().count(), 2);

    //all the links in LinkManager must be deleted because LinkManager::instance
    //is static. 
    LinkManager::instance()->removeLink(link3);
    delete link3;
    QCOMPARE(LinkManager::instance()->getLinks().count(), 1);
    LinkManager::instance()->removeLink(link);
    delete link;

    QCOMPARE(LinkManager::instance()->getLinks().count(), 0);
}

void UASUnitTest::signalIdUASLink_test()
{

    QCOMPARE(LinkManager::instance()->getLinks().count(), 0);
    SerialLink* myLink = new SerialLink();
    myLink->setPortName("COM 17");
    LinkManager::instance()->add(myLink);
    LinkManager::instance()->addProtocol(myLink, mav);

    SerialLink* myLink2 = new SerialLink();
    myLink2->setPortName("COM 18");
    LinkManager::instance()->add(myLink2);
    LinkManager::instance()->addProtocol(myLink2, mav);

    SerialLink* myLink3 = new SerialLink();
    myLink3->setPortName("COM 19");
    LinkManager::instance()->add(myLink3);
    LinkManager::instance()->addProtocol(myLink3, mav);

    SerialLink* myLink4 = new SerialLink();
    myLink4->setPortName("COM 20");
    LinkManager::instance()->add(myLink4);
    LinkManager::instance()->addProtocol(myLink4, mav);

    QCOMPARE(LinkManager::instance()->getLinks().count(), 4);

    QList<LinkInterface*> links = LinkManager::instance()->getLinks();

    LinkInterface* a = static_cast<LinkInterface*>(links.at(0));
    LinkInterface* b = static_cast<LinkInterface*>(links.at(1));
    LinkInterface* c = static_cast<LinkInterface*>(links.at(2));
    LinkInterface* d = static_cast<LinkInterface*>(links.at(3));
    QCOMPARE(a->getName(), QString("serial port COM 17"));
    QCOMPARE(b->getName(), QString("serial port COM 18"));
    QCOMPARE(c->getName(), QString("serial port COM 19"));
    QCOMPARE(d->getName(), QString("serial port COM 20"));

    LinkManager::instance()->removeLink(myLink4);
    delete myLink4;
    LinkManager::instance()->removeLink(myLink3);
    delete myLink3;
    LinkManager::instance()->removeLink(myLink2);
    delete myLink2;
    LinkManager::instance()->removeLink(myLink);
    delete myLink;

    QCOMPARE(LinkManager::instance()->getLinks().count(), 0);
}
