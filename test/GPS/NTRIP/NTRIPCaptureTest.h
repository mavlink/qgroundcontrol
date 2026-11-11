#pragma once

#include "UnitTest.h"

/// Decodes what real casters sent (fixtures/*.cap, whose sources fixtures/SOURCES.md lists) and compares it with what
/// the pinned tools read from the same bytes (fixtures/NTRIPCaptureExpectations.h). Every capture is delivered whole,
/// byte by byte and in chunks of seven bytes.
class NTRIPCaptureTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _sourceTable_data();
    void _sourceTable();
    void _casterStream_data();
    void _casterStream();
    void _stationPositions();
};
