#pragma once

#include "UnitTest.h"

class GPSConnectionStateTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _intentAndPause();
    void _pauseDuringRetryNotification();
    void _lifecycleAndRetry();
    void _stoppingBlocksAttempts();
    void _cancelDuringAdmission_data();
    void _cancelDuringAdmission();
    void _replacementAdmissionSurvivesRollback();
};
