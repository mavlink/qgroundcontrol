#include <QtCore/QString>
#include <QtTest/QtTest>
#include "UAS.h"
#include "MAVLinkProtocol.h"
#include "UASInterface.h"

class UASUnitTest : public QObject
{
    Q_OBJECT

public:
    #define  UASID  50
    MAVLinkProtocol* mav;
    UAS* uas;
    UASUnitTest();

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
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

private:

protected:
    UAS *prueba;
};

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

QTEST_APPLESS_MAIN(UASUnitTest);

#include "tst_uasunittest.moc"
