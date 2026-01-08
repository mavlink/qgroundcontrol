#include "QGCStreamingDecompressionTest.h"
#include "QGCDecompressDevice.h"
#include "QGCArchiveFile.h"

#include <QtCore/QBuffer>
#include <QtCore/QFile>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QTextStream>
#include <QtTest/QTest>

void QGCStreamingDecompressionTest::init()
{
    UnitTest::init();
    _tempDir = new QTemporaryDir();
    QVERIFY(_tempDir->isValid());
}

void QGCStreamingDecompressionTest::cleanup()
{
    delete _tempDir;
    _tempDir = nullptr;
    UnitTest::cleanup();
}

// ============================================================================
// QGCDecompressDevice Tests
// ============================================================================

void QGCStreamingDecompressionTest::_testDecompressDeviceFromFile()
{
    // Copy resource to temp file for testing
    QFile resource(":/unittest/manifest.json.gz");
    QVERIFY(resource.open(QIODevice::ReadOnly));
    const QByteArray compressedData = resource.readAll();
    resource.close();

    const QString tempFile = _tempDir->filePath("test.json.gz");
    QFile outFile(tempFile);
    QVERIFY(outFile.open(QIODevice::WriteOnly));
    outFile.write(compressedData);
    outFile.close();

    // Test decompression from file
    QGCDecompressDevice device(tempFile);
    QVERIFY(device.open(QIODevice::ReadOnly));
    QVERIFY(device.isSequential());

    const QByteArray decompressed = device.readAll();
    QVERIFY(!decompressed.isEmpty());
    QVERIFY(decompressed.contains("\"name\""));

    device.close();
}

void QGCStreamingDecompressionTest::_testDecompressDeviceFromQBuffer()
{
    // Load compressed data into QBuffer
    QFile resource(":/unittest/manifest.json.gz");
    QVERIFY(resource.open(QIODevice::ReadOnly));
    const QByteArray compressedData = resource.readAll();
    resource.close();

    QBuffer buffer;
    buffer.setData(compressedData);
    QVERIFY(buffer.open(QIODevice::ReadOnly));

    QGCDecompressDevice device(&buffer);
    QVERIFY(device.open(QIODevice::ReadOnly));

    const QByteArray decompressed = device.readAll();
    QVERIFY(!decompressed.isEmpty());
    QVERIFY(decompressed.contains("\"name\""));

    device.close();
}

void QGCStreamingDecompressionTest::_testDecompressDeviceWithQTextStream()
{
    QGCDecompressDevice device(":/unittest/manifest.json.gz");
    QVERIFY(device.open(QIODevice::ReadOnly));

    QTextStream stream(&device);
    const QString content = stream.readAll();

    QVERIFY(!content.isEmpty());
    QVERIFY(content.contains("\"name\""));
    QVERIFY(content.contains("\"version\""));

    device.close();
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

    device.close();
}

void QGCStreamingDecompressionTest::_testDecompressDeviceFormats()
{
    // Test all supported compression formats
    const QStringList formats = {
        ":/unittest/manifest.json.gz",
        ":/unittest/manifest.json.xz",
        ":/unittest/manifest.json.zst",
        ":/unittest/manifest.json.bz2",
        ":/unittest/manifest.json.lz4"
    };

    for (const QString &format : formats) {
        QGCDecompressDevice device(format);
        QVERIFY2(device.open(QIODevice::ReadOnly), qPrintable(format));

        const QByteArray data = device.readAll();
        QVERIFY2(!data.isEmpty(), qPrintable(format));
        QVERIFY2(data.contains("\"name\""), qPrintable(format));

        device.close();
    }
}

void QGCStreamingDecompressionTest::_testDecompressDeviceFromQtResource()
{
    QGCDecompressDevice device(":/unittest/manifest.json.gz");
    QVERIFY(device.open(QIODevice::ReadOnly));

    const QByteArray data = device.readAll();
    QVERIFY(!data.isEmpty());
    QVERIFY(data.contains("\"name\""));

    device.close();
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

    // Test writeData returns -1
    QGCDecompressDevice device4(":/unittest/manifest.json.gz");
    QVERIFY(device4.open(QIODevice::ReadOnly));
    QCOMPARE(device4.write("test", 4), qint64(-1));
    device4.close();
}

