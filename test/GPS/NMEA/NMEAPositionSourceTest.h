#pragma once

#include "UnitTest.h"

class NMEAPositionSourceTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _fixLoss_data();
    void _fixLoss();
    void _lateFixLossDoesNotRejectRecovery();
    void _fixLossPreservesPendingRequest();
    void _fixLossCanDestroySource();
    void _gllRecoversFromFixLoss();
    void _schedulerCanBeDestroyed();
    void _gstAccuracy_data();
    void _gstAccuracy();
    void _fixMetadata_data();
    void _fixMetadata();
    void _metadataDoesNotCrossEpochs();
    void _restartClearsParserState();
    void _pendingRequestSurvivesStopAndStart();
    void _requestTimeoutAndRecovery();
    void _queuedUpdateCannotSurviveRestart();
};
