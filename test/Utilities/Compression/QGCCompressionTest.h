#pragma once

#include "QGCCompression.h"
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
    void _testFormatParameter();

    // Archive extraction from Qt resources
    void _testExtractFromResource_data();
    void _testExtractFromResource();
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
    void _testDecompressEmptyStream_data();
    void _testDecompressEmptyStream();
    void _testDecompressRejectsStreamWithoutData_data();
    void _testDecompressRejectsStreamWithoutData();
    void _testDecompressPlainInput_data();
    void _testDecompressPlainInput();
    void _testDecompressFileCopiesPlainInput_data();
    void _testDecompressFileCopiesPlainInput();

    // Progress callbacks
    void _testProgressCallbackExtract_data();
    void _testProgressCallbackExtract();

    // Format reported by an operation
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
    void _testSequentialDeviceEnd_data();
    void _testSequentialDeviceEnd();
    void _testSequentialDeviceReadError();
    void _testExtractFileDataErrors_data();
    void _testExtractFileDataErrors();
    void _testExtractToFileErrors_data();
    void _testExtractToFileErrors();
    void _testDecompressDataErrors();
    void _testOutputWithoutPermission_data();
    void _testOutputWithoutPermission();
    void _testInputWithoutPermission_data();
    void _testInputWithoutPermission();
    void _testBlockedOutput_data();
    void _testBlockedOutput();
    void _testOutcomeBeforeInput_data();
    void _testOutcomeBeforeInput();

    // QIODevice-based operations
    void _testDecompressFromDevice();
    void _testExtractFromDevice();
    void _testExtractFileDataFromDevice();
    void _testInterleavedDeviceReaders();
    void _testDeviceAtEnd_data();
    void _testDeviceAtEnd();
    void _testArchiveFileLeavesOutLinks();

    // QGCCompressionJob (async operations)
    void _testCompressionJobExtract();
    void _testCompressionJobReentrantStartFromFinished();
    void _testCompressionJobCancel_data();
    void _testCompressionJobCancel();
    void _testCompressionJobAsyncStatic();
    void _testCompressionJobCancelFuture();
    void _testCompressionJobErrorString();
    void _testCompressionJobPlainInput_data();
    void _testCompressionJobPlainInput();
    void _testCompressionJobWork();

    // Sparse file handling
    void _testSparseFileExtraction_data();
    void _testSparseFileExtraction();

    // Thread safety
    void _testConcurrentExtractions();

    // Boundary conditions
    void _testEmptyArchive();
    void _testSingleByteFile();
    void _testLargeFileDecompression();
    void _testDecompressToFileSizeLimit();
    void _testDecompressToMemorySizeLimit();
    void _testExtractionStopsCleanly_data();
    void _testExtractionStopsCleanly();
    void _testDecompressionCancelled_data();
    void _testDecompressionCancelled();
    void _testExtractArchiveKeepsEntriesBeforeDamage_data();
    void _testExtractArchiveKeepsEntriesBeforeDamage();
    void _testMaxEntriesArchive();
    void _testOverstatedEntrySize();
    void _testExtractFileDataSizeLimit_data();
    void _testExtractFileDataSizeLimit();
    void _testUncompressDataSizeLimit_data();
    void _testUncompressDataSizeLimit();

    // Cross-platform path handling
    void _testWindowsPathSeparators();
    void _testExtractArchiveToSymlinkedOutputPath();
    void _testExtractRejectsEscapingNames_data();
    void _testExtractRejectsEscapingNames();
    void _testExtractRejectsEscapingLinks();
    void _testExtractLinks_data();
    void _testExtractLinks();
    void _testExtractRestoresModificationTime_data();
    void _testExtractRestoresModificationTime();
    void _testExtractLeavesOutSpecialFiles_data();
    void _testExtractLeavesOutSpecialFiles();
    void _testSpecialCharactersInPath();
    void _testUnicodePaths();

    // Benchmarks (run with --benchmark flag)
    void _benchmarkExtract_data();
    void _benchmarkExtract();
    void _benchmarkDecompress_data();
    void _benchmarkDecompress();
    void _benchmarkConcurrentExtraction();
    void _benchmarkListArchive();

private:
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
            return data.isEmpty() && names.isEmpty() && entries.isEmpty() && (stats.totalEntries == 0);
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
