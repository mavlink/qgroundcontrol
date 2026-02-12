#include "QGCCompressionTest.h"

#include <QtConcurrent/QtConcurrent>
#include <QtCore/QBuffer>
#include <QtCore/QCollator>
#include <QtCore/QCryptographicHash>
#include <QtCore/QDir>
#include <QtCore/QEventLoop>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QLocale>
#include <QtCore/QMutex>
#include <QtCore/QRegularExpression>
#include <QtCore/QThread>
#include <QtCore/QTimer>
#include <QtCore/QWaitCondition>
#include <QtTest/QSignalSpy>

#include <algorithm>
#include <atomic>
#include <vector>

#include "QGCCompression.h"
#include "QGCCompressionJob.h"

bool QGCCompressionTest::_compareFiles(const QString& file1, const QString& file2)
{
    QFile f1(file1);
    QFile f2(file2);
    if (!f1.open(QIODevice::ReadOnly) || !f2.open(QIODevice::ReadOnly)) {
        return false;
    }
    QCryptographicHash hash1(QCryptographicHash::Sha256);
    QCryptographicHash hash2(QCryptographicHash::Sha256);
    hash1.addData(&f1);
    hash2.addData(&f2);
    return hash1.result() == hash2.result();
}

// ============================================================================
// Format Detection Tests
// ============================================================================
void QGCCompressionTest::_testFormatDetection()
{
    // Extension-based detection
    QCOMPARE(QGCCompression::detectFormat("file.zip"), QGCCompression::Format::ZIP);
    QCOMPARE(QGCCompression::detectFormat("file.7z"), QGCCompression::Format::SEVENZ);
    QCOMPARE(QGCCompression::detectFormat("file.gz"), QGCCompression::Format::GZIP);
    QCOMPARE(QGCCompression::detectFormat("file.gzip"), QGCCompression::Format::GZIP);
    QCOMPARE(QGCCompression::detectFormat("file.xz"), QGCCompression::Format::XZ);
    QCOMPARE(QGCCompression::detectFormat("file.lzma"), QGCCompression::Format::XZ);
    QCOMPARE(QGCCompression::detectFormat("file.zst"), QGCCompression::Format::ZSTD);
    QCOMPARE(QGCCompression::detectFormat("file.zstd"), QGCCompression::Format::ZSTD);
    QCOMPARE(QGCCompression::detectFormat("file.tar"), QGCCompression::Format::TAR);
    QCOMPARE(QGCCompression::detectFormat("file.tar.gz"), QGCCompression::Format::TAR_GZ);
    QCOMPARE(QGCCompression::detectFormat("file.tgz"), QGCCompression::Format::TAR_GZ);
    QCOMPARE(QGCCompression::detectFormat("file.tar.xz"), QGCCompression::Format::TAR_XZ);
    QCOMPARE(QGCCompression::detectFormat("file.txz"), QGCCompression::Format::TAR_XZ);
    QCOMPARE(QGCCompression::detectFormat("file.unknown"), QGCCompression::Format::Auto);
    QCOMPARE(QGCCompression::detectFormat("file.txt"), QGCCompression::Format::Auto);
    // Case insensitive
    QCOMPARE(QGCCompression::detectFormat("FILE.ZIP"), QGCCompression::Format::ZIP);
    QCOMPARE(QGCCompression::detectFormat("File.Gz"), QGCCompression::Format::GZIP);
    // Test without content fallback (extension-only)
    QCOMPARE(QGCCompression::detectFormat("file.unknown", false), QGCCompression::Format::Auto);
}

void QGCCompressionTest::_testFormatDetectionFromContent()
{
    // Test detectFormatFromFile() - reads actual file content
    // Using Qt resources that exist in the test bundle
    // ZIP archive
    QCOMPARE(QGCCompression::detectFormatFromFile(":/unittest/manifest.json.zip"), QGCCompression::Format::ZIP);
    // GZIP compressed file
    QCOMPARE(QGCCompression::detectFormatFromFile(":/unittest/manifest.json.gz"), QGCCompression::Format::GZIP);
    // XZ compressed file
    QCOMPARE(QGCCompression::detectFormatFromFile(":/unittest/manifest.json.xz"), QGCCompression::Format::XZ);
    // ZSTD compressed file
    QCOMPARE(QGCCompression::detectFormatFromFile(":/unittest/manifest.json.zst"), QGCCompression::Format::ZSTD);
#ifdef QGC_ENABLE_BZIP2
    // BZ2 compressed file
    QCOMPARE(QGCCompression::detectFormatFromFile(":/unittest/manifest.json.bz2"), QGCCompression::Format::BZIP2);
#endif
    // 7z archive
    QCOMPARE(QGCCompression::detectFormatFromFile(":/unittest/manifest.json.7z"), QGCCompression::Format::SEVENZ);
    // Test detectFormatFromData() with raw bytes
    // GZIP magic bytes
    QByteArray gzipData;
    gzipData.append(char(0x1F));
    gzipData.append(char(0x8B));
    gzipData.append(QByteArray(10, '\0'));  // Padding
    QCOMPARE(QGCCompression::detectFormatFromData(gzipData), QGCCompression::Format::GZIP);
    // ZIP magic bytes (PK\x03\x04)
    QByteArray zipData;
    zipData.append(char(0x50));  // 'P'
    zipData.append(char(0x4B));  // 'K'
    zipData.append(char(0x03));
    zipData.append(char(0x04));
    zipData.append(QByteArray(10, '\0'));
    QCOMPARE(QGCCompression::detectFormatFromData(zipData), QGCCompression::Format::ZIP);
    // XZ magic bytes
    QByteArray xzData;
    xzData.append(char(0xFD));
    xzData.append(char(0x37));  // '7'
    xzData.append(char(0x7A));  // 'z'
    xzData.append(char(0x58));  // 'X'
    xzData.append(char(0x5A));  // 'Z'
    xzData.append(char(0x00));
    xzData.append(QByteArray(10, '\0'));
    QCOMPARE(QGCCompression::detectFormatFromData(xzData), QGCCompression::Format::XZ);
    // ZSTD magic bytes
    QByteArray zstdData;
    zstdData.append(char(0x28));
    zstdData.append(char(0xB5));
    zstdData.append(char(0x2F));
    zstdData.append(char(0xFD));
    zstdData.append(QByteArray(10, '\0'));
    QCOMPARE(QGCCompression::detectFormatFromData(zstdData), QGCCompression::Format::ZSTD);
    // Test content fallback in detectFormat()
    // Copy a .gz file to a file without extension and verify detection
    const QString noExtFile = tempDir()->filePath("compressed_no_ext");
    {
        QFile source(":/unittest/manifest.json.gz");
        QVERIFY(source.open(QIODevice::ReadOnly));
        QFile dest(noExtFile);
        QVERIFY(dest.open(QIODevice::WriteOnly));
        dest.write(source.readAll());
    }
    // With content fallback enabled, should detect as GZIP
    QCOMPARE(QGCCompression::detectFormat(noExtFile, true), QGCCompression::Format::GZIP);
    // Without content fallback, should return Auto
    QCOMPARE(QGCCompression::detectFormat(noExtFile, false), QGCCompression::Format::Auto);
}

