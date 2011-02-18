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

void SlugsMavUnitTest::getPwmCommands_test()
{
    mavlink_pwm_commands_t* k = slugsMav->getPwmCommands();
    k->aux1=80;

    mavlink_pwm_commands_t* k2 = slugsMav->getPwmCommands();
    k2->aux1=81;

    QCOMPARE(k->aux1, k2->aux1);
}

