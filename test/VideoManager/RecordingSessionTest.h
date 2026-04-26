#pragma once

#include "UnitTest.h"

class RecordingSessionTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testStartCreatesManifest();
    void _testStopCleansUpManifest();
    void _testScanForOrphansFindsIncomplete();
    void _testRecordingPolicyOwnsFormatAndStorageRules();
};
