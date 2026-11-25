#include "QGCStreamingDecompressionTest.h"

#include <memory>

#include <QtCore/QBuffer>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QRegularExpression>
#include <QtCore/QTemporaryDir>
#include <QtCore/QTextStream>

#include "FileFixtures.h"
#include "QGCArchiveFile.h"
#include "QGCDecompressDevice.h"

// ============================================================================
// Both Devices
// ============================================================================
void QGCStreamingDecompressionTest::_testReadsContent_data()
{
    QTest::addColumn<bool>("archive");
    QTest::addColumn<QString>("resource");
    // What the device is given: the path of the "resource", that of a "file", or a "buffer"
    QTest::addColumn<QString>("source");

    const QString gz = QStringLiteral(":/unittest/manifest.json.gz");
    const QString zip = QStringLiteral(":/unittest/manifest.json.zip");

    QTest::newRow("gz resource") << false << gz << "resource";
    QTest::newRow("xz resource") << false << QStringLiteral(":/unittest/manifest.json.xz") << "resource";
    QTest::newRow("zst resource") << false << QStringLiteral(":/unittest/manifest.json.zst") << "resource";
#ifdef QGC_ENABLE_BZIP2
    QTest::newRow("bz2 resource") << false << QStringLiteral(":/unittest/manifest.json.bz2") << "resource";
#endif
#ifdef QGC_ENABLE_LZ4
    QTest::newRow("lz4 resource") << false << QStringLiteral(":/unittest/manifest.json.lz4") << "resource";
#endif
    QTest::newRow("gz file") << false << gz << "file";
    QTest::newRow("gz buffer") << false << gz << "buffer";
    QTest::newRow("zip resource") << true << zip << "resource";
    QTest::newRow("7z resource") << true << QStringLiteral(":/unittest/manifest.json.7z") << "resource";
    QTest::newRow("zip buffer") << true << zip << "buffer";
}

void QGCStreamingDecompressionTest::_testReadsContent()
{
    QFETCH(bool, archive);
    QFETCH(QString, resource);
    QFETCH(QString, source);

    QTemporaryDir tempDir;
    QString path = resource;
    if (source == "file") {
        path = tempDir.filePath(QFileInfo(resource).fileName());
        QVERIFY(QFile::copy(resource, path));
    }
    QBuffer buffer;
    buffer.setData(TestFixtures::readFile(resource));
    QVERIFY(buffer.open(QIODevice::ReadOnly));

    const QString entryName = QStringLiteral("manifest.json");
    std::unique_ptr<QGCArchiveDeviceBase> device;
    if (source == "buffer") {
        if (archive) {
            device = std::make_unique<QGCArchiveFile>(&buffer, entryName);
        } else {
            device = std::make_unique<QGCDecompressDevice>(&buffer);
        }
    } else if (archive) {
        device = std::make_unique<QGCArchiveFile>(path, entryName);
    } else {
        device = std::make_unique<QGCDecompressDevice>(path);
    }

    QVERIFY2(device->open(QIODevice::ReadOnly), qPrintable(device->errorString()));
    QVERIFY(device->isSequential());
    if (archive) {
        const auto* entry = static_cast<const QGCArchiveFile*>(device.get());
        QVERIFY(entry->entryFound());
        QVERIFY(entry->entrySize() > 0);
    }
    QVERIFY(device->readAll().contains("\"name\""));
}

// ============================================================================
// QGCDecompressDevice Tests
// ============================================================================
void QGCStreamingDecompressionTest::_testDecompressDeviceWithQTextStream()
{
    QGCDecompressDevice device(":/unittest/manifest.json.gz");
    QVERIFY(device.open(QIODevice::ReadOnly));
    QTextStream stream(&device);
    const QString content = stream.readAll();
    QVERIFY(!content.isEmpty());
    QVERIFY(content.contains("\"name\""));
    QVERIFY(content.contains("\"version\""));
}

void QGCStreamingDecompressionTest::_testDecompressDeviceWithQJsonDocument()
{
    QGCDecompressDevice device(":/unittest/manifest.json.gz");
    QVERIFY(device.open(QIODevice::ReadOnly));
    const QByteArray data = device.readAll();
    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    QCOMPARE(error.error, QJsonParseError::NoError);
    QVERIFY(doc.isObject());
    QVERIFY(doc.object().contains("name"));
}