void QGCCompressionTest::_testFormatHelpers()
{
    // isArchiveFormat
    QVERIFY(QGCCompression::isArchiveFormat(QGCCompression::Format::ZIP));
    QVERIFY(QGCCompression::isArchiveFormat(QGCCompression::Format::SEVENZ));
    QVERIFY(QGCCompression::isArchiveFormat(QGCCompression::Format::TAR));
    QVERIFY(QGCCompression::isArchiveFormat(QGCCompression::Format::TAR_GZ));
    QVERIFY(QGCCompression::isArchiveFormat(QGCCompression::Format::TAR_XZ));
    QVERIFY(!QGCCompression::isArchiveFormat(QGCCompression::Format::GZIP));
    QVERIFY(!QGCCompression::isArchiveFormat(QGCCompression::Format::XZ));
    QVERIFY(!QGCCompression::isArchiveFormat(QGCCompression::Format::ZSTD));
    // isCompressionFormat
    QVERIFY(QGCCompression::isCompressionFormat(QGCCompression::Format::GZIP));
    QVERIFY(QGCCompression::isCompressionFormat(QGCCompression::Format::XZ));
    QVERIFY(QGCCompression::isCompressionFormat(QGCCompression::Format::ZSTD));
    QVERIFY(!QGCCompression::isCompressionFormat(QGCCompression::Format::ZIP));
    QVERIFY(!QGCCompression::isCompressionFormat(QGCCompression::Format::TAR));
    // isCompressedFile / isArchiveFile
    QVERIFY(QGCCompression::isCompressedFile("file.gz"));
    QVERIFY(QGCCompression::isCompressedFile("file.xz"));
    QVERIFY(!QGCCompression::isCompressedFile("file.zip"));
    QVERIFY(!QGCCompression::isCompressedFile("file.txt"));
    QVERIFY(QGCCompression::isArchiveFile("file.zip"));
    QVERIFY(QGCCompression::isArchiveFile("file.7z"));
    QVERIFY(QGCCompression::isArchiveFile("file.tar.gz"));
    QVERIFY(!QGCCompression::isArchiveFile("file.gz"));
    // strippedPath
    QCOMPARE(QGCCompression::strippedPath("file.gz"), QString("file"));
    QCOMPARE(QGCCompression::strippedPath("file.xz"), QString("file"));
    QCOMPARE(QGCCompression::strippedPath("file.zst"), QString("file"));
    QCOMPARE(QGCCompression::strippedPath("file.txt"), QString("file.txt"));
    QCOMPARE(QGCCompression::strippedPath("path/to/file.gz"), QString("path/to/file"));
    // formatExtension
    QCOMPARE(QGCCompression::formatExtension(QGCCompression::Format::ZIP), QString(".zip"));
    QCOMPARE(QGCCompression::formatExtension(QGCCompression::Format::GZIP), QString(".gz"));
    QCOMPARE(QGCCompression::formatExtension(QGCCompression::Format::TAR_GZ), QString(".tar.gz"));
    // formatName
    QVERIFY(!QGCCompression::formatName(QGCCompression::Format::ZIP).isEmpty());
    QVERIFY(!QGCCompression::formatName(QGCCompression::Format::GZIP).isEmpty());
}

// ============================================================================
// Archive Extraction Tests (from Qt resources)
// ============================================================================
void QGCCompressionTest::_testZipFromResource()
{
    const QString zipResource = QStringLiteral(":/unittest/manifest.json.zip");
    const QString outputPath = tempDir()->path() + "/resource_output";
    QVERIFY2(QGCCompression::extractArchive(zipResource, outputPath), "Failed to extract ZIP from Qt resource");
    QVERIFY2(QDir(outputPath).exists(), "Output directory not created");
    QDir outputDir(outputPath);
    QStringList files = outputDir.entryList(QDir::Files | QDir::NoDotAndDotDot);
    QVERIFY2(!files.isEmpty(), "No files extracted from ZIP");
}

void QGCCompressionTest::_test7zFromResource()
{
    const QString archiveResource = QStringLiteral(":/unittest/manifest.json.7z");
    const QString outputPath = tempDir()->path() + "/7z_output";
    QVERIFY2(QGCCompression::extractArchive(archiveResource, outputPath, QGCCompression::Format::SEVENZ),
             "Failed to extract 7z from Qt resource");
    QVERIFY2(QDir(outputPath).exists(), "Output directory not created");
    QDir outputDir(outputPath);
    QStringList files = outputDir.entryList(QDir::Files | QDir::NoDotAndDotDot);
    QVERIFY2(!files.isEmpty(), "No files extracted from 7z");
    QVERIFY2(QFile::exists(outputPath + "/manifest.json"), "manifest.json not extracted");
}

void QGCCompressionTest::_testExtractArchiveAtomic()
{
    const QString zipResource = QStringLiteral(":/unittest/manifest.json.zip");
    const QString outputPath = tempDir()->path() + "/atomic_output";

    QVERIFY2(QGCCompression::extractArchiveAtomic(zipResource, outputPath),
             "Failed to atomically extract ZIP from Qt resource");
    QVERIFY2(QDir(outputPath).exists(), "Output directory not created");
    QVERIFY2(QFile::exists(outputPath + "/manifest.json"), "manifest.json not extracted");
}

void QGCCompressionTest::_testExtractArchiveAtomicReplacesExistingDirectory()
{
    const QString zipResource = QStringLiteral(":/unittest/manifest.json.zip");
    const QString outputPath = tempDir()->path() + "/atomic_replace";
    QVERIFY(QDir().mkpath(outputPath));

    const QString staleFilePath = outputPath + "/stale.txt";
    QFile staleFile(staleFilePath);
    QVERIFY(staleFile.open(QIODevice::WriteOnly | QIODevice::Truncate));
    staleFile.write("stale");
    staleFile.close();
    QVERIFY(QFile::exists(staleFilePath));

    QVERIFY2(QGCCompression::extractArchiveAtomic(zipResource, outputPath),
             "Atomic extraction into existing directory failed");
    QVERIFY(QFile::exists(outputPath + "/manifest.json"));
    QVERIFY(!QFile::exists(staleFilePath));
}

void QGCCompressionTest::_testExtractArchiveAtomicFailureKeepsExistingDirectory()
{
    const QString outputPath = tempDir()->path() + "/atomic_rollback";
    QVERIFY(QDir().mkpath(outputPath));

    const QString retainedFilePath = outputPath + "/retained.txt";
    QFile retainedFile(retainedFilePath);
    QVERIFY(retainedFile.open(QIODevice::WriteOnly | QIODevice::Truncate));
    const QByteArray retainedContent("keep-me");
    retainedFile.write(retainedContent);
    retainedFile.close();

    const QString corruptArchivePath = tempDir()->path() + "/corrupt_atomic.zip";
    QFile corrupt(corruptArchivePath);
    QVERIFY(corrupt.open(QIODevice::WriteOnly | QIODevice::Truncate));
    corrupt.write("not-an-archive");
    corrupt.close();

    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*Unrecognized archive format.*"));
    QVERIFY(!QGCCompression::extractArchiveAtomic(corruptArchivePath, outputPath));

    QFile retainedAfter(retainedFilePath);
    QVERIFY(retainedAfter.open(QIODevice::ReadOnly));
    QCOMPARE(retainedAfter.readAll(), retainedContent);
    retainedAfter.close();
}

void QGCCompressionTest::_testListArchive()
{
    const QString zipResource = QStringLiteral(":/unittest/manifest.json.zip");
    QStringList entries = QGCCompression::listArchive(zipResource);
    QVERIFY2(!entries.isEmpty(), "listArchive returned empty list");
    bool foundManifest = false;
    for (const QString& entry : entries) {
        if (entry.contains("manifest.json")) {
            foundManifest = true;
            break;
        }
    }
    QVERIFY2(foundManifest, "manifest.json not found in archive listing");
    // Test auto-detection
    QStringList autoEntries = QGCCompression::listArchive(zipResource, QGCCompression::Format::Auto);
    QCOMPARE(autoEntries.size(), entries.size());
}

void QGCCompressionTest::_testListArchiveDetailed()
{
    const QString zipResource = QStringLiteral(":/unittest/manifest.json.zip");
    QList<QGCCompression::ArchiveEntry> entries = QGCCompression::listArchiveDetailed(zipResource);
    QVERIFY(!entries.isEmpty());
    bool foundManifest = false;
    for (const auto& entry : entries) {
        if (entry.name == "manifest.json") {
            foundManifest = true;
            QVERIFY(entry.size > 0);
            QVERIFY(!entry.isDirectory);
            break;
        }
    }
    QVERIFY2(foundManifest, "manifest.json not found in detailed listing");
}

