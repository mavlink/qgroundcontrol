#pragma once

#include "QGCCompression.h"
#include "UnitTest.h"

/// Tests for QGCCompression (decompression-only: format detection, extraction, decompression)
class QGCCompressionTest : public UnitTest
{
    Q_OBJECT

private slots:
    // Format detection
    UT_PARAMETERIZED_TEST(_testFormatDetection);
    UT_PARAMETERIZED_TEST(_testDetectFormatFromFile);
    UT_PARAMETERIZED_TEST(_testDetectFormatFromMagicBytes);
    void _testFormatDetectionFromContent();
    void _testFormatHelpers();
    void _testFormatParameter();

    // Archive extraction from Qt resources
    UT_PARAMETERIZED_TEST(_testExtractFromResource);
    void _testExtractArchiveAtomicReplacesExistingDirectory();
    void _testExtractArchiveAtomicFailureKeepsExistingDirectory();
    UT_PARAMETERIZED_TEST(_testListOneFile);
    void _testListArchiveNaturalSort();
    void _testValidateArchive();
    void _testFileExists();
    void _testExtractArchiveFiltered();
    void _testExtractSingleFile();
    void _testExtractFileData();
    void _testExtractMultipleFiles();
    void _testExtractByPattern();

    // Single-file decompression from Qt resources
    UT_PARAMETERIZED_TEST(_testDecompressEveryFormat);
    void _testDecompressIfNeeded();
    UT_PARAMETERIZED_TEST(_testDecompressIfNeededDefaultOutput);
    UT_PARAMETERIZED_TEST(_testDecompressEmptyStream);
    UT_PARAMETERIZED_TEST(_testDecompressRejectsStreamWithoutData);
    UT_PARAMETERIZED_TEST(_testDecompressPlainInput);
    void _testComputeFileHashOfUncompressedFile();
    UT_PARAMETERIZED_TEST(_testDecompressFileCopiesPlainInput);

    // Progress callbacks
    UT_PARAMETERIZED_TEST(_testProgressCallbackExtract);

    // Format reported by an operation
    UT_PARAMETERIZED_TEST(_testDetectedFormat);
    UT_PARAMETERIZED_TEST(_testDetectedFormatAfterFailure);

    // Edge cases and error handling
    UT_PARAMETERIZED_TEST(_testMissingInput);
    void _testNoExternalDecompressors();
    UT_PARAMETERIZED_TEST(_testTruncatedArchive);
    UT_PARAMETERIZED_TEST(_testTruncatedStream);
    void _testExtractStreamedZipEntry();
    UT_PARAMETERIZED_TEST(_testSequentialDeviceEnd);
    void _testSequentialDeviceReadError();
    void _testSequentialDeviceSkip();
    UT_PARAMETERIZED_TEST(_testExtractFileDataErrors);
    UT_PARAMETERIZED_TEST(_testExtractToFileErrors);
    void _testDecompressDataErrors();
    UT_PARAMETERIZED_TEST(_testUnwritableOutput);
    UT_PARAMETERIZED_TEST(_testInputWithoutPermission);
    UT_PARAMETERIZED_TEST(_testOutcomeBeforeInput);

    // QIODevice-based operations
    void _testDecompressFromDevice();
    void _testExtractFromDevice();
    void _testExtractFileDataFromDevice();
    void _testInterleavedDeviceReaders();
    UT_PARAMETERIZED_TEST(_testDeviceAtEnd);
    UT_PARAMETERIZED_TEST(_testDeviceOpenedAgain);
    void _testArchiveFileLeavesOutLinks();

    // QGCCompressionJob (async operations)
    void _testCompressionJobExtract();
    void _testCompressionJobReentrantStartFromFinished();
    UT_PARAMETERIZED_TEST(_testCompressionJobCancel);
    void _testCompressionJobAsyncStatic();
    void _testCompressionJobCancelFuture();
    void _testCompressionJobErrorString();
    UT_PARAMETERIZED_TEST(_testCompressionJobPlainInput);
    void _testCompressionJobWork();

    // Sparse file handling
    UT_PARAMETERIZED_TEST(_testSparseFileExtraction);

    // Thread safety
    void _testConcurrentExtractions();

    // Boundary conditions
    void _testEmptyArchive();
    void _testDecompressToFileSizeLimit();
    void _testDecompressToMemorySizeLimit();
    UT_PARAMETERIZED_TEST(_testExtractionStopsCleanly);
    UT_PARAMETERIZED_TEST(_testDecompressionCancelled);
    UT_PARAMETERIZED_TEST(_testExtractArchiveKeepsEntriesBeforeDamage);
    void _testOverstatedEntrySize();
    UT_PARAMETERIZED_TEST(_testExtractFileDataSizeLimit);
    UT_PARAMETERIZED_TEST(_testUncompressDataSizeLimit);

