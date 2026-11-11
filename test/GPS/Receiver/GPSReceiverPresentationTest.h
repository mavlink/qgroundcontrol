#pragma once

#include "UnitTest.h"

/// What the receiver views offer for each receiver family, for Automatic and for an unknown manufacturer.
class GPSReceiverPresentationTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _presentation_data();
    void _presentation();
};