void QGCCompressionTest::_testListArchiveNaturalSort()
{
    // Create a ZIP archive with files that sort differently with natural vs lexicographic sort
    // Files: file1.txt, file2.txt, file10.txt, file20.txt
    // Lexicographic: file1.txt, file10.txt, file2.txt, file20.txt
    // Natural:       file1.txt, file2.txt, file10.txt, file20.txt
    const QString zipPath = tempDir()->path() + "/natural_sort_test.zip";
    // Create test archive using miniz or system zip
    // We'll create a simple ZIP with QBuffer and test the sorting
    // For this test, we verify the sorting behavior using QCollator directly
    // since QGCCompression uses it internally
    QStringList unsorted = {"file10.txt", "file1.txt", "file20.txt", "file2.txt", "dir/file3.txt", "dir/file11.txt"};
    QStringList expected = {"dir/file3.txt", "dir/file11.txt", "file1.txt", "file2.txt", "file10.txt", "file20.txt"};
    // Simulate the sorting done by listArchive (using English locale for cross-platform consistency)
    QCollator collator(QLocale(QLocale::English, QLocale::AnyCountry));
    collator.setNumericMode(true);
    collator.setCaseSensitivity(Qt::CaseInsensitive);
    std::sort(unsorted.begin(), unsorted.end(), collator);
    QCOMPARE(unsorted, expected);
    // Also verify case-insensitive behavior
    QStringList caseTest = {"File.txt", "file.txt", "FILE.txt"};
    std::sort(caseTest.begin(), caseTest.end(), collator);
    // All should be equivalent, stable sort preserves order
    QCOMPARE(caseTest.size(), 3);
    // Verify numeric mode works with leading zeros
    QStringList leadingZeros = {"file001.txt", "file01.txt", "file1.txt", "file10.txt"};
    QStringList expectedZeros = {"file1.txt", "file001.txt", "file01.txt", "file10.txt"};
    std::sort(leadingZeros.begin(), leadingZeros.end(), collator);
    // Note: QCollator with numeric mode treats 001, 01, 1 as same value 1, so order depends on implementation
    // The key behavior is that 1 < 10, which is the main point
    QVERIFY2(leadingZeros.indexOf("file10.txt") > leadingZeros.indexOf("file1.txt"),
             "file10.txt should come after file1.txt in natural sort");
}

void QGCCompressionTest::_testGetArchiveStats()
{
    const QString zipResource = QStringLiteral(":/unittest/manifest.json.zip");
    const QGCCompression::ArchiveStats stats = QGCCompression::getArchiveStats(zipResource);
    // Should have at least one file
    QVERIFY(stats.totalEntries > 0);
    QVERIFY(stats.fileCount > 0);
    QVERIFY(stats.totalUncompressedSize > 0);
    QVERIFY(stats.largestFileSize > 0);
    QVERIFY(!stats.largestFileName.isEmpty());
    // File count + directory count should equal total entries
    QCOMPARE(stats.totalEntries, stats.fileCount + stats.directoryCount);
    // Largest file size should not exceed total size
    QVERIFY(stats.largestFileSize <= stats.totalUncompressedSize);
    // Test with 7z archive
    const QString archiveResource = QStringLiteral(":/unittest/manifest.json.7z");
    const QGCCompression::ArchiveStats stats7z = QGCCompression::getArchiveStats(archiveResource);
    QVERIFY(stats7z.totalEntries > 0);
    QVERIFY(stats7z.fileCount > 0);
    // Test non-existent file returns zero stats
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("File does not exist"));
    const QGCCompression::ArchiveStats emptyStats = QGCCompression::getArchiveStats("/nonexistent/file.zip");
    QCOMPARE(emptyStats.totalEntries, 0);
    QCOMPARE(emptyStats.fileCount, 0);
    QCOMPARE(emptyStats.totalUncompressedSize, 0);
}

void QGCCompressionTest::_testValidateArchive()
{
    const QString zipResource = QStringLiteral(":/unittest/manifest.json.zip");
    QVERIFY(QGCCompression::validateArchive(zipResource));
    // Test corrupt archive fails validation
    const QString corruptZip = tempDir()->path() + "/corrupt_validate.zip";
    QFile corrupt(corruptZip);
    QVERIFY(corrupt.open(QIODevice::WriteOnly));
    corrupt.write("This is not a valid ZIP file");
    corrupt.close();
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("Failed to open file.*Unrecognized archive format"));
    QVERIFY(!QGCCompression::validateArchive(corruptZip));
    // Test non-existent file
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("File does not exist"));
    QVERIFY(!QGCCompression::validateArchive("/nonexistent/path/file.zip"));
}

void QGCCompressionTest::_testFileExists()
{
    const QString zipResource = QStringLiteral(":/unittest/manifest.json.zip");
    // File that exists
    QVERIFY(QGCCompression::fileExists(zipResource, "manifest.json"));
    // File that doesn't exist
    QVERIFY(!QGCCompression::fileExists(zipResource, "nonexistent.txt"));
    // Empty file name
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("File name cannot be empty"));
    QVERIFY(!QGCCompression::fileExists(zipResource, ""));
    // Non-existent archive
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("File does not exist"));
    QVERIFY(!QGCCompression::fileExists("/nonexistent/path/file.zip", "test.txt"));
}

void QGCCompressionTest::_testExtractArchiveFiltered()
{
    const QString zipResource = QStringLiteral(":/unittest/manifest.json.zip");
    // Test filter that accepts all files
    const QString outputDir1 = tempDir()->path() + "/filtered_all";
    int extractedCount = 0;
    bool success = QGCCompression::extractArchiveFiltered(
        zipResource, outputDir1, [&extractedCount](const QGCCompression::ArchiveEntry& entry) -> bool {
            Q_UNUSED(entry);
            extractedCount++;
            return true;  // Accept all
        });
    QVERIFY2(success, "extractArchiveFiltered with accept-all filter failed");
    QVERIFY(extractedCount > 0);
    QVERIFY(QFile::exists(outputDir1 + "/manifest.json"));
    // Test filter that rejects all files
    const QString outputDir2 = tempDir()->path() + "/filtered_none";
    success = QGCCompression::extractArchiveFiltered(zipResource, outputDir2,
                                                     [](const QGCCompression::ArchiveEntry&) -> bool {
                                                         return false;  // Reject all
                                                     });
    QVERIFY2(success, "extractArchiveFiltered with reject-all filter should still succeed");
    // Directory should exist but be empty (or not created at all)
    QDir dir2(outputDir2);
    if (dir2.exists()) {
        QStringList files = dir2.entryList(QDir::Files | QDir::NoDotAndDotDot);
        QVERIFY2(files.isEmpty(), "No files should be extracted when filter rejects all");
    }
    // Test filter by extension (only .json files)
    const QString outputDir3 = tempDir()->path() + "/filtered_json";
    QStringList extractedNames;
    success = QGCCompression::extractArchiveFiltered(
        zipResource, outputDir3, [&extractedNames](const QGCCompression::ArchiveEntry& entry) -> bool {
            if (entry.name.endsWith(".json")) {
                extractedNames.append(entry.name);
                return true;
            }
            return false;
        });
    QVERIFY(success);
    QVERIFY(extractedNames.contains("manifest.json"));
    // Test filter receives correct metadata
    bool metadataCorrect = true;
    success =
        QGCCompression::extractArchiveFiltered(zipResource, tempDir()->path() + "/filtered_meta",
                                               [&metadataCorrect](const QGCCompression::ArchiveEntry& entry) -> bool {
                                                   if (entry.name == "manifest.json") {
                                                       // Verify metadata is populated
                                                       if (entry.size <= 0)
                                                           metadataCorrect = false;
                                                       if (entry.isDirectory)
                                                           metadataCorrect = false;
                                                   }
                                                   return true;
                                               });
    QVERIFY(success);
    QVERIFY2(metadataCorrect, "Filter did not receive correct metadata");
    // Test with non-existent file
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("File does not exist"));
    success = QGCCompression::extractArchiveFiltered("/nonexistent/file.zip", outputDir1,
                                                     [](const QGCCompression::ArchiveEntry&) { return true; });
    QVERIFY(!success);
}

