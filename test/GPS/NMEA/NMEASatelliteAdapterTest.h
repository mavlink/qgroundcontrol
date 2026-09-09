#pragma once

#include "UnitTest.h"

class NMEASatelliteAdapterTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _modernConstellationsAndSignals();
    void _incompleteReportIsDiscarded();
    void _idleBatchAndSourceClose();
};