void QGCStreamingDecompressionTest::_testDecompressDeviceErrors()
{
    // Test null source
    QGCDecompressDevice device1(static_cast<QIODevice*>(nullptr));
    QVERIFY(!device1.open(QIODevice::ReadOnly));
    // Test write mode
    QGCDecompressDevice device2(":/unittest/manifest.json.gz");
    QVERIFY(!device2.open(QIODevice::WriteOnly));
    // Test non-existent file
    QGCDecompressDevice device3("/nonexistent/file.gz");
    QVERIFY(!device3.open(QIODevice::ReadOnly));
    // Test writeData returns -1 (QIODevice short-circuits write calls on ReadOnly devices)
    QGCDecompressDevice device4(":/unittest/manifest.json.gz");
    QVERIFY(device4.open(QIODevice::ReadOnly));
    expectLogMessage("default", QtWarningMsg, QRegularExpression("ReadOnly device"));
    QCOMPARE(device4.write("test", 4), qint64(-1));
    verifyExpectedLogMessage();
    // A stream which cannot be opened: the reason is in errorString(), and nothing is logged
    QBuffer headerOnly;
    headerOnly.setData(QByteArray::fromHex("1f8b08000000000000ff"));
    QVERIFY(headerOnly.open(QIODevice::ReadOnly));
    QGCDecompressDevice device5(&headerOnly);
    QVERIFY(!device5.open(QIODevice::ReadOnly));
    QVERIFY2(device5.errorString().contains("truncated gzip input"), qPrintable(device5.errorString()));
}

void QGCStreamingDecompressionTest::_testDecompressDeviceFormatInfo()
{
    QGCDecompressDevice device(":/unittest/manifest.json.gz");
    QVERIFY(device.open(QIODevice::ReadOnly));
    // Format info available after open
    QVERIFY(!device.filterName().isEmpty());
    QCOMPARE(device.filterName(), QStringLiteral("gzip"));
}

// ============================================================================
// QGCArchiveFile Tests
// ============================================================================
void QGCStreamingDecompressionTest::_testArchiveFileWithQTextStream()
{
    QGCArchiveFile device(":/unittest/manifest.json.zip", "manifest.json");
    QVERIFY(device.open(QIODevice::ReadOnly));
    QTextStream stream(&device);
    const QString content = stream.readAll();
    QVERIFY(!content.isEmpty());
    QVERIFY(content.contains("\"name\""));
}

void QGCStreamingDecompressionTest::_testArchiveFileWithQJsonDocument()
{
    QGCArchiveFile device(":/unittest/manifest.json.zip", "manifest.json");
    QVERIFY(device.open(QIODevice::ReadOnly));
    const QByteArray data = device.readAll();
    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    QCOMPARE(error.error, QJsonParseError::NoError);
    QVERIFY(doc.isObject());
    QVERIFY(doc.object().contains("name"));
}

void QGCStreamingDecompressionTest::_testArchiveFileNotFound()
{
    QGCArchiveFile device(":/unittest/manifest.json.zip", "nonexistent.txt");
    QVERIFY(!device.open(QIODevice::ReadOnly));
    QVERIFY(!device.entryFound());
    QVERIFY(device.errorString().contains("not found"));
    // The archive was read to its end, which tells its format
    QVERIFY2(device.formatName().startsWith("ZIP"), qPrintable(device.formatName()));
}

