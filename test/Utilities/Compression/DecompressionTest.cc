#include "DecompressionTest.h"
#include "QGCLZMA.h"
#include "QGCZlib.h"
#include "QGCZip.h"

#include <QtTest/QTest>

void DecompressionTest::_testDecompressGzip()
{
    const QString gzippedFileName = QStringLiteral(":/manifest.json.gz");
	const QString decompressedFilename = QStringLiteral("manifest.json");
    const bool result = QGCZlib::inflateGzipFile(gzippedFileName, decompressedFilename);
	QVERIFY(result);
}

void DecompressionTest::_testDecompressLZMA()
{
    const QString lzmaFilename = QStringLiteral(":/manifest.json.xz");
	const QString decompressedFilename = QStringLiteral("manifest.json");
    const bool result = QGCLZMA::inflateLZMAFile(lzmaFilename, decompressedFilename);
	QVERIFY(result);
}

void DecompressionTest::_testUnzip()
{
    const QString zipFilename = QStringLiteral(":/manifest.json.zip");
    const QString decompressedPath = QDir::tempPath() + "/QGC_DECOMPRESSION_TEST";
    const bool result = QGCZip::unzipFile(zipFilename, decompressedPath);
    QVERIFY(result);
}
