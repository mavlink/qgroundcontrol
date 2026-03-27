#pragma once

#include "UnitTest.h"

class VehicleGPSFactGroupTest : public UnitTest
{
    Q_OBJECT

private slots:
    void testInitialValues();
    void testGpsRawInt();
    void testHighLatency2();
    void testGpsRtk();
    void testGps2Raw();
    void testGps2Rtk();
    void testGnssIntegrity();
    void testGnssIntegrityFiltering();
    void testSentinelValues();
    void testHighLatency();
    void testRtkReceiverIdRejection();
    void testQualityNone();
    void testQualityPoor();
    void testQualityGoodHdopAndSats();
    void testQualityExcellentRtkFixed();
    void testMavlinkCoordinateHelpers();
};
