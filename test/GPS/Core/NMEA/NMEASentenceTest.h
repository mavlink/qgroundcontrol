#pragma once

#include "UnitTest.h"

class NMEASentenceTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _frameValidation_data();
    void _frameValidation();
    void _utcMilliseconds_data();
    void _utcMilliseconds();
    void _qtTimestampEquivalence();
    void _makeGga_data();
    void _makeGga();
    void _repairChecksum_data();
    void _repairChecksum();
    void _incrementalFraming_data();
    void _incrementalFraming();
    void _incrementalReset();
    void _lineFraming_data();
    void _lineFraming();
    void _navigationFreshnessBoundaries();
    void _navigationFixLoss_data();
    void _navigationFixLoss();
    void _fixQuality_data();
    void _fixQuality();
    void _ggaHdopValidation_data();
    void _ggaHdopValidation();
    void _gstFieldCounts_data();
    void _gstFieldCounts();
    void _satelliteIds_data();
    void _satelliteIds();
    void _coordinateNumbers_data();
    void _coordinateNumbers();
    void _numberFields_data();
    void _numberFields();
    void _nmeaWireContract();
    void _ggaValidity_data();
    void _ggaValidity();
    void _navigationStatus_data();
    void _navigationStatus();
    void _fixtureSentences();
};
