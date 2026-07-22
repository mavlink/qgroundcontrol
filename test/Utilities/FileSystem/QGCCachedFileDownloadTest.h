#pragma once

#include "UnitTest.h"

class QGCCachedFileDownloadTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testCachedFileDownloadConstruction();
    void _testCachedFileDownloadCacheDirectory();
    void _testCachedFileDownloadMaxCacheSize();
    void _testCachedFileDownloadEmptyUrl();
    void _testCachedFileDownloadNoCacheDir();
    void _testCachedFileDownloadDownload();
    void _testCachedFileDownloadMaximumSize();
    void _testCachedFileDownloadSyntheticCacheHit();
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
    void _testCachedFileDownloadStartupReentrancyRejected();
    void _testCachedFileDownloadRejectedStartNoFinishedSignal();
    void _testCachedFileDownloadReentrantStartFromCompletion();
    void _testCachedFileDownloadDeferredStartFailureCompletes();
    void _testCachedFileDownloadReentrantStartFromRunningChanged();
    void _testCachedFileDownloadCancelFromRunningChanged();
    void _testCachedFileDownloadDirectConnectionDeletion();
    void _testCachedFileDownloadFallbackStartError();
    void _testCachedFileDownloadFallbackCancellationAndDeletion();
    void _testCachedFileDownloadSignals();
};
