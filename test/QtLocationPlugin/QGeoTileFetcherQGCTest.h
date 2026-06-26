#pragma once

#include "QtLocationTestBase.h"

class QGeoTileFetcherQGCTest : public QtLocationTestBase
{
    Q_OBJECT

private slots:
    void init();

    void _testConcurrentDownloadsConstant();
    void _testGetNetworkRequestInvalidMapId();
    void _testGetNetworkRequestValidMapBasics();
    void _testGetNetworkRequestAttributes();
    void _testDedupCoalescesSameUrl();
    void _testDedupSeparateUrlsDistinct();
    void _testParseExpirySMaxAgeFallback();
    void _testParseExpiryMaxAgeWinsOverSMaxAge();
    void _testParseExpirySMaxAgeZeroIsNotCached();
    void _testConcurrencyGateCapsInFlight();
    void _testHostConcurrencyGateSharedCap();
    void _testSnapshotParsesMustRevalidate();
};
