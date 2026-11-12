#pragma once

#include "UnitTest.h"

/// The Position Receiver section of the GNSS Receiver settings, over settings of its own and a scripted receiver that
/// follows them, and the position receiver's status in the GPS indicator's drop-down.
class GPSPositionReceiverTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _showsConnectionOnceInUse();
    void _connectsAndDisconnectsFromSection();
    void _dropDownShowsStatusWhileInUse();
};
