#pragma once

#include "UnitTest.h"

class QGCFileDownloadTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testFileDownload();
    void _testAutoDecompressGzip();
    void _testAutoDecompressDisabled();
    void _testAutoDecompressUncompressedFile();

    // Integration tests (download + compression)
    void _testDownloadAndExtractZip();
    void _testDownloadDecompressAndParse();

    // QGCCachedFileDownload tests
    void _testCachedFileDownloadConstruction();
    void _testCachedFileDownloadCacheDirectory();
    void _testCachedFileDownloadMaxCacheSize();
    void _testCachedFileDownloadEmptyUrl();
    void _testCachedFileDownloadNoCacheDir();
    void _testCachedFileDownloadDownload();
    void _testCachedFileDownloadPreferCache();
    void _testCachedFileDownloadNoCache();
    void _testCachedFileDownloadIsCached();
    void _testCachedFileDownloadCacheAge();
    void _testCachedFileDownloadClearCache();
    void _testCachedFileDownloadRemoveFromCache();
    void _testCachedFileDownloadCancel();
    void _testCachedFileDownloadConcurrentDownloadRejected();
    void _testCachedFileDownloadSignals();
};
