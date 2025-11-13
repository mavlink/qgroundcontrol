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

class QGCCachedTileSetTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testDownloadStatsUpdates();
    void _testRetryFailedTilesConcurrentGuard();
    void _testErrorCountReset();
    void _testDownloadProgressCalculation();
    void _testDownloadProgressEdgeCases();
    void _testCopyFrom();
    void _testCopyFromNull();
    void _testCopyFromSelf();
    void _testCompleteStateTransitions();
    void _testDefaultSetCompleteState();
    void _testTotalAndSavedTileCountSignals();
    void _testConcurrentPrepareDownload();
    void _testElevationProviderCastValidation();
    void _testMapIdValidation();
};
