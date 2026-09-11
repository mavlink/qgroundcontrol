#pragma once

#include "UnitTest.h"

class NMEASatelliteAdapterTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _schedulerCanBeDestroyed();
    void _canonicalIdentities();
    void _identityResolution_data();
    void _identityResolution();
    void _mixedLegacyIdentities_data();
    void _mixedLegacyIdentities();
    void _ambiguousLegacyIdentities_data();
    void _ambiguousLegacyIdentities();
    void _explicitZeroAndUnknownCoverage();
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
