#pragma once

#include "UnitTest.h"

/// The receiver's satellites in view: the model the receiver fills from its reports, and the signal bars and sky plot
/// the receiver status shows of it.
class GPSSatelliteViewTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _receiverListsItsSatellites();
    void _viewStaysInsideItsBox_data();
    void _viewStaysInsideItsBox();
    void _viewMarksSatellites();
};
