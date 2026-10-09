#pragma once

#include "UnitTest.h"

/// The developer recording tee keeps the link working when its files fail or reach their size cap.
class GPSRecordingTransportTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _fileErrorKeepsLink_data();
    void _fileErrorKeepsLink();
    void _filesStopAtSizeCap();
};
