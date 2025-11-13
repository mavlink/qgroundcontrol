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

class QGCMapEngineManagerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();
    void _testCachePauseReferenceCount();
    void _testCachePauseNestedCalls();
    void _testCachePauseUnbalancedResume();
    void _testDefaultCacheControl();
    void _testDownloadMetricsAggregation();
    void _testTileSetUpdate();
    void _testGlobalPauseDispatchesToTileSets();
    void _testUserPauseDoesNotClearSystemPause();
    void _testRapidPauseResumeCycles();
};
