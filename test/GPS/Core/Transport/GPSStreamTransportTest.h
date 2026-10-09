#pragma once

#include "UnitTest.h"

class GPSStreamTransportTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _writePreconditions_data();
    void _writePreconditions();
    void _writeResultCounts_data();
    void _writeResultCounts();
    void _sharedWriterEvidence_data();
    void _sharedWriterEvidence();
};