    // Cross-platform path handling
    void _testExtractArchiveToSymlinkedOutputPath();
    UT_PARAMETERIZED_TEST(_testExtractRejectsEscapingNames);
    void _testExtractRejectsEscapingLinks();
    UT_PARAMETERIZED_TEST(_testExtractLinks);
    UT_PARAMETERIZED_TEST(_testExtractRestoresModificationTime);
    UT_PARAMETERIZED_TEST(_testExtractLeavesOutSpecialFiles);
    UT_PARAMETERIZED_TEST(_testExtractToUnusualPath);

    // Benchmarks (run with --benchmark flag)
    UT_PARAMETERIZED_TEST(_benchmarkExtract);
    UT_PARAMETERIZED_TEST(_benchmarkDecompress);
    void _benchmarkConcurrentExtraction();
    void _benchmarkListArchive();

private:
    // Builds the archive of its directory entry with _makeTar()
    friend class QGCArchiveModelTest;

    // The test resources: one manifest.json, as an archive and as a single compressed stream
    static inline const QString _zipResource = QStringLiteral(":/unittest/manifest.json.zip");
    static inline const QString _sevenZipResource = QStringLiteral(":/unittest/manifest.json.7z");
    static inline const QString _gzResource = QStringLiteral(":/unittest/manifest.json.gz");
    static inline const QString _xzResource = QStringLiteral(":/unittest/manifest.json.xz");
    static inline const QString _zstResource = QStringLiteral(":/unittest/manifest.json.zst");

    /// What a test hands to an entry point of QGCCompression, which takes of it what it has a parameter for
    struct Inputs
    {
        /// The archive or the compressed file. An entry point that reads a device or data is given this file
        /// as a device, or its content.
        QString path = {};
        /// Where to write: the directory to extract into, which a single entry is written to under its own
        /// name, or the file for a decompressed stream
        QString output = {};
        /// The entries asked for by name, or the patterns. An entry point for one entry takes the first.
        QStringList names = {QStringLiteral("manifest.json")};
        QGCCompression::EntryFilter filter = [](const QGCCompression::ArchiveEntry&) { return true; };
        QGCCompression::Format format = QGCCompression::Format::Auto;
        QGCCompression::ProgressCallback progress = {};
        qint64 maxBytes = 0;
    };

    /// What an entry point of QGCCompression gives back besides its outcome
    struct Results
    {
        QByteArray data = {};                              ///< Of the entry points that read to memory
        QStringList names = {};                            ///< Of listArchive
        QList<QGCCompression::ArchiveEntry> entries = {};  ///< Of listArchiveDetailed
        QGCCompression::ArchiveStats stats = {};           ///< Of getArchiveStats

        /// Whether nothing came back, which is so for an entry point that gives nothing back
        bool isEmpty() const
        {
            return data.isEmpty() && names.isEmpty() && entries.isEmpty() && (stats.totalEntries == 0) &&
                   (stats.fileCount == 0) && (stats.totalUncompressedSize == 0);
        }
    };

    /// Calls the entry point of QGCCompression that a row of test data names: a function, or
    /// "decompressFromDevice-file" and "decompressFromDevice-memory" for the two of that name
    /// @param results If not null, receives what the entry point gave back. Where the entry point is handed
    ///        something to fill, it is handed it with content, so that a failure has to empty it
    /// @return What the entry point returned
    static QGCCompression::Outcome _call(const QString& api, const Inputs& inputs, Results* results = nullptr);

    /// Whether @a api decompresses a single stream, and not an archive
    static bool _isStreamEntryPoint(const QString& api) { return api.startsWith(QLatin1String("decompress")); }

    /// Bytes for a stream of several blocks, which do not repeat from one block to the next
    static QByteArray _payload(qsizetype size, char first = 'a');

    /// Paths of everything below @a directoryPath, relative to it and sorted
    static QStringList _entriesBelow(const QString& directoryPath);

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
        /// '0' file, '1' hard link, '2' symbolic link, '3' character device, '4' block device, '5' directory,
        /// '6' FIFO, 'S' old GNU sparse file
        char type = '0';
        QByteArray linkName = {};
        QList<TarSparseData> sparseData = {};  ///< For 'S': where the data belongs in the file, at most four parts
        qint64 sparseFileSize = 0;             ///< For 'S': size of the file with its holes
        qint64 modified = 0;                   ///< Modification time in seconds since the epoch
    };

    /// Entry of a hand-built ZIP archive
    struct ZipEntry
    {
        QByteArray name = {};
        QByteArray data = {};
        quint32 unixMode = 0;  ///< File type and permissions as Unix has them, for a reader that can seek (0 = none)
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
