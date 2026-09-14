#pragma once

#include "UnitTest.h"

class NMEASatelliteAdapterTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _gsvFields_data();
    void _gsvFields();
    void _combinedTalkerReports_data();
    void _combinedTalkerReports();
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
    void _qtLegacyEquivalence_data();
    void _qtLegacyEquivalence();
    void _gsvDoesNotClearFreshUsedReport();
    void _constellationsExpireIndependently();
    void _decoderKeepsFreshConstellation();
    void _preservesReceiptAgeAcrossReports();
    void _decoderRejectsDelayedSatelliteBatch();
    void _modernConstellationsAndSignals();
    void _incompleteReportIsDiscarded();
    void _incompleteReportIsDiscarded_data();
    void _epochTimeNormalization_data();
    void _epochTimeNormalization();
    void _idleBatchAndSourceClose();
};