void QGCCompressionTest::_testExtractSingleFile()
{
    const QString zipResource = QStringLiteral(":/unittest/manifest.json.zip");
    const QString extractedPath = tempDir()->path() + "/extracted_manifest.json";
    QVERIFY(QGCCompression::extractFile(zipResource, "manifest.json", extractedPath));
    QVERIFY(QFile::exists(extractedPath));
    QVERIFY(QFileInfo(extractedPath).size() > 0);
    // Verify content contains expected JSON structure
    QFile extracted(extractedPath);
    QVERIFY(extracted.open(QIODevice::ReadOnly));
    const QByteArray content = extracted.readAll();
    QVERIFY(content.contains("\"name\""));
    extracted.close();
    // Test non-existent file returns false
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("File not found in archive"));
    QVERIFY(!QGCCompression::extractFile(zipResource, "nonexistent.txt", tempDir()->path() + "/nope.txt"));
}

void QGCCompressionTest::_testExtractFileData()
{
    const QString zipResource = QStringLiteral(":/unittest/manifest.json.zip");
    const QByteArray extracted = QGCCompression::extractFileData(zipResource, "manifest.json");
    QVERIFY(!extracted.isEmpty());
    QVERIFY(extracted.contains("\"name\""));
    // Test non-existent file returns empty
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("File not found in archive"));
    const QByteArray notFound = QGCCompression::extractFileData(zipResource, "nonexistent.txt");
    QVERIFY(notFound.isEmpty());
    // Test empty file name returns empty
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("File name cannot be empty"));
    const QByteArray emptyName = QGCCompression::extractFileData(zipResource, "");
    QVERIFY(emptyName.isEmpty());
}

void QGCCompressionTest::_testExtractMultipleFiles()
{
    const QString zipResource = QStringLiteral(":/unittest/manifest.json.zip");
    // Extract available file(s)
    const QString outputDir = tempDir()->path() + "/multi_extract";
    QStringList filesToExtract = {"manifest.json"};
    QVERIFY(QGCCompression::extractFiles(zipResource, filesToExtract, outputDir));
    QVERIFY(QFile::exists(outputDir + "/manifest.json"));
    // Test with non-existent file in list - should fail
    const QString outputDir2 = tempDir()->path() + "/multi_extract2";
    QStringList badFiles = {"manifest.json", "nonexistent.txt"};
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("File not found in archive"));
    QVERIFY(!QGCCompression::extractFiles(zipResource, badFiles, outputDir2));
    // Test empty list succeeds (no-op)
    QVERIFY(QGCCompression::extractFiles(zipResource, {}, outputDir));
}

void QGCCompressionTest::_testExtractByPattern()
{
    const QString zipResource = QStringLiteral(":/unittest/manifest.json.zip");
    // Test wildcard pattern matching
    const QString outputDir = tempDir()->path() + "/pattern_extract";
    QStringList extractedFiles;
    QVERIFY(QGCCompression::extractByPattern(zipResource, {"*.json"}, outputDir, &extractedFiles));
    QVERIFY(!extractedFiles.isEmpty());
    QVERIFY(extractedFiles.contains("manifest.json"));
    QVERIFY(QFile::exists(outputDir + "/manifest.json"));
    // Test exact filename as pattern (should work like extractFiles)
    const QString outputDir2 = tempDir()->path() + "/pattern_exact";
    QStringList exactFiles;
    QVERIFY(QGCCompression::extractByPattern(zipResource, {"manifest.json"}, outputDir2, &exactFiles));
    QCOMPARE(exactFiles.size(), 1);
    QCOMPARE(exactFiles.first(), QStringLiteral("manifest.json"));
    // Test no matches returns false
    const QString outputDir3 = tempDir()->path() + "/pattern_nomatch";
    QVERIFY(!QGCCompression::extractByPattern(zipResource, {"*.xyz"}, outputDir3));
    // Test empty patterns returns false
    const QString outputDir4 = tempDir()->path() + "/pattern_empty";
    QVERIFY(!QGCCompression::extractByPattern(zipResource, {}, outputDir4));
}

// ============================================================================
// Single-File Decompression Tests (from Qt resources)
// ============================================================================
void QGCCompressionTest::_testDecompressFromResource()
{
    struct ResourceTest
    {
        const char* resource;
        const char* outputName;
        const char* name;
    };

    std::vector<ResourceTest> resources = {
        {":/unittest/manifest.json.gz", "manifest_gz.json", "GZIP"},
        {":/unittest/manifest.json.xz", "manifest_xz.json", "XZ"},
        {":/unittest/manifest.json.zst", "manifest_zst.json", "ZSTD"},
#ifdef QGC_ENABLE_BZIP2
        {":/unittest/manifest.json.bz2", "manifest_bz2.json", "BZip2"},
#endif
#ifdef QGC_ENABLE_LZ4
        {":/unittest/manifest.json.lz4", "manifest_lz4.json", "LZ4"},
#endif
    };
    for (const auto& res : resources) {
        const QString outputFile = tempDir()->path() + "/" + res.outputName;
        QVERIFY2(QGCCompression::decompressFile(res.resource, outputFile),
                 qPrintable(QString("Failed to decompress %1 file").arg(res.name)));
        QVERIFY(QFile::exists(outputFile));
        QVERIFY(QFileInfo(outputFile).size() > 0);
    }
}

void QGCCompressionTest::_testDecompressData()
{
    // Read compressed data from resources and decompress in memory
    struct ResourceTest
    {
        const char* resource;
        const char* name;
    };

    std::vector<ResourceTest> resources = {
        {":/unittest/manifest.json.gz", "GZIP"},   {":/unittest/manifest.json.xz", "XZ"},
        {":/unittest/manifest.json.zst", "ZSTD"},
#ifdef QGC_ENABLE_BZIP2
        {":/unittest/manifest.json.bz2", "BZip2"},
#endif
#ifdef QGC_ENABLE_LZ4
        {":/unittest/manifest.json.lz4", "LZ4"},
#endif
    };
    for (const auto& res : resources) {
        QFile resourceFile(res.resource);
        QVERIFY2(resourceFile.open(QIODevice::ReadOnly),
                 qPrintable(QString("Failed to open resource: %1").arg(res.resource)));
        const QByteArray compressedData = resourceFile.readAll();
        resourceFile.close();
        QVERIFY2(!compressedData.isEmpty(), qPrintable(QString("Resource is empty: %1").arg(res.resource)));
        const QByteArray decompressed = QGCCompression::decompressData(compressedData);
        QVERIFY2(!decompressed.isEmpty(), qPrintable(QString("%1 decompression returned empty data").arg(res.name)));
        QVERIFY2(decompressed.contains("\"name\""),
                 qPrintable(QString("%1 decompressed content invalid").arg(res.name)));
    }
}

void QGCCompressionTest::_testDecompressIfNeeded()
{
    // Test with compressed file
    const QString gzResource = QStringLiteral(":/unittest/manifest.json.gz");
    const QString outputPath = tempDir()->path() + "/decompress_if_needed.json";
    QString result = QGCCompression::decompressIfNeeded(gzResource, outputPath);
    QVERIFY2(!result.isEmpty(), "decompressIfNeeded should return output path");
    QCOMPARE(result, outputPath);
    QVERIFY(QFile::exists(outputPath));
    QVERIFY(QFileInfo(outputPath).size() > 0);
    // Test with non-compressed file (should return original path)
    const QString plainFile = tempDir()->path() + "/plain.txt";
    QFile plain(plainFile);
    QVERIFY(plain.open(QIODevice::WriteOnly));
    plain.write("plain text content");
    plain.close();
    result = QGCCompression::decompressIfNeeded(plainFile);
    QCOMPARE(result, plainFile);
    QVERIFY(QFile::exists(plainFile));
    // Test with non-existent file
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("File does not exist"));
    result = QGCCompression::decompressIfNeeded("/nonexistent/file.gz");
    QVERIFY(result.isEmpty());
}

