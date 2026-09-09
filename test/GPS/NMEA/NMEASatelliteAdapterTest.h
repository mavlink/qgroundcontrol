#pragma once

#include "UnitTest.h"

class NMEASatelliteAdapterTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _constellationsExpireIndependently();
    void _decoderKeepsFreshConstellation();
    void _preservesReceiptAgeAcrossReports();
    void _decoderRejectsDelayedSatelliteBatch();
    void _modernConstellationsAndSignals();
    void _incompleteReportIsDiscarded();
    void _idleBatchAndSourceClose();
};
