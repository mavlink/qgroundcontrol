#pragma once

#include "UnitTest.h"

class NTRIPGgaProviderTest : public UnitTest
{
    Q_OBJECT

private slots:
    void testSourceClearedOnStopAndFreshStart();
    void _invalidProviderAltitude_data();
    void _invalidProviderAltitude();
    void testRTKReceiverProvider();
    void _providerMetadata_data();
    void _providerMetadata();
    void _gcsObservation_data();
    void _gcsObservation();
    void ggaAltitudeDatum_data();
    void ggaAltitudeDatum();
    void ggaSourceSelection();
    void ggaSelectionDiagnostics();
    void ggaDiagnosticRetiresProvider_data();
    void ggaDiagnosticRetiresProvider();
    void ggaSourceChangesPreserveCadence();
    void ggaIntervalChangesRestartCadence_data();
    void ggaIntervalChangesRestartCadence();
    void ggaConfigurationPreservesFastRetry();
    void ggaCallbackStopsProvider();

private:
    void _expectDebugMessage(const char* category, const QString& message);
    void _verifyDebugMessage();
};