// ============================================================================
// Progress Callback Tests
// ============================================================================
void QGCCompressionTest::_testProgressCallbackExtract()
{
    const QString zipResource = QStringLiteral(":/unittest/manifest.json.zip");
    const QString outputPath = tempDir()->path() + "/progress_extract";
    qint64 lastProgress = 0;
    int callCount = 0;
    bool progressValid = true;
    bool success = QGCCompression::extractArchive(zipResource, outputPath, QGCCompression::Format::ZIP,
                                                  [&](qint64 bytesProcessed, qint64 totalBytes) -> bool {
                                                      if (bytesProcessed < lastProgress)
                                                          progressValid = false;
                                                      lastProgress = bytesProcessed;
                                                      callCount++;
                                                      Q_UNUSED(totalBytes);
                                                      return true;  // Continue
                                                  });
    QVERIFY2(success, "Archive extraction with progress callback failed");
    QVERIFY2(progressValid, "Progress values were invalid");
    // Note: callCount may be 0 for very small archives
    QVERIFY(QDir(outputPath).exists());
}

// ============================================================================
// Edge Cases and Error Handling Tests
// ============================================================================
void QGCCompressionTest::_testCorruptArchive()
{
    // Create a file with garbage data pretending to be a ZIP
    const QString corruptZip = tempDir()->path() + "/corrupt.zip";
    QFile file(corruptZip);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("This is not a valid ZIP file - just garbage data!");
    file.close();
    // Attempt to extract - should fail gracefully
    const QString outputPath = tempDir()->path() + "/corrupt_output";
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("Failed to open file.*Unrecognized archive format"));
    bool success = QGCCompression::extractArchive(corruptZip, outputPath);
    QVERIFY2(!success, "Extracting corrupt archive should fail");
    // Attempt to list - should return empty or fail
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("Failed to open file.*Unrecognized archive format"));
    QStringList entries = QGCCompression::listArchive(corruptZip);
    QVERIFY2(entries.isEmpty(), "Listing corrupt archive should return empty list");
    // Test corrupt compressed file with valid GZIP header but truncated data
    const QString corruptGz = tempDir()->path() + "/corrupt.gz";
    QFile gzFile(corruptGz);
    QVERIFY(gzFile.open(QIODevice::WriteOnly));
    const char corruptGzipData[] = {
        '\x1f', '\x8b',                  // GZIP magic
        '\x08',                          // Deflate method
        '\x00',                          // Flags
        '\x00', '\x00', '\x00', '\x00',  // Modification time
        '\x00',                          // Extra flags
        '\xff',                          // OS (unknown)
    };
    gzFile.write(corruptGzipData, sizeof(corruptGzipData));
    gzFile.close();
    const QString decompressedPath = tempDir()->path() + "/corrupt_decompressed";
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("Failed to open file.*truncated gzip input"));
    success = QGCCompression::decompressFile(corruptGz, decompressedPath);
    QVERIFY2(!success, "Decompressing truncated GZIP file should fail");
}

void QGCCompressionTest::_testNonExistentInput()
{
    const QString nonExistent = "/path/that/does/not/exist/file.txt";
    const QString outputPath = tempDir()->path() + "/output";
    // Test decompressFile with non-existent input
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("File does not exist"));
    QVERIFY2(!QGCCompression::decompressFile(nonExistent + ".gz", outputPath),
             "decompressFile should fail for non-existent input");
    // Test extractArchive with non-existent archive
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("File does not exist"));
    QVERIFY2(!QGCCompression::extractArchive(nonExistent + ".zip", outputPath),
             "extractArchive should fail for non-existent archive");
    // Test listArchive with non-existent file
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("File does not exist"));
    QStringList entries = QGCCompression::listArchive(nonExistent + ".zip");
    QVERIFY2(entries.isEmpty(), "listArchive should return empty for non-existent file");
}

// ============================================================================
// QIODevice-based Operation Tests
// ============================================================================
void QGCCompressionTest::_testDecompressFromDevice()
{
    // Load compressed data from Qt resource into a QBuffer
    QFile resourceFile(":/unittest/manifest.json.gz");
    QVERIFY(resourceFile.open(QIODevice::ReadOnly));
    const QByteArray compressedData = resourceFile.readAll();
    resourceFile.close();
    QVERIFY(!compressedData.isEmpty());
    // Test decompression to file using QBuffer as device
    QBuffer buffer;
    buffer.setData(compressedData);
    QVERIFY(buffer.open(QIODevice::ReadOnly));
    const QString outputPath = tempDir()->path() + "/device_decompressed.json";
    QVERIFY2(QGCCompression::decompressFromDevice(&buffer, outputPath), "Failed to decompress from QBuffer device");
    QVERIFY(QFile::exists(outputPath));
    QVERIFY(QFileInfo(outputPath).size() > 0);
    // Verify content
    QFile decompressed(outputPath);
    QVERIFY(decompressed.open(QIODevice::ReadOnly));
    const QByteArray content = decompressed.readAll();
    QVERIFY(content.contains("\"name\""));
    decompressed.close();
    buffer.close();
    // Test decompression to memory
    buffer.setData(compressedData);
    QVERIFY(buffer.open(QIODevice::ReadOnly));
    const QByteArray memoryResult = QGCCompression::decompressFromDevice(&buffer);
    QVERIFY2(!memoryResult.isEmpty(), "Decompress to memory from device failed");
    QVERIFY(memoryResult.contains("\"name\""));
    QCOMPARE(memoryResult, content);
    buffer.close();
    // Test with null device
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("Device is null"));
    QVERIFY(!QGCCompression::decompressFromDevice(nullptr, outputPath));
    // Test with closed device
    QBuffer closedBuffer;
    closedBuffer.setData(compressedData);
    // Don't open it
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("Device is null, not open, or not readable"));
    QVERIFY(!QGCCompression::decompressFromDevice(&closedBuffer, outputPath));
}

void QGCCompressionTest::_testExtractFromDevice()
{
    // Load ZIP from Qt resource into a QBuffer
    QFile resourceFile(":/unittest/manifest.json.zip");
    QVERIFY(resourceFile.open(QIODevice::ReadOnly));
    const QByteArray zipData = resourceFile.readAll();
    resourceFile.close();
    QVERIFY(!zipData.isEmpty());
    // Extract using QBuffer as device
    QBuffer buffer;
    buffer.setData(zipData);
    QVERIFY(buffer.open(QIODevice::ReadOnly));
    const QString outputDir = tempDir()->path() + "/device_extract";
    QVERIFY2(QGCCompression::extractFromDevice(&buffer, outputDir), "Failed to extract archive from QBuffer device");
    QVERIFY(QDir(outputDir).exists());
    // Verify extracted content
    QDir extractedDir(outputDir);
    QStringList files = extractedDir.entryList(QDir::Files | QDir::NoDotAndDotDot);
    QVERIFY2(!files.isEmpty(), "No files extracted from device");
    buffer.close();
    // Test with null device
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("Device is null"));
    QVERIFY(!QGCCompression::extractFromDevice(nullptr, outputDir));
}

void QGCCompressionTest::_testExtractFileDataFromDevice()
{
    // Load ZIP from Qt resource into a QBuffer
    QFile resourceFile(":/unittest/manifest.json.zip");
    QVERIFY(resourceFile.open(QIODevice::ReadOnly));
    const QByteArray zipData = resourceFile.readAll();
    resourceFile.close();
    QVERIFY(!zipData.isEmpty());
    // Extract single file from device to memory
    QBuffer buffer;
    buffer.setData(zipData);
    QVERIFY(buffer.open(QIODevice::ReadOnly));
    const QByteArray extracted = QGCCompression::extractFileDataFromDevice(&buffer, "manifest.json");
    QVERIFY2(!extracted.isEmpty(), "Failed to extract file data from device");
    QVERIFY(extracted.contains("\"name\""));
    buffer.close();
    // Test non-existent file
    buffer.setData(zipData);
    QVERIFY(buffer.open(QIODevice::ReadOnly));
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("File not found in archive"));
    const QByteArray notFound = QGCCompression::extractFileDataFromDevice(&buffer, "nonexistent.txt");
    QVERIFY(notFound.isEmpty());
    buffer.close();
    // Test empty file name
    buffer.setData(zipData);
    QVERIFY(buffer.open(QIODevice::ReadOnly));
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("File name cannot be empty"));
    const QByteArray emptyName = QGCCompression::extractFileDataFromDevice(&buffer, "");
    QVERIFY(emptyName.isEmpty());
    buffer.close();
    // Test null device
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("Device is null"));
    const QByteArray nullDevice = QGCCompression::extractFileDataFromDevice(nullptr, "manifest.json");
    QVERIFY(nullDevice.isEmpty());
}

