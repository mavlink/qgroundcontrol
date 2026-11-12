#pragma once

#include "UnitTest.h"

/// Tests what GPSConnectionSupervisor answers to sequences of what its owner tells it.
class GPSConnectionSupervisorTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _answers_data();
    void _answers();
};
