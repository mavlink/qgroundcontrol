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

class QNetworkAccessManager;
class QGeoTileFetcherQGC;

/// Unit tests for QGeoTileFetcherQGC LoadBalancer integration
class QGeoTileFetcherQGCTest : public UnitTest
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    // Integration Tests
    void testLoadBalancerInitialization();
    void testProviderSelectionBasic();
    void testProviderSelectionWithFailover();
    void testHealthCheckBeforeFetch();
    void testAutomaticRetryOnError();
    void testRetryExponentialBackoff();
    void testMaxRetriesReached();
    void testConcurrentRequestLimiting();
    void testMetricsTracking();
    void testConfigurationPersistence();
    void testConfigurationLoad();

    // Edge Cases
    void testAllProvidersDisabled();
    void testAllProvidersUnhealthy();
    void testNullNetworkManager();
    void testInvalidMapId();
    void testCorruptedSettings();
    void testProviderWeightBounds();

    // Thread Safety
    void testConcurrentTileFetches();
    void testProviderAccessRaceCondition();

    // Memory Safety
    void testNoMemoryLeaksOnSuccess();
    void testNoMemoryLeaksOnError();
    void testProviderLifecycleSafety();

    // Performance
    void testLoadBalancerPerformance();
    void testProviderSelectionPerformance();

private:
    QNetworkAccessManager* _networkManager = nullptr;
    QGeoTileFetcherQGC* _tileFetcher = nullptr;
};