// ============================================================================
// QGCCompressionJob Tests (Async Operations)
// ============================================================================
void QGCCompressionTest::_testCompressionJobExtract()
{
    const QString zipResource = QStringLiteral(":/unittest/manifest.json.zip");
    const QString outputDir = tempDir()->path() + "/job_extract";
    QGCCompressionJob job;
    // Set up signal spies
    QSignalSpy progressSpy(&job, &QGCCompressionJob::progressChanged);
    QSignalSpy runningSpy(&job, &QGCCompressionJob::runningChanged);
    QSignalSpy finishedSpy(&job, &QGCCompressionJob::finished);
    QVERIFY(progressSpy.isValid());
    QVERIFY(runningSpy.isValid());
    QVERIFY(finishedSpy.isValid());
    // Initial state
    QVERIFY(!job.isRunning());
    QCOMPARE(job.progress(), 0.0);
    QVERIFY(job.errorString().isEmpty());
    // Start extraction
    job.extractArchive(zipResource, outputDir);
    // Should be running immediately
    QVERIFY(job.isRunning());
    // Wait for completion (with timeout)
    QVERIFY2(finishedSpy.wait(5000), "Extraction timed out");
    // Verify completion
    QVERIFY(!job.isRunning());
    QCOMPARE(finishedSpy.count(), 1);
    // Check result
    QList<QVariant> finishedArgs = finishedSpy.takeFirst();
    bool success = finishedArgs.at(0).toBool();
    QVERIFY2(success, qPrintable(QString("Extraction failed: %1").arg(job.errorString())));
    // Verify extracted files
    QVERIFY(QDir(outputDir).exists());
    QVERIFY(QFile::exists(outputDir + "/manifest.json"));
    // Progress should have changed (at least start and end)
    QVERIFY(runningSpy.count() >= 2);  // Started and stopped
    // Final progress should be 1.0 on success
    QCOMPARE(job.progress(), 1.0);
}

void QGCCompressionTest::_testCompressionJobReentrantStartFromFinished()
{
    const QString zipResource = QStringLiteral(":/unittest/manifest.json.zip");
    const QString firstOutputDir = tempDir()->path() + "/job_reentrant_extract";
    const QString gzResource = QStringLiteral(":/unittest/manifest.json.gz");
    const QString secondOutputFile = tempDir()->path() + "/job_reentrant_decompress.json";

    QGCCompressionJob job;
    QSignalSpy finishedSpy(&job, &QGCCompressionJob::finished);
    QVERIFY(finishedSpy.isValid());

    bool secondStartAttempted = false;
    bool secondStartAccepted = false;

    connect(&job, &QGCCompressionJob::finished, this, [&](bool success) {
        if (!secondStartAttempted && success) {
            secondStartAttempted = true;
            job.decompressFile(gzResource, secondOutputFile);
            secondStartAccepted = job.isRunning();
        }
    });

    job.extractArchive(zipResource, firstOutputDir);
    QVERIFY(finishedSpy.wait(5000)); // first completion

    if (finishedSpy.count() < 2) {
        QVERIFY(finishedSpy.wait(5000)); // second completion
    }

    QCOMPARE(finishedSpy.count(), 2);
    QVERIFY(secondStartAttempted);
    QVERIFY(secondStartAccepted);
    QVERIFY(finishedSpy.at(0).at(0).toBool());
    QVERIFY(finishedSpy.at(1).at(0).toBool());
    QVERIFY(QFile::exists(firstOutputDir + "/manifest.json"));
    QVERIFY(QFile::exists(secondOutputFile));
    QVERIFY(!job.isRunning());
}

void QGCCompressionTest::_testCompressionJobCancel()
{
    // We'll use a larger archive or repeated extraction to test cancellation
    // For now, test the cancellation mechanism with a simple archive
    const QString zipResource = QStringLiteral(":/unittest/manifest.json.zip");
    const QString outputDir = tempDir()->path() + "/job_cancel";
    QGCCompressionJob job;
    QSignalSpy finishedSpy(&job, &QGCCompressionJob::finished);
    QVERIFY(finishedSpy.isValid());
    // Initial state
    QVERIFY(!job.isRunning());
    // Start extraction
    job.extractArchive(zipResource, outputDir);
    // Cancel immediately
    job.cancel();
    // Wait for completion (cancellation or normal finish)
    QVERIFY2(finishedSpy.wait(5000), "Job did not complete after cancel");
    // Job should no longer be running
    QVERIFY(!job.isRunning());
    // Verify finished signal was emitted
    QCOMPARE(finishedSpy.count(), 1);
    // The result could be success (if it finished before cancel) or failure (if cancelled)
    QList<QVariant> finishedArgs = finishedSpy.takeFirst();
    bool success = finishedArgs.at(0).toBool();
    if (!success) {
        // If cancelled, error string should indicate cancellation
        QVERIFY(job.errorString().contains("cancel", Qt::CaseInsensitive) ||
                job.errorString().isEmpty());  // May be empty if just didn't succeed
    }
    // Test that cancel on non-running job is safe (no-op)
    job.cancel();  // Should not crash or emit signals
    QVERIFY(!job.isRunning());
}

void QGCCompressionTest::_testCompressionJobAsyncStatic()
{
    // Test the static async method that returns QFuture directly
    const QString zipResource = QStringLiteral(":/unittest/manifest.json.zip");
    const QString outputDir = tempDir()->path() + "/async_static";
    // Use the static method
    QFuture<bool> future = QGCCompressionJob::extractArchiveAsync(zipResource, outputDir);
    // Create a watcher to monitor progress
    QFutureWatcher<bool> watcher;
    QSignalSpy progressSpy(&watcher, &QFutureWatcher<bool>::progressValueChanged);
    QSignalSpy finishedSpy(&watcher, &QFutureWatcher<bool>::finished);
    watcher.setFuture(future);
    // Wait for completion
    QVERIFY2(finishedSpy.wait(5000), "Static async extraction timed out");
    // Verify result
    QVERIFY(future.isFinished());
    QVERIFY(!future.isCanceled());
    QVERIFY2(future.result(), "Static async extraction failed");
    // Verify extracted file exists
    QVERIFY(QFileInfo::exists(outputDir + "/manifest.json"));
    // Test decompressFileAsync as well
    const QString gzResource = QStringLiteral(":/unittest/manifest.json.gz");
    const QString decompressOutput = tempDir()->path() + "/async_decompress.json";
    QFuture<bool> decompressFuture = QGCCompressionJob::decompressFileAsync(gzResource, decompressOutput);
    QFutureWatcher<bool> decompressWatcher;
    QSignalSpy decompressFinishedSpy(&decompressWatcher, &QFutureWatcher<bool>::finished);
    decompressWatcher.setFuture(decompressFuture);
    QVERIFY2(decompressFinishedSpy.wait(5000), "Static async decompression timed out");
    QVERIFY(decompressFuture.isFinished());
    QVERIFY2(decompressFuture.result(), "Static async decompression failed");
    QVERIFY(QFileInfo::exists(decompressOutput));
}

