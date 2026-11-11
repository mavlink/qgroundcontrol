#pragma once

#include "UnitTest.h"

class QGCCachedFileDownloadTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testCachedFileDownloadConstruction();
    void _testCachedFileDownloadCacheDirectory();
    void _testCachedFileDownloadMaxCacheSize();
    void _testCachedFileDownloadRejectedStart_data();
    void _testCachedFileDownloadRejectedStart();
    void _testCachedFileDownloadNoCacheNeedsNoCacheDirectory();
    void _testCachedFileDownloadDownload();
    void _testCachedFileDownloadSyntheticCacheHit();
    void _testCachedFileDownloadCacheHitKeepsTimestamp();
    void _testCachedFileDownloadMaxAgeDecidesCacheOrNetwork_data();
    void _testCachedFileDownloadMaxAgeDecidesCacheOrNetwork();
    void _testCachedFileDownloadExpiredFallsBackToCache();
    void _testCachedFileDownloadStartForgetsFailedNetworkAttempt_data();
    void _testCachedFileDownloadStartForgetsFailedNetworkAttempt();
    void _testCachedFileDownloadNoCacheDoesNotFallBack();
    void _testCachedFileDownloadFailedFallbackReportsNetworkError();
    void _testCachedFileDownloadCachedPath();
    void _testCachedFileDownloadPreferCache();
    void _testCachedFileDownloadNoCache();
    void _testCachedFileDownloadIsCached();
    void _testCachedFileDownloadIsCachedInvalidTimestamp();
    void _testCachedFileDownloadCacheAge();
    void _testCachedFileDownloadClearCache();
    void _testCachedFileDownloadRemoveFromCache();
    void _testCachedFileDownloadCancel();
    void _testCachedFileDownloadConcurrentDownloadRejected();
    void _testCachedFileDownloadReentrantStartFromCompletion();
    void _testCachedFileDownloadSignals();
};
