#pragma once

#include "BaseClasses/TempDirectoryTest.h"

class QGCCachedFileDownloadTest : public TempDirectoryTest
{
    Q_OBJECT

private slots:
    void _testCachedFileDownloadConstruction();
    void _testCachedFileDownloadCacheDirectory();
    void _testCachedFileDownloadMaxCacheSize();
    void _testCachedFileDownloadEmptyUrl();
    void _testCachedFileDownloadNoCacheDir();
    void _testCachedFileDownloadDownload();
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
    void _testCachedFileDownloadRejectedStartNoFinishedSignal();
    void _testCachedFileDownloadReentrantStartFromCompletion();
    void _testCachedFileDownloadSignals();
};