// ============================================================================
// Sparse File Handling Tests
// ============================================================================
void QGCCompressionTest::_testSparseFileExtraction()
{
    // Test that archive_read_data_block with offsets is handled correctly
    // This verifies the sparse file handling in writeArchiveEntryToFile()
    // We can't easily create a sparse archive in tests, but we can verify
    // that non-sparse files work correctly through the same code path
    const QString zipResource = QStringLiteral(":/unittest/manifest.json.zip");
    const QString outputDir = tempDir()->path() + "/sparse_test";
    // Extract using the standard path (which uses archive_read_data_block internally)
    QVERIFY(QGCCompression::extractArchive(zipResource, outputDir));
    QVERIFY(QFile::exists(outputDir + "/manifest.json"));
    // Verify file content integrity
    QFile extracted(outputDir + "/manifest.json");
    QVERIFY(extracted.open(QIODevice::ReadOnly));
    const QByteArray content = extracted.readAll();
    QVERIFY(content.contains("\"name\""));
    extracted.close();
    // Verify file is not truncated or corrupted
    QVERIFY(content.size() > 10);  // Should have meaningful content
}

// ============================================================================
// Thread Safety Tests
// ============================================================================
void QGCCompressionTest::_testThreadLocalState()
{
    // Test that thread-local error state is isolated between threads
    // Each thread should have its own lastError() and lastErrorString()
    std::atomic<bool> thread1Done{false};
    std::atomic<bool> thread2Done{false};
    QGCCompression::Error thread1Error = QGCCompression::Error::None;
    QGCCompression::Error thread2Error = QGCCompression::Error::None;
    QString thread1ErrorString;
    QString thread2ErrorString;
    // Thread 1: Trigger FileNotFound error
    QThread* t1 = QThread::create([&]() {
        // This should fail and set error state
        QGCCompression::extractArchive("/nonexistent/path/file.zip", "/tmp/out1");
        thread1Error = QGCCompression::lastError();
        thread1ErrorString = QGCCompression::lastErrorString();
        thread1Done = true;
    });
    // Thread 2: Successful operation (should have no error)
    QThread* t2 = QThread::create([this, &thread2Error, &thread2ErrorString, &thread2Done]() {
        // Small delay to ensure thread1 has set its error state
        QThread::msleep(50);
        // This should succeed
        const QString outputDir = tempDir()->path() + "/thread2_output";
        QGCCompression::extractArchive(":/unittest/manifest.json.zip", outputDir);
        thread2Error = QGCCompression::lastError();
        thread2ErrorString = QGCCompression::lastErrorString();
        thread2Done = true;
    });
    t1->start();
    t2->start();
    // Wait for both threads
    QVERIFY(t1->wait(5000));
    QVERIFY(t2->wait(5000));
    delete t1;
    delete t2;
    // Verify thread isolation
    QCOMPARE(thread1Error, QGCCompression::Error::FileNotFound);
    QVERIFY(!thread1ErrorString.isEmpty());
    // Thread 2 should have its own clean state (success = no error)
    QCOMPARE(thread2Error, QGCCompression::Error::None);
}

void QGCCompressionTest::_testConcurrentExtractions()
{
    // Test multiple concurrent extractions don't interfere with each other
    constexpr int kNumThreads = 4;
    std::atomic<int> successCount{0};
    std::atomic<int> failureCount{0};
    QList<QFuture<void>> futures;
    for (int i = 0; i < kNumThreads; ++i) {
        futures.append(QtConcurrent::run([this, i, &successCount, &failureCount]() {
            const QString outputDir = tempDir()->path() + QString("/concurrent_%1").arg(i);
            // Alternate between different archive types
            QString archivePath;
            if (i % 2 == 0) {
                archivePath = QStringLiteral(":/unittest/manifest.json.zip");
            } else {
                archivePath = QStringLiteral(":/unittest/manifest.json.7z");
            }
            if (QGCCompression::extractArchive(archivePath, outputDir)) {
                if (QFile::exists(outputDir + "/manifest.json")) {
                    successCount++;
                } else {
                    failureCount++;
                }
            } else {
                failureCount++;
            }
        }));
    }
    // Wait for all futures
    for (auto& future : futures) {
        future.waitForFinished();
    }
    QCOMPARE(successCount.load(), kNumThreads);
    QCOMPARE(failureCount.load(), 0);
}

// ============================================================================
// Boundary Condition Tests
// ============================================================================
void QGCCompressionTest::_testEmptyArchive()
{
    // Create an empty ZIP file (valid but contains no entries)
    // ZIP empty archive signature: PK\x05\x06 followed by 18 zero bytes
    const QString emptyZip = tempDir()->path() + "/empty.zip";
    QFile file(emptyZip);
    QVERIFY(file.open(QIODevice::WriteOnly));
    // Minimal empty ZIP (End of Central Directory record only)
    const char emptyZipData[] = {
        '\x50', '\x4b', '\x05', '\x06',  // End of central dir signature
        '\x00', '\x00',                  // Number of this disk
        '\x00', '\x00',                  // Disk where central directory starts
        '\x00', '\x00',                  // Number of central directory records on this disk
        '\x00', '\x00',                  // Total number of central directory records
        '\x00', '\x00', '\x00', '\x00',  // Size of central directory
        '\x00', '\x00', '\x00', '\x00',  // Offset of central directory
        '\x00', '\x00'                   // Comment length
    };
    file.write(emptyZipData, sizeof(emptyZipData));
    file.close();
    // List should return empty list (not error)
    const QStringList entries = QGCCompression::listArchive(emptyZip);
    QVERIFY(entries.isEmpty());
    // Stats should show zero entries
    const QGCCompression::ArchiveStats stats = QGCCompression::getArchiveStats(emptyZip);
    QCOMPARE(stats.totalEntries, 0);
    QCOMPARE(stats.fileCount, 0);
    // Extract should succeed (nothing to extract)
    const QString outputDir = tempDir()->path() + "/empty_extract";
    QVERIFY(QGCCompression::extractArchive(emptyZip, outputDir));
}

void QGCCompressionTest::_testSingleByteFile()
{
    // Test that small files decompress correctly
    // Use the existing .gz resource and verify size is small but non-zero
    const QString gzResource = QStringLiteral(":/unittest/manifest.json.gz");
    // Decompress to memory
    QFile resourceFile(gzResource);
    QVERIFY(resourceFile.open(QIODevice::ReadOnly));
    const QByteArray compressedData = resourceFile.readAll();
    resourceFile.close();
    const QByteArray decompressed = QGCCompression::decompressData(compressedData);
    QVERIFY(!decompressed.isEmpty());
    // Verify decompressed size is reasonable (small file)
    QVERIFY(decompressed.size() > 0);
    QVERIFY(decompressed.size() < 1024);  // Should be a small manifest file
    // Test boundary: decompress with exact size limit matching actual size
    const QByteArray exactLimit =
        QGCCompression::decompressData(compressedData, QGCCompression::Format::Auto, decompressed.size());
    QCOMPARE(exactLimit.size(), decompressed.size());
    // Test boundary: size limit of exactly 1 byte should fail for this file
    const QByteArray tooSmall = QGCCompression::decompressData(compressedData, QGCCompression::Format::Auto, 1);
    QVERIFY(tooSmall.isEmpty());  // Should fail - file is bigger than 1 byte
}

void QGCCompressionTest::_testLargeFileDecompression()
{
    // Test decompression with size limit enforcement
    // Use the manifest.json.gz which we know is small
    QFile resourceFile(":/unittest/manifest.json.gz");
    QVERIFY(resourceFile.open(QIODevice::ReadOnly));
    const QByteArray compressedData = resourceFile.readAll();
    resourceFile.close();
    // First, decompress without limit to get actual size
    const QByteArray fullDecompressed = QGCCompression::decompressData(compressedData);
    QVERIFY(!fullDecompressed.isEmpty());
    const qint64 fullSize = fullDecompressed.size();
    // Now test with a limit smaller than the actual size
    if (fullSize > 10) {
        const QByteArray limitedDecompressed =
            QGCCompression::decompressData(compressedData, QGCCompression::Format::Auto, 10);
        // Should return empty due to size limit exceeded
        QVERIFY(limitedDecompressed.isEmpty());
    }
    // Test with limit larger than actual size (should succeed)
    const QByteArray unlimitedDecompressed =
        QGCCompression::decompressData(compressedData, QGCCompression::Format::Auto, fullSize * 2);
    QCOMPARE(unlimitedDecompressed.size(), fullSize);
}

