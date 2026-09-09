#pragma once

#include "UnitTest.h"

class GPSDriverUBXTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _configurationReport_data();
    void _configurationReport();
    void _receiverSettings_data();
    void _receiverSettings();
    void _managedNmeaKeepsTransportUntilStopped();
    void _correctionBacklogPublishesBufferedPositions();
    void _positionMode_data();
    void _positionMode();
    void _unsupportedBaseDoesNotWriteConfiguration();
    void _surveyRestart_data();
    void _surveyRestart();
    void _readFailure_data();
    void _readFailure();
    void _surveyReadFailure_data();
    void _surveyReadFailure();
    void _commsDiagnostics();
    void _invalidCommsDiagnostics_data();
    void _invalidCommsDiagnostics();
};
