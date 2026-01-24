#pragma once

#include "TestFixtures.h"

/// Tests for LogDownloadController.
/// Tests controller creation, properties, and state management.
/// Uses VehicleTest since it needs a connected vehicle.
class LogDownloadTest : public VehicleTest
{
    Q_OBJECT

public:
    LogDownloadTest() = default;

private slots:
    // Controller creation tests
    void _testControllerCreation();
    void _testControllerParent();

    // Model tests
    void _testModelExists();
    void _testModelInitiallyEmpty();

    // Initial state tests
    void _testInitialRequestingListFalse();
    void _testInitialDownloadingLogsFalse();
    void _testInitialCompressLogsFalse();
    void _testInitialCompressingFalse();
    void _testInitialCompressionProgressZero();

    // CompressLogs property tests
    void _testSetCompressLogsTrue();
    void _testSetCompressLogsFalse();
    void _testCompressLogsSignal();

    // Signal existence tests
    void _testRequestingListChangedSignal();
    void _testDownloadingLogsChangedSignal();
    void _testSelectionChangedSignal();
    void _testCompressingChangedSignal();
    void _testCompressionProgressChangedSignal();
    void _testCompressionCompleteSignal();

    // Q_INVOKABLE method tests
    void _testRefreshMethod();
    void _testCancelMethod();
    void _testCancelCompressionMethod();
};
