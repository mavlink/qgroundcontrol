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

QTEST_APPLESS_MAIN(UASUnitTest);

#include "tst_uasunittest.moc"
