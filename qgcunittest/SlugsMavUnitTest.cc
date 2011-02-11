#include "SlugsMavUnitTest.h"

SlugsMavUnitTest::SlugsMavUnitTest()
{
}

void SlugsMavUnitTest::initTestCase()
{
    mav = new MAVLinkProtocol();
    slugsMav = new SlugsMAV(mav, UASID);
}

void SlugsMavUnitTest::cleanupTestCase()
{
    delete slugsMav;
    delete mav;
}

void SlugsMavUnitTest::first_test()
{
  QCOMPARE(1,1);
}

