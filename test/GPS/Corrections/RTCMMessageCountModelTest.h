#pragma once

#include "UnitTest.h"

/// The per-message counts and intervals of a correction stream, as the item model a view shows.
class RTCMMessageCountModelTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _messageCountsChangeInPlace();
    void _messageIntervals();
};
