/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "UnitTest.h"

/// @file QGCTileCacheWorkerTest.h
/// @brief Unit tests for QGCTileCacheWorker database operations
/// Tests cover CRITICAL-1 (database validation), CRITICAL-2 (query lifecycle),
/// HIGH-4 (mapId validation), and general SQL safety

class QGCTileCacheWorkerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    // Database validation tests (CRITICAL-1)
    void _testDatabaseConnectionValidation();
    void _testDatabaseOpenStateChecking();
    void _testDatabaseErrorLogging();

    // SQL query lifecycle tests (CRITICAL-2)
    void _testQueryFinishBeforeRollback();
    void _testMultipleQueriesCleanup();
    void _testTransactionErrorPaths();

    // MapId validation tests (HIGH-4)
    void _testInvalidMapIdRejection();
    void _testNegativeMapIdHandling();
    void _testValidMapIdAcceptance();

    // Additional database safety tests
    void _testConcurrentDatabaseAccess();
    void _testDatabaseRecoveryAfterError();

    // Cache pruning tests
    void _testPruneTaskCreation();
    void _testPruneAmountValidation();
};
