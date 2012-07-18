#ifndef UASUNITTEST_H
#define UASUNITTEST_H

#include <QObject>
#include <QtCore/QString>
#include <QtTest/QtTest>
#include <QApplication>

#include "UAS.h"
#include "MAVLinkProtocol.h"
#include "SerialLink.h"
#include "UASInterface.h"
#include "AutoTest.h"
#include "LinkManager.h"
#include "UASWaypointManager.h"
#include "SerialLink.h"
#include "LinkInterface.h"

class UASUnitTest : public QObject
{
    Q_OBJECT
public:
  #define  UASID  100
  MAVLinkProtocol* mav;
  UAS* uas;
  UASUnitTest();

private slots:
  void init();
  void cleanup();
  
  void getUASID_test();
  void getUASName_test();
  void getUpTime_test();
  void getCommunicationStatus_test();
  void filterVoltage_test();
  void getAutopilotType_test();
  void setAutopilotType_test();
  void getStatusForCode_test();
  void getLocalX_test();
  void getLocalY_test();
  void getLocalZ_test();
  void getLatitude_test();
  void getLongitude_test();
  void getAltitude_test();
  void getRoll_test();
  void getPitch_test();
  void getYaw_test();
  void getSelected_test();
  void getSystemType_test();
  void getAirframe_test();
  void setAirframe_test();
  void getWaypointList_test();
  void signalWayPoint_test();
  void getWaypoint_test();
  void signalUASLink_test();
  void signalIdUASLink_test();
};

DECLARE_TEST(UASUnitTest)
#endif // UASUNITTEST_H
