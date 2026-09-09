#pragma once

#include "UnitTest.h"

class NMEAPositionSourceTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _fixMetadata_data();
    void _fixMetadata();
    void _metadataDoesNotCrossEpochs();
    void _restartClearsParserState();
    void _pendingRequestSurvivesStopAndStart();
    void _requestTimeoutAndRecovery();
    void _queuedUpdateCannotSurviveRestart();
};
