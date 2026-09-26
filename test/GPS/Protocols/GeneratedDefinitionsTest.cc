#include "GeneratedDefinitionsTest.h"

#include <cstdint>
#include <span>

#include <QtCore/QByteArray>
#include <QtCore/QDir>
#include <QtCore/QFile>

#include "Checksums.h"
#include "SBF/Generated/SBFBlocks.h"
#include "WireFields.h"

namespace {
// SBF Reference Guide: every block starts with "$@"; PVTGeodetic is block 4007 and QGC reads it up to VAccuracy.
static_assert(SBF::SYNC1 == '$' && SBF::SYNC2 == '@');
static_assert(SBF::BlockId::PVT_GEODETIC == 4007);
static_assert(Wire::SIZE<SBF::PVTGeodetic> == 94);
}  // namespace

void GeneratedDefinitionsTest::_sbfCorpusBlocks_data()
{
    QTest::addColumn<QString>("path");

    const QDir corpus(QStringLiteral(GPS_PROTOCOL_CORPUS_DIR));
    const QStringList names = corpus.entryList({QStringLiteral("*.sbf")}, QDir::Files, QDir::Name);
    for (const QString& name : names) {
        QTest::newRow(qPrintable(name)) << corpus.filePath(name);
    }
    if (names.isEmpty()) {
        QTest::newRow("no SBF corpus") << corpus.filePath(QStringLiteral("*.sbf"));
    }
}

void GeneratedDefinitionsTest::_sbfCorpusBlocks()
{
    QFETCH(QString, path);

    QFile file(path);
    QVERIFY2(file.open(QIODevice::ReadOnly), qPrintable(path));
    const QByteArray data = file.readAll();
    const std::span bytes(reinterpret_cast<const uint8_t*>(data.constData()), static_cast<size_t>(data.size()));

    int pvtGeodeticBlocks = 0;
    size_t start = 0;
    while (start < bytes.size()) {
        QVERIFY(bytes.size() - start >= Wire::SIZE<SBF::BlockHeader>);
        QCOMPARE(bytes[start], SBF::SYNC1);
        QCOMPARE(bytes[start + 1], SBF::SYNC2);
        const auto header = Wire::decode<SBF::BlockHeader>(bytes, start);
        QVERIFY(header.length >= Wire::SIZE<SBF::BlockHeader> && header.length <= bytes.size() - start);
        const auto block = bytes.subspan(start, header.length);
        start += header.length;

        // The CRC covers the block from its id field on; a match proves the crc and length fields decode in place.
        QCOMPARE(header.crc, QGC::crc16Ccitt(block.subspan(4)));
        if (header.number() == SBF::BlockId::PVT_GEODETIC) {
            ++pvtGeodeticBlocks;
            QVERIFY(header.length >= Wire::SIZE<SBF::PVTGeodetic>);
        }
    }
    QVERIFY(pvtGeodeticBlocks > 0);
}

void GeneratedDefinitionsTest::_sbfModeBits()
{
    for (unsigned mode = 0; mode <= UINT8_MAX; ++mode) {
        SBF::PVTGeodetic pvt{};
        pvt.mode = static_cast<uint8_t>(mode);
        // SBF Mode: bits 0-3 are the PVT mode type, bit 6 an automatic base position still being determined,
        // bit 7 a 2D solution.
        QCOMPARE(pvt.modeType(), static_cast<uint8_t>(mode & 0x0f));
        QCOMPARE(pvt.modeAutoSet(), (mode & 0x40) != 0);
        QCOMPARE(pvt.mode2D(), (mode & 0x80) != 0);
    }
}

UT_REGISTER_TEST(GeneratedDefinitionsTest, TestLabel::Unit)