void QGCStreamingDecompressionTest::_testArchiveFileErrors()
{
    // Test empty entry name
    QGCArchiveFile device1(":/unittest/manifest.json.zip", "");
    QVERIFY(!device1.open(QIODevice::ReadOnly));
    // Test write mode
    QGCArchiveFile device2(":/unittest/manifest.json.zip", "manifest.json");
    QVERIFY(!device2.open(QIODevice::WriteOnly));
    // Test non-existent archive
    QGCArchiveFile device3("/nonexistent/archive.zip", "file.txt");
    QVERIFY(!device3.open(QIODevice::ReadOnly));
    // Test writeData returns -1
    QGCArchiveFile device4(":/unittest/manifest.json.zip", "manifest.json");
    QVERIFY(device4.open(QIODevice::ReadOnly));
    expectLogMessage("default", QtWarningMsg, QRegularExpression("ReadOnly device"));
    QCOMPARE(device4.write("test", 4), qint64(-1));
    verifyExpectedLogMessage();
    // Data which is not an archive: the reason is in errorString(), and nothing is logged
    QBuffer notAnArchive;
    notAnArchive.setData("This is not an archive");
    QVERIFY(notAnArchive.open(QIODevice::ReadOnly));
    QGCArchiveFile device5(&notAnArchive, "file.txt");
    QVERIFY(!device5.open(QIODevice::ReadOnly));
    QVERIFY2(device5.errorString().contains("Unrecognized archive format"), qPrintable(device5.errorString()));
}

void QGCStreamingDecompressionTest::_testArchiveFileMetadata()
{
    QGCArchiveFile device(":/unittest/manifest.json.zip", "manifest.json");
    QVERIFY(device.open(QIODevice::ReadOnly));
    QVERIFY(device.entryFound());
    QCOMPARE(device.entryName(), QStringLiteral("manifest.json"));
    QVERIFY(device.entrySize() > 0);
    QVERIFY(!device.formatName().isEmpty());
}

// ============================================================================
// Edge Cases
// ============================================================================
void QGCStreamingDecompressionTest::_testPartialReads()
{
    QGCDecompressDevice device(":/unittest/manifest.json.gz");
    QVERIFY(device.open(QIODevice::ReadOnly));
    // Read in small chunks
    QByteArray full;
    char buf[16];
    qint64 bytesRead;
    while ((bytesRead = device.read(buf, sizeof(buf))) > 0) {
        full.append(buf, static_cast<int>(bytesRead));
    }
    QVERIFY(!full.isEmpty());
    QVERIFY(full.contains("\"name\""));
}

void QGCStreamingDecompressionTest::_testMultipleOpens()
{
    QGCDecompressDevice device(":/unittest/manifest.json.gz");
    // First open/read/close
    QVERIFY(device.open(QIODevice::ReadOnly));
    const QByteArray data1 = device.readAll();
    device.close();
    // Second open/read/close - should work the same
    QVERIFY(device.open(QIODevice::ReadOnly));
    const QByteArray data2 = device.readAll();
    QCOMPARE(data1, data2);
}

void QGCStreamingDecompressionTest::_testErrorString_data()
{
    QTest::addColumn<bool>("archive");

    QTest::newRow("QGCDecompressDevice") << false;
    QTest::newRow("QGCArchiveFile") << true;
}

void QGCStreamingDecompressionTest::_testErrorString()
{
    QFETCH(bool, archive);

    QBuffer source;
    source.setData(TestFixtures::readFile(archive ? ":/unittest/manifest.json.zip" : ":/unittest/manifest.json.gz"));
    std::unique_ptr<QGCArchiveDeviceBase> device;
    if (archive) {
        device = std::make_unique<QGCArchiveFile>(&source, QStringLiteral("manifest.json"));
    } else {
        device = std::make_unique<QGCDecompressDevice>(&source);
    }
    // QIODevice::errorString() is not virtual: this is what a caller holding a QIODevice sees
    const QIODevice& asDevice = *device;

    // The source is not open yet
    QVERIFY(!device->open(QIODevice::ReadOnly));
    QCOMPARE(device->errorString(), QStringLiteral("Source device is not ready"));
    QCOMPARE(asDevice.errorString(), device->errorString());

    // The failure is forgotten when the device opens, and a stream read to its end leaves no error
    QVERIFY(source.open(QIODevice::ReadOnly));
    QVERIFY(device->open(QIODevice::ReadOnly));
    QVERIFY2(device->errorString().isEmpty(), qPrintable(device->errorString()));
    QVERIFY(device->readAll().contains("\"name\""));
    QVERIFY2(device->errorString().isEmpty(), qPrintable(device->errorString()));
}

UT_REGISTER_TEST(QGCStreamingDecompressionTest, TestLabel::Unit, TestLabel::Utilities)
