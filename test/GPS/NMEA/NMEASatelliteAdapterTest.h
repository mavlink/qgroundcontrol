#pragma once

#include "UnitTest.h"

class NMEASatelliteAdapterTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _reentrantStopKeepsReplacement();
    void _identicalReportsAndGsaWithoutView();
    void _qtLegacyEquivalence();
    void _gsvDoesNotClearFreshUsedReport();
    void _constellationsExpireIndependently();
    void _decoderKeepsFreshConstellation();
    void _preservesReceiptAgeAcrossReports();
    void _decoderRejectsDelayedSatelliteBatch();
    void _modernConstellationsAndSignals();
    void _incompleteReportIsDiscarded();
    void _idleBatchAndSourceClose();
};
