#pragma once

#include "Compression.h"
#include "PortableTest.h"

/// Tests for Compression: file kind detection, the decompression of a single stream, and what all entry points
/// have in common. QGCArchiveExtractionTest covers archive extraction, and QGCCompressionJobTest the job.
class QGCCompressionTest : public PortableTest
{
    Q_OBJECT

private slots:
    // Kind of a file
    UT_PARAMETERIZED_TEST(_testKindFromName);
    UT_PARAMETERIZED_TEST(_testKindFromContent);
    void _testNameDoesNotChooseTheReader();

    // Single-file decompression
    UT_PARAMETERIZED_TEST(_testDecompressEveryFormat);
    UT_PARAMETERIZED_TEST(_testDecompressEmptyStream);
    void _testDecompressRejectsCutEmptyStream();
    UT_PARAMETERIZED_TEST(_testDecompressPlainInput);
    UT_PARAMETERIZED_TEST(_testDecompressFileToFileCopiesPlainInput);
    UT_PARAMETERIZED_TEST(_testDecompressFileToFileDefaultOutput);

    // Detected compression filter
    UT_PARAMETERIZED_TEST(_testDetectedFilter);
    UT_PARAMETERIZED_TEST(_testDetectedFilterAfterFailure);

    // Edge cases and error handling
    UT_PARAMETERIZED_TEST(_testMissingInput);
    UT_PARAMETERIZED_TEST(_testInputThatCannotBeRead);
    UT_PARAMETERIZED_TEST(_testInputThatCannotSeek);
    void _testErrorNames();
    void _testMessagesAreTranslated();
    void _testNoExternalDecompressors();
    UT_PARAMETERIZED_TEST(_testLegacyLzmaStream);
    UT_PARAMETERIZED_TEST(_testDataThatIsNotLzma);
    UT_PARAMETERIZED_TEST(_testStreamInsideStream);
    void _testRefusalRulesAgreeWithLibarchive();
    UT_PARAMETERIZED_TEST(_testTruncatedStream);
    UT_PARAMETERIZED_TEST(_testEveryPrefixOfAStream);
    UT_PARAMETERIZED_TEST(_testDamagedStreamHeader);
    UT_PARAMETERIZED_TEST(_testGzipChecksums);
    void _testDecompressDataToMemoryInPlace();
    void _testPlainDataIsShared();
    void _testDecompressFileToMemory();
    void _testDecompressFileToSink();
    UT_PARAMETERIZED_TEST(_testUnwritableOutput);
    UT_PARAMETERIZED_TEST(_testOutputIsTheInput);
    UT_PARAMETERIZED_TEST(_testInputWithoutPermission);

    // Boundary conditions
    void _testDecompressToFileSizeLimit();
    void _testDecompressToMemorySizeLimit();
    void _testDecompressionCancelled();

private:
    /// Arguments for an entry point of Compression. Each entry point takes those it has parameters for
    struct Inputs
    {
        /// The archive or the compressed file. decompressDataToMemory() is given its content
        QString path = {};
        /// The directory to extract into, or the file to decompress to
        QString output = {};
    };

    /// What an entry point of Compression returns besides its outcome
    struct Results
    {
        /// From decompressDataToMemory() and decompressFileToMemory(), or what decompressFileToFile() wrote
        QByteArray data = {};
        bool wroteFile = false;  ///< Whether decompressFileToFile() left a file at its output path

        /// Whether nothing was returned or written, which is always so for an entry point that returns nothing else
        bool isEmpty() const { return data.isEmpty() && !wroteFile; }
    };

    /// Calls the entry point of Compression that a row of test data names: extractArchive, decompressFileToFile,
    /// decompressDataToMemory, decompressFileToMemory or decompressFileToSink. decompressFileToFile needs
    /// Inputs::output
    /// @param results If not null, receives what the entry point returned. Output parameters are filled before the
    ///        call, so that a failure must empty them
    /// @return What the entry point returned
    static Compression::Outcome _call(const QString& api, const Inputs& inputs, Results* results = nullptr);

    /// Whether @a api decompresses a single stream, and not an archive
    static bool _isStreamEntryPoint(const QString& api) { return api != QLatin1String("extractArchive"); }
};
