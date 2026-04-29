#pragma once

#include "UnitTest.h"

/// Tests for QGCKeychain secure credential storage utility
class QGCKeychainTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testWriteReadRoundTrip();
    void _testOverwriteExistingKey();
    void _testReadNonexistentReturnsEmpty();
    void _testRemoveExistingKey();
    void _testRemoveNonexistentKey();
    void _testEmptyData();
    void _testLargeData();
    void _testMultipleKeysCoexist();
    void _testBinaryDataWithNulls();
    void _testBackendProbeAndRoundTrip();
};
