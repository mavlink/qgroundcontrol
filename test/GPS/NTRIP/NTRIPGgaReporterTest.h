#pragma once

#include "UnitTest.h"

class NTRIPGgaReporterTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _sourceClearedOnStopAndFreshStart();
    void _providerMetadata_data();
    void _providerMetadata();
    void _gcsObservation_data();
    void _gcsObservation();
    void _ggaSourceSelection();
    void _ggaSelectionDiagnostics();
    void _ggaSourceChangesPreserveCadence();
    void _ggaIntervalChangesRestartCadence_data();
    void _ggaIntervalChangesRestartCadence();
    void _ggaConfigurationPreservesFastRetry();
    void _ggaFastRetryUntilFirstFix();
};
