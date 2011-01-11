#include <QtCore/QString>
#include <QtTest/QtTest>
#include "UAS.h"
#include "MAVLinkProtocol.h"

class UASUnitTest : public QObject
{
    Q_OBJECT

public:
    UASUnitTest();

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void testCase1();
};

UASUnitTest::UASUnitTest()
{
}

void UASUnitTest::initTestCase()
{
  MAVLinkProtocol *mav= new MAVLinkProtocol();
  UAS *prueba=new UAS(mav,0);
}

void UASUnitTest::cleanupTestCase()
{
}

void UASUnitTest::testCase1()
{
    QVERIFY2(true, "Failure");
}

QTEST_APPLESS_MAIN(UASUnitTest);

#include "tst_uasunittest.moc"
