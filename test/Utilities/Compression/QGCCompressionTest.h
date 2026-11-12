#pragma once

#include "UnitTest.h"

/// Tests for QGCCompression (decompression-only: format detection, extraction, decompression)
class QGCCompressionTest : public UnitTest
{
    Q_OBJECT

private slots:
    // Format detection
    void _testFormatDetection_data();
    void _testFormatDetection();
    void _testDetectFormatFromFile_data();
    void _testDetectFormatFromFile();
    void _testDetectFormatFromMagicBytes_data();
    void _testDetectFormatFromMagicBytes();
    void _testFormatDetectionFromContent();
    void _testFormatHelpers();

    // Archive extraction from Qt resources
    void _testZipFromResource();
    void _test7zFromResource();
    void _testExtractArchiveAtomic();
    void _testExtractArchiveAtomicReplacesExistingDirectory();
    void _testExtractArchiveAtomicFailureKeepsExistingDirectory();
    void _testListArchive();
    void _testListArchiveDetailed();
    void _testListArchiveNaturalSort();
    void _testGetArchiveStats();
    void _testValidateArchive();
    void _testFileExists();
    void _testExtractArchiveFiltered();
    void _testExtractSingleFile();
    void _testExtractFileData();
    void _testExtractMultipleFiles();
    void _testExtractByPattern();

    // Single-file decompression from Qt resources
    void _testDecompressFromResource();
    void _testDecompressData();
    void _testDecompressIfNeeded();
    void _testDecompressIfNeededDefaultOutput_data();
    void _testDecompressIfNeededDefaultOutput();

    // Progress callbacks
    void _testProgressCallbackExtract_data();
    void _testProgressCallbackExtract();

    // Format reported for the last operation
    void _testDetectedFormat_data();
    void _testDetectedFormat();
    void _testDetectedFormatAfterFailure_data();
    void _testDetectedFormatAfterFailure();

    // Edge cases and error handling
    void _testCorruptArchive();
    void _testNonExistentInput();
    void _testNoExternalDecompressors();
    void _testTruncatedArchive_data();
    void _testTruncatedArchive();
    void _testTruncatedStream_data();
    void _testTruncatedStream();
    void _testExtractStreamedZipEntry();
    void _testExtractFileDataErrors_data();
    void _testExtractFileDataErrors();
    void _testDecompressDataErrors();

    // QIODevice-based operations
    void _testDecompressFromDevice();
    void _testExtractFromDevice();
    void _testExtractFileDataFromDevice();
    void _testInterleavedDeviceReaders();
    void _testDeviceAtEnd_data();
    void _testDeviceAtEnd();

    // QGCCompressionJob (async operations)
    void _testCompressionJobExtract();
    void _testCompressionJobReentrantStartFromFinished();
    void _testCompressionJobCancel_data();
    void _testCompressionJobCancel();
    void _testCompressionJobAsyncStatic();
    void _testCompressionJobCancelFuture();
    void _testCompressionJobErrorString();

    // Sparse file handling
    void _testSparseFileExtraction_data();
    void _testSparseFileExtraction();

    // Thread safety
    void _testThreadLocalState();
    void _testConcurrentExtractions();

    // Boundary conditions
    void _testEmptyArchive();
    void _testSingleByteFile();
    void _testLargeFileDecompression();
    void _testDecompressToFileSizeLimit();
    void _testDecompressToMemorySizeLimit();
    void _testExtractionStopsCleanly_data();
    void _testExtractionStopsCleanly();
    void _testExtractArchiveKeepsEntriesBeforeDamage_data();
    void _testExtractArchiveKeepsEntriesBeforeDamage();
    void _testMaxEntriesArchive();
    void _testOverstatedEntrySize();
    void _testExtractFileDataSizeLimit_data();
    void _testExtractFileDataSizeLimit();

    // Cross-platform path handling
    void _testWindowsPathSeparators();
    void _testExtractArchiveToSymlinkedOutputPath();
    void _testExtractRejectsEscapingNames_data();
    void _testExtractRejectsEscapingNames();
    void _testExtractRejectsEscapingLinks();
    void _testExtractLeavesOutLinks_data();
    void _testExtractLeavesOutLinks();
    void _testSpecialCharactersInPath();
    void _testUnicodePaths();

    // Benchmarks (run with --benchmark flag)
    void _benchmarkExtractZip();
    void _benchmarkExtract7z();
    void _benchmarkDecompressGzip();
    void _benchmarkDecompressXz();
    void _benchmarkConcurrentExtraction();
    void _benchmarkListArchive();

private:
    bool _compareFiles(const QString& file1, const QString& file2);

    static quint32 _crc32(const QByteArray& data);

    static bool _writeFile(const QString& path, const QByteArray& data);
    static QByteArray _readFile(const QString& path);

    /// Paths of everything below @a directoryPath, relative to it and sorted
    static QStringList _entriesBelow(const QString& directoryPath);

    /// One member of a gzip file, holding @a payload
    static QByteArray _gzipMember(const QByteArray& payload);

    /// Writes a gzip file of @a firstMember followed by @a memberCount members, each expanding to 64 KiB of zeros
    static bool _writeGzipOfZeros(const QString& path, int memberCount, const QByteArray& firstMember = {});

    /// Part of a sparse file that is stored in the archive
    struct TarSparseData
    {
        qint64 offset = 0;
        qint64 size = 0;
    };

    /// Entry of a hand-built tar archive
    struct TarEntry
    {
        QByteArray name = {};
        QByteArray data = {};
        char type = '0';  ///< '0' file, '1' hard link, '2' symbolic link, '5' directory, 'S' old GNU sparse file
        QByteArray linkName = {};
        QList<TarSparseData> sparseData = {};  ///< For 'S': where the data belongs in the file, at most four parts
        qint64 sparseFileSize = 0;             ///< For 'S': size of the file with its holes
    };

    /// Entry of a hand-built ZIP archive
    struct ZipEntry
    {
        QByteArray name = {};
        QByteArray data = {};
    };

    /// Builds a ZIP archive as it is written to a stream: the sizes of an entry follow its data instead of being
    /// in its local header. Without @a endRecord the central directory cannot be found, so that a reader which
    /// can seek does not learn the sizes from there either.
    static QByteArray _makeStreamedZip(const QList<ZipEntry>& entries, bool endRecord);

    /// Header block of a tar entry whose data is @a dataSize bytes long
    static QByteArray _tarHeader(const TarEntry& entry, qint64 dataSize);

    /// Builds a tar archive, so that tests can make archives libarchive's writer would refuse to
    static QByteArray _makeTar(const QList<TarEntry>& entries);
};