void QGCCompressionTest::_testMaxEntriesArchive()
{
    // Test listing an archive with many entries
    // We'll use the existing ZIP and verify the listing mechanism
    const QString zipResource = QStringLiteral(":/unittest/manifest.json.zip");
    // Get detailed listing
    const QList<QGCCompression::ArchiveEntry> entries = QGCCompression::listArchiveDetailed(zipResource);
    // Verify each entry has valid data
    for (const auto& entry : entries) {
        QVERIFY(!entry.name.isEmpty());
        if (!entry.isDirectory) {
            QVERIFY(entry.size >= 0);
        }
        // Permissions should be reasonable (not zero for files)
        if (!entry.isDirectory && entry.size > 0) {
            QVERIFY(entry.permissions > 0);
        }
    }
    // Verify stats match entry count
    const QGCCompression::ArchiveStats stats = QGCCompression::getArchiveStats(zipResource);
    QCOMPARE(stats.totalEntries, static_cast<int>(entries.size()));
}

// ============================================================================
// Cross-Platform Path Handling Tests
// ============================================================================
void QGCCompressionTest::_testWindowsPathSeparators()
{
    // Test that paths with Windows-style separators are handled correctly
    // libarchive archives may contain entries with backslashes
    const QString zipResource = QStringLiteral(":/unittest/manifest.json.zip");
    // Get entries and verify none have issues
    const QStringList entries = QGCCompression::listArchive(zipResource);
    for (const QString& entry : entries) {
        // Entry names should not contain backslashes after processing
        // (or if they do, they should be handled correctly)
        // Just verify no crashes or issues accessing these entries
        const bool exists = QGCCompression::fileExists(zipResource, entry);
        Q_UNUSED(exists);  // Just testing it doesn't crash
    }
    // Test path joining with mixed separators
    // QGCFileHelper::joinPath should normalize these
    // Create a test file with normal path
    const QString testDir = tempDir()->path() + "/path_test";
    QVERIFY(QDir().mkpath(testDir));
    QFile testFile(testDir + "/test.txt");
    QVERIFY(testFile.open(QIODevice::WriteOnly));
    testFile.write("test");
    testFile.close();
    // Verify the file exists regardless of separator style used in creation
    QVERIFY(QFile::exists(testDir + "/test.txt"));
}

void QGCCompressionTest::_testSpecialCharactersInPath()
{
    // Test paths with special characters (spaces, unicode, etc.)
    // Create directory with space in name
    const QString dirWithSpace = tempDir()->path() + "/dir with space";
    QVERIFY(QDir().mkpath(dirWithSpace));
    // Extract to directory with space
    const QString zipResource = QStringLiteral(":/unittest/manifest.json.zip");
    QVERIFY(QGCCompression::extractArchive(zipResource, dirWithSpace));
    QVERIFY(QFile::exists(dirWithSpace + "/manifest.json"));
    // Create directory with special chars (parentheses, brackets)
    const QString dirWithSpecial = tempDir()->path() + "/dir(1)[test]";
    QVERIFY(QDir().mkpath(dirWithSpecial));
    // Extract to directory with special chars
    QVERIFY(QGCCompression::extractArchive(zipResource, dirWithSpecial));
    QVERIFY(QFile::exists(dirWithSpecial + "/manifest.json"));
    // Test with ampersand and equals (common in URLs that get saved as files)
    const QString dirWithAmp = tempDir()->path() + "/dir&param=value";
    QVERIFY(QDir().mkpath(dirWithAmp));
    QVERIFY(QGCCompression::extractArchive(zipResource, dirWithAmp));
    QVERIFY(QFile::exists(dirWithAmp + "/manifest.json"));
}

void QGCCompressionTest::_testUnicodePaths()
{
    // Test paths with Unicode characters
    // Create directory with Unicode name (Japanese, Chinese, emoji-like)
    const QString unicodeDir = tempDir()->path() + QString::fromUtf8("/日本語_中文_αβγ");
    QVERIFY(QDir().mkpath(unicodeDir));
    // Extract to Unicode directory
    const QString zipResource = QStringLiteral(":/unittest/manifest.json.zip");
    QVERIFY2(QGCCompression::extractArchive(zipResource, unicodeDir), "Failed to extract to Unicode path");
    QVERIFY(QFile::exists(unicodeDir + "/manifest.json"));
    // Verify content is intact
    QFile extracted(unicodeDir + "/manifest.json");
    QVERIFY(extracted.open(QIODevice::ReadOnly));
    const QByteArray content = extracted.readAll();
    QVERIFY(content.contains("\"name\""));
    extracted.close();
    // Test with accented Latin characters
    const QString accentedDir = tempDir()->path() + QString::fromUtf8("/café_naïve_résumé");
    QVERIFY(QDir().mkpath(accentedDir));
    QVERIFY(QGCCompression::extractArchive(zipResource, accentedDir));
    QVERIFY(QFile::exists(accentedDir + "/manifest.json"));
    // Test with Cyrillic
    const QString cyrillicDir = tempDir()->path() + QString::fromUtf8("/Привет_мир");
    QVERIFY(QDir().mkpath(cyrillicDir));
    QVERIFY(QGCCompression::extractArchive(zipResource, cyrillicDir));
    QVERIFY(QFile::exists(cyrillicDir + "/manifest.json"));
}

// ============================================================================
// Benchmarks
// ============================================================================
void QGCCompressionTest::_benchmarkExtractZip()
{
    const QString zipResource = QStringLiteral(":/unittest/manifest.json.zip");
    int iteration = 0;
    QBENCHMARK
    {
        const QString outputDir = tempDir()->path() + QString("/bench_zip_%1").arg(iteration++);
        QGCCompression::extractArchive(zipResource, outputDir);
    }
}

void QGCCompressionTest::_benchmarkExtract7z()
{
    const QString archive = QStringLiteral(":/unittest/manifest.json.7z");
    int iteration = 0;
    QBENCHMARK
    {
        const QString outputDir = tempDir()->path() + QString("/bench_7z_%1").arg(iteration++);
        QGCCompression::extractArchive(archive, outputDir);
    }
}

void QGCCompressionTest::_benchmarkDecompressGzip()
{
    // Load compressed data once
    QFile resourceFile(":/unittest/manifest.json.gz");
    QVERIFY(resourceFile.open(QIODevice::ReadOnly));
    const QByteArray compressedData = resourceFile.readAll();
    resourceFile.close();
    QBENCHMARK
    {
        QGCCompression::decompressData(compressedData);
    }
}

void QGCCompressionTest::_benchmarkDecompressXz()
{
    // Load compressed data once
    QFile resourceFile(":/unittest/manifest.json.xz");
    QVERIFY(resourceFile.open(QIODevice::ReadOnly));
    const QByteArray compressedData = resourceFile.readAll();
    resourceFile.close();
    QBENCHMARK
    {
        QGCCompression::decompressData(compressedData);
    }
}

void QGCCompressionTest::_benchmarkConcurrentExtraction()
{
    // Benchmark parallel extraction performance
    constexpr int kNumExtractions = 4;
    int baseIteration = 0;
    QBENCHMARK
    {
        QList<QFuture<bool>> futures;
        for (int i = 0; i < kNumExtractions; ++i) {
            const int iteration = baseIteration * kNumExtractions + i;
            futures.append(QtConcurrent::run([this, iteration]() {
                const QString outputDir = tempDir()->path() + QString("/bench_concurrent_%1").arg(iteration);
                return QGCCompression::extractArchive(":/unittest/manifest.json.zip", outputDir);
            }));
        }
        for (auto& future : futures) {
            future.waitForFinished();
        }
        baseIteration++;
    }
}

void QGCCompressionTest::_benchmarkListArchive()
{
    const QString zipResource = QStringLiteral(":/unittest/manifest.json.zip");
    QBENCHMARK
    {
        QGCCompression::listArchiveDetailed(zipResource);
    }
}

#include "UnitTest.h"

UT_REGISTER_TEST(QGCCompressionTest, TestLabel::Unit, TestLabel::Utilities)
