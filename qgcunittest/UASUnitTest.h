#ifndef UASUNITTEST_H
#define UASUNITTEST_H

#include <QObject>
#include <QtCore/QString>
#include <QtTest/QtTest>
#include "UAS.h"
#include "MAVLinkProtocol.h"
#include "SerialLink.h"
#include "UASInterface.h"
#include "AutoTest.h"

class UASUnitTest : public QObject
{
    Q_OBJECT
public:
#define  UASID  50
    MAVLinkProtocol* mav;
    UAS* uas;
    SerialLink* link;
    UASUnitTest();

signals:

private slots:
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
    void attitudeLimitsZero_test();
    void attitudeLimitsPi_test();
    void attitudeLimitsMinusPi_test();
    void attitudeLimitsTwoPi_test();
    void attitudeLimitsMinusTwoPi_test();
    void attitudeLimitsTwoPiOne_test();
    void attitudeLimitsMinusTwoPiOne_test();

protected:
    UAS *prueba;

};

DECLARE_TEST(UASUnitTest)
#endif // UASUNITTEST_H
