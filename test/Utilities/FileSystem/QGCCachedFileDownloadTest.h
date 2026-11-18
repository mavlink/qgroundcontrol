#pragma once

#include "UnitTest.h"

class QGCCachedFileDownloadTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testCachedFileDownloadConstruction();
    void _testCachedFileDownloadCacheDirectory();
    void _testCachedFileDownloadMaxCacheSize();
    UT_PARAMETERIZED_TEST(_testCachedFileDownloadRejectedStart);
    UT_PARAMETERIZED_TEST(_testCachedFileDownloadDownload);
    void _testCachedFileDownloadSyntheticCacheHit();
    void _testCachedFileDownloadCacheHitKeepsTimestamp();
    UT_PARAMETERIZED_TEST(_testCachedFileDownloadMaxAgeDecidesCacheOrNetwork);
    void _testCachedFileDownloadExpiredFallsBackToCache();
    UT_PARAMETERIZED_TEST(_testCachedFileDownloadStartForgetsFailedNetworkAttempt);
    void _testCachedFileDownloadNoCacheDoesNotFallBack();
    void _testCachedFileDownloadFailedFallbackReportsNetworkError();
    void _testCachedFileDownloadCachedPath();
    void _testCachedFileDownloadEmptyCache();
    void _testCachedFileDownloadIsCachedInvalidTimestamp();
    void _testCachedFileDownloadCancel();
    void _testCachedFileDownloadConcurrentDownloadRejected();
    void _testCachedFileDownloadReentrantStartFromCompletion();
};
