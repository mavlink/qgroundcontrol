#include "LogCompressionTest.h"
#include "LogCompression.h"

#include <QtTest/QTest>

UT_REGISTER_TEST(LogCompressionTest, TestLabel::Unit, TestLabel::Utilities)

void LogCompressionTest::_testCompressDecompress()
{
    // Test with compressible data
    const QByteArray original = QByteArray("Hello World! ").repeated(100);

    const QByteArray compressed = LogCompression::compress(original);
    QVERIFY(!compressed.isEmpty());
    QVERIFY(compressed.size() < original.size());  // Should be smaller

    const QByteArray decompressed = LogCompression::decompress(compressed);
    QCOMPARE(decompressed, original);
}

void LogCompressionTest::_testCompressLevel()
{
    const QByteArray original = QByteArray("Test data for compression levels ").repeated(50);

    // Fast compression
    const QByteArray fast = LogCompression::compress(original, LogCompression::Level::Fast);
    QVERIFY(!fast.isEmpty());

    // Best compression
    const QByteArray best = LogCompression::compress(original, LogCompression::Level::Best);
    QVERIFY(!best.isEmpty());

    // Best should be smaller or equal to fast
    QVERIFY(best.size() <= fast.size());

    // Both should decompress to original
    QCOMPARE(LogCompression::decompress(fast), original);
    QCOMPARE(LogCompression::decompress(best), original);
}

void LogCompressionTest::_testMinSize()
{
    const QByteArray smallData = "Small";
    const QByteArray largeData = QByteArray("Large data ").repeated(50);

    // Small data should not be compressed (minSize = 256)
    const QByteArray smallResult = LogCompression::compress(smallData, LogCompression::Level::Default, 256);
    QVERIFY(!LogCompression::isCompressed(smallResult));
    QCOMPARE(LogCompression::decompress(smallResult), smallData);

    // Large data should be compressed
    const QByteArray largeResult = LogCompression::compress(largeData, LogCompression::Level::Default, 256);
    QVERIFY(LogCompression::isCompressed(largeResult));
    QCOMPARE(LogCompression::decompress(largeResult), largeData);

    // With minSize = 0, even small data gets header (but may not actually compress)
    const QByteArray smallWithZeroMin = LogCompression::compress(smallData, LogCompression::Level::Default, 0);
    QCOMPARE(LogCompression::decompress(smallWithZeroMin), smallData);
}

void LogCompressionTest::_testIsCompressed()
{
    const QByteArray data = QByteArray("Compressible data ").repeated(50);

    // Compressed data
    const QByteArray compressed = LogCompression::compress(data, LogCompression::Level::Default, 0);
    QVERIFY(LogCompression::isCompressed(compressed));

    // Uncompressed data (Level::None)
    const QByteArray uncompressed = LogCompression::compress(data, LogCompression::Level::None);
    QVERIFY(!LogCompression::isCompressed(uncompressed));

    // Empty data
    QVERIFY(!LogCompression::isCompressed(QByteArray()));
}

void LogCompressionTest::_testEmptyData()
{
    const QByteArray empty;

    // Compress empty data
    const QByteArray compressed = LogCompression::compress(empty);
    QVERIFY(!compressed.isEmpty());  // Should have header byte
    QCOMPARE(compressed.size(), 1);  // Just the header

    // Decompress back to empty
    const QByteArray decompressed = LogCompression::decompress(compressed);
    QVERIFY(decompressed.isEmpty());

    // Decompress truly empty data
    QVERIFY(LogCompression::decompress(QByteArray()).isEmpty());
}

void LogCompressionTest::_testInvalidData()
{
    // Invalid header byte
    QByteArray invalidHeader;
    invalidHeader.append(static_cast<char>(0xFF));
    invalidHeader.append("some data");
    QVERIFY(LogCompression::decompress(invalidHeader).isEmpty());

    // Corrupted compressed data
    QByteArray corrupted;
    corrupted.append(static_cast<char>(LogCompression::HeaderCompressed));
    corrupted.append("not valid zlib data");
    QVERIFY(LogCompression::decompress(corrupted).isEmpty());
}

void LogCompressionTest::_testCompressionRatio()
{
    // Highly compressible data
    const QByteArray repetitive = QByteArray(1000, 'A');
    LogCompression::compress(repetitive, LogCompression::Level::Default, 0);
    const int ratio = LogCompression::lastCompressionRatio();
    QVERIFY(ratio < 50);  // Should compress to less than 50% of original

    // Incompressible data - use pseudo-random values with good distribution
    // Linear congruential generator: produces non-repeating pattern that doesn't compress well
    QByteArray random;
    random.resize(1000);
    quint32 seed = 12345;
    for (int i = 0; i < 1000; ++i) {
        seed = seed * 1103515245 + 12345;  // LCG formula
        random[i] = static_cast<char>((seed >> 16) & 0xFF);
    }
    LogCompression::compress(random, LogCompression::Level::Default, 0);
    // Ratio should be high (compression didn't help much, or data sent uncompressed)
    QVERIFY(LogCompression::lastCompressionRatio() >= 50);
}

void LogCompressionTest::_testDecompressionBombProtection()
{
    // Compress data that's large enough to trigger the bomb check
    const QByteArray original = QByteArray(100000, 'A');  // 100 KB of repetitive data
    const QByteArray compressed = LogCompression::compress(original, LogCompression::Level::Default, 0);
    QVERIFY(LogCompression::isCompressed(compressed));

    // Decompress with default limit (64 MB) should succeed
    const QByteArray result = LogCompression::decompress(compressed);
    QCOMPARE(result, original);

    // Decompress with a limit smaller than the declared size should be rejected
    const QByteArray rejected = LogCompression::decompress(compressed, 1024);  // 1 KB limit
    QVERIFY(rejected.isEmpty());

    // Decompress with limit of 0 (no limit) should succeed
    const QByteArray unlimited = LogCompression::decompress(compressed, 0);
    QCOMPARE(unlimited, original);

    // Decompress with limit exactly equal to original size should succeed
    const QByteArray exact = LogCompression::decompress(compressed, original.size());
    QCOMPARE(exact, original);
}
