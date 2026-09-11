#pragma once

#include "UnitTest.h"

class GPSReceiverPositionSourceTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _convertsFixAndMotion();
    void _validatesFix_data();
    void _validatesFix();
    void _requestsAndReset();
    void _pendingRequestKeepsDeadline();
    void _reportsLossOnceUntilRecovery();
    void _intervalCoalescesLatestFix();
    void _intervalChangesWhileStarted();
    void _requestBypassesInterval();
    void _stopPreservesRequest();
    void _resetDiscardsPendingUpdate();
    void _intervalRejectsStaleFix();
    void _silentIntervalsReportLossOnce();
};
