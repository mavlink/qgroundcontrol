#pragma once

#include "UnitTest.h"

class GPSConnectionControlTest : public UnitTest
{
    Q_OBJECT
private slots:
    void _reentrantProfileReplacement_data();
    void _reentrantProfileReplacement();
    void _retryScheduling_data();
    void _retryScheduling();
    void _queuedCommandsRetireWithOwner();
};
