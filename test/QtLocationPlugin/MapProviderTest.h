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

class MapProviderTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testCoordinateValidation();
    void _testCapabilities();
    void _testURLCaching();
    void _testHealthTracking();
    void _testMetadata();
    void _testRateLimiting();
    void _testTileValidation();
    void _testFactory();
    void _testFallbackChain();
    void _testCoordinateCache();
    void _testMemoryMonitoring();
    void _testErrorTracking();
    void _testHealthDashboard();
    void _testEnhancedFormatDetection();
    void _testMetadataValidation();
    void _testAttributionManagement();
    void _testGeographicBounds();
    void _testTileCoordinateValidation();
    void _testTileSizeValidation();

    // Phase 5 & Refactoring tests
    void _testThreadSafetyURLCache();
    void _testThreadSafetyHealth();
    void _testThreadSafetyRateLimiter();
    void _testRequestGuardRAII();
    void _testConcurrentRequestLimiter();
    void _testRetryIntegration();
    void _testMetricsIntegration();
    void _testIntegrationWorkflow();
    void _testMapProviderManager();

    // Phase 7 - Provider Rotation & Load Balancing tests
    void _testProviderRotationRoundRobin();
    void _testProviderRotationWeightedHealth();
    void _testProviderRotationLeastConnections();
    void _testProviderRotationRandom();
    void _testLoadBalancerBasic();
    void _testLoadBalancerAutoFailover();
    void _testLoadBalancerStatistics();
};
