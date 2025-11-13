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

/// @file
/// @brief Integration test for tile download, caching, and database workflow
/// @author QGroundControl Development Team

class TileDownloadIntegrationTest : public UnitTest
{
    Q_OBJECT

private slots:
    /// Test creating a tile set and verifying database structure
    void _testTileSetCreation();

    /// Test download state transitions (Pending -> Downloading -> Complete)
    void _testDownloadStateTransitions();

    /// Test pause and resume functionality
    void _testPauseResume();

    /// Test error handling and retry logic
    void _testErrorHandling();

    /// Test cache enable/disable functionality
    void _testCacheControl();

    /// Test concurrent tile set downloads
    void _testConcurrentDownloads();

    /// Test tile set deletion and cleanup
    void _testTileSetDeletion();

    /// Test download stats aggregation across multiple tile sets
    void _testDownloadStatsAggregation();

    /// Test pause vs cancel behavior differences
    void _testPauseVsCancelBehavior();

    /// Test download status string generation
    void _testDownloadStatusStrings();

    /// Test default set special behavior
    void _testDefaultSetBehavior();

    /// Test network error count tracking
    void _testNetworkErrorTracking();

    /// Test error tile state transitions
    void _testErrorTileStateHandling();
};