void QGCStreamingDecompressionTest::_testDecompressDeviceFormatInfo()
{
    QGCDecompressDevice device(":/unittest/manifest.json.gz");
    QVERIFY(device.open(QIODevice::ReadOnly));

    // Format info available after open
    QVERIFY(!device.filterName().isEmpty());
    QCOMPARE(device.filterName(), QStringLiteral("gzip"));

    device.close();
}

// ============================================================================
// QGCArchiveFile Tests
// ============================================================================

void QGCStreamingDecompressionTest::_testArchiveFileFromZip()
{
    QGCArchiveFile device(":/unittest/manifest.json.zip", "manifest.json");
    QVERIFY(device.open(QIODevice::ReadOnly));
    QVERIFY(device.entryFound());
    QVERIFY(device.entrySize() > 0);

    const QByteArray content = device.readAll();
    QVERIFY(!content.isEmpty());
    QVERIFY(content.contains("\"name\""));

    device.close();
}

void QGCStreamingDecompressionTest::_testArchiveFileFrom7z()
{
    QGCArchiveFile device(":/unittest/manifest.json.7z", "manifest.json");
    QVERIFY(device.open(QIODevice::ReadOnly));
    QVERIFY(device.entryFound());

    const QByteArray content = device.readAll();
    QVERIFY(!content.isEmpty());
    QVERIFY(content.contains("\"name\""));

    device.close();
}

void QGCStreamingDecompressionTest::_testArchiveFileFromQBuffer()
{
    // Load archive into QBuffer
    QFile resource(":/unittest/manifest.json.zip");
    QVERIFY(resource.open(QIODevice::ReadOnly));
    const QByteArray archiveData = resource.readAll();
    resource.close();

    QBuffer buffer;
    buffer.setData(archiveData);
    QVERIFY(buffer.open(QIODevice::ReadOnly));

    QGCArchiveFile device(&buffer, "manifest.json");
    QVERIFY(device.open(QIODevice::ReadOnly));
    QVERIFY(device.entryFound());

    const QByteArray content = device.readAll();
    QVERIFY(!content.isEmpty());
    QVERIFY(content.contains("\"name\""));

    device.close();
}

void QGCStreamingDecompressionTest::_testArchiveFileWithQTextStream()
{
    QGCArchiveFile device(":/unittest/manifest.json.zip", "manifest.json");
    QVERIFY(device.open(QIODevice::ReadOnly));

    QTextStream stream(&device);
    const QString content = stream.readAll();

    QVERIFY(!content.isEmpty());
    QVERIFY(content.contains("\"name\""));

    device.close();
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

    device.close();
}

void QGCStreamingDecompressionTest::_testArchiveFileNotFound()
{
    QGCArchiveFile device(":/unittest/manifest.json.zip", "nonexistent.txt");
    QVERIFY(!device.open(QIODevice::ReadOnly));
    QVERIFY(!device.entryFound());
    QVERIFY(device.errorString().contains("not found"));
}

void QGCStreamingDecompressionTest::_testArchiveFileFromQtResource()
{
    QGCArchiveFile device(":/unittest/manifest.json.zip", "manifest.json");
    QVERIFY(device.open(QIODevice::ReadOnly));

    const QByteArray content = device.readAll();
    QVERIFY(!content.isEmpty());
    QVERIFY(content.contains("\"name\""));

    device.close();
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
    QCOMPARE(device4.write("test", 4), qint64(-1));
    device4.close();
}

void QGCStreamingDecompressionTest::_testArchiveFileMetadata()
{
    QGCArchiveFile device(":/unittest/manifest.json.zip", "manifest.json");
    QVERIFY(device.open(QIODevice::ReadOnly));

    QVERIFY(device.entryFound());
    QCOMPARE(device.entryName(), QStringLiteral("manifest.json"));
    QVERIFY(device.entrySize() > 0);
    QVERIFY(!device.formatName().isEmpty());

    device.close();
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

    device.close();
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
    device.close();

    QCOMPARE(data1, data2);
}
