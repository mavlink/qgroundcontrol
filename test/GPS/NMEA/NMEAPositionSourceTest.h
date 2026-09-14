#pragma once

#include "UnitTest.h"

class NMEAPositionSourceTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _dateOrdering_data();
    void _dateOrdering();
    void _fixLoss_data();
    void _fixLoss();
    void _lateFixLossDoesNotRejectRecovery_data();
    void _lateFixLossDoesNotRejectRecovery();
    void _fixLossPreservesPendingRequest();
    void _fixLossCanDestroySource();
    void _gllRecoversFromFixLoss();
    void _schedulerCanBeDestroyed();
    void _gstAccuracy_data();
    void _gstAccuracy();
    void _fixMetadata_data();
    void _fixMetadata();
    void _fixDimensionOrdering_data();
    void _fixDimensionOrdering();
    void _bufferedRequest_data();
    void _bufferedRequest();
    void _metadataDoesNotCrossEpochs();
    void _restartClearsParserState();
    void _pendingRequestSurvivesStopAndStart();
    void _requestTimeoutAndRecovery();
    void _queuedUpdateCannotSurviveRestart();
};
