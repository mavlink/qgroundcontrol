#pragma once

#include "TestFixtures.h"

class QGCCachedFileDownload;

/// Unit test for QGCFileDownload.
/// Uses TempDirTest for automatic temp directory management.
class QGCFileDownloadTest : public TempDirTest
{
    Q_OBJECT

public:
    QGCFileDownloadTest() = default;

private slots:

    // QGCFileDownload tests
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

private:
    /// Creates a cached file downloader using the test's temp directory
    QGCCachedFileDownload* _createCachedDownloader();
};
