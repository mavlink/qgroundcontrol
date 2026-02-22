#pragma once

#include "BaseClasses/VehicleTest.h"

class LogDownloadTest : public VehicleTestNoInitialConnect
{
    Q_OBJECT

private slots:
    void _downloadTest();
    void _downloadWithGapsTest();
    void _downloadMultipleGapsTest();
    void _downloadStopMidstreamTest();
    void _downloadEfficientGapRetryTest();
};
