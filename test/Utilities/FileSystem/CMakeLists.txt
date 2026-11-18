add_qgc_tests(QGCArchiveWatcherTest LABELS Unit Utilities)
add_qgc_tests(
    QGCCachedFileDownloadTest
    QGCFileDownloadTest
    LABELS Integration Network Utilities
)
add_qgc_tests(
    QGCFileHelperTest
    QGCFileWatcherTest
    QGCFileWriterTest
    LABELS Unit Utilities
)
