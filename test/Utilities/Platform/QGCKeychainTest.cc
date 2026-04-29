#include "QGCKeychainTest.h"

#include <qtkeychain/keychain.h>

#include "QGCKeychain.h"
#include "UnitTest.h"

void QGCKeychainTest::_testWriteReadRoundTrip()
{
    const QString key = QStringLiteral("test/roundtrip");
    const QByteArray data = QByteArray::fromHex("deadbeefcafebabe0123456789abcdef");

    QVERIFY(QGCKeychain::writeBinary(key, data));

    const QByteArray result = QGCKeychain::readBinary(key);
    QCOMPARE(result, data);

    QGCKeychain::remove(key);
}

void QGCKeychainTest::_testOverwriteExistingKey()
{
    const QString key = QStringLiteral("test/overwrite");
    const QByteArray first = QByteArray(32, 'A');
    const QByteArray second = QByteArray(32, 'B');

    QVERIFY(QGCKeychain::writeBinary(key, first));
    QCOMPARE(QGCKeychain::readBinary(key), first);

    QVERIFY(QGCKeychain::writeBinary(key, second));
    QCOMPARE(QGCKeychain::readBinary(key), second);

    QGCKeychain::remove(key);
}

void QGCKeychainTest::_testReadNonexistentReturnsEmpty()
{
    const QByteArray result = QGCKeychain::readBinary(QStringLiteral("test/nonexistent_key_xyz"));
    QVERIFY(result.isEmpty());
}

void QGCKeychainTest::_testRemoveExistingKey()
{
    const QString key = QStringLiteral("test/remove");
    QVERIFY(QGCKeychain::writeBinary(key, QByteArray(16, '\x01')));
    QVERIFY(QGCKeychain::remove(key));

    const QByteArray result = QGCKeychain::readBinary(key);
    QVERIFY(result.isEmpty());
}

void QGCKeychainTest::_testRemoveNonexistentKey()
{
    QGCKeychain::remove(QStringLiteral("test/never_existed_xyz"));
}

void QGCKeychainTest::_testEmptyData()
{
    const QString key = QStringLiteral("test/empty");

    QVERIFY(QGCKeychain::writeBinary(key, QByteArray()));

    const QByteArray result = QGCKeychain::readBinary(key);
    QVERIFY(result.isEmpty());

    QGCKeychain::remove(key);
}

void QGCKeychainTest::_testLargeData()
{
    const QString key = QStringLiteral("test/large");
    QByteArray data(4096, '\x00');
    for (int i = 0; i < data.size(); ++i) {
        data[i] = static_cast<char>(i & 0xFF);
    }

    QVERIFY(QGCKeychain::writeBinary(key, data));
    QCOMPARE(QGCKeychain::readBinary(key), data);

    QGCKeychain::remove(key);
}

void QGCKeychainTest::_testMultipleKeysCoexist()
{
    const QString keyA = QStringLiteral("test/coexist/a");
    const QString keyB = QStringLiteral("test/coexist/b");
    const QByteArray dataA = QByteArray(16, '\xA1');
    const QByteArray dataB = QByteArray(24, '\xB2');

    QVERIFY(QGCKeychain::writeBinary(keyA, dataA));
    QVERIFY(QGCKeychain::writeBinary(keyB, dataB));

    QCOMPARE(QGCKeychain::readBinary(keyA), dataA);
    QCOMPARE(QGCKeychain::readBinary(keyB), dataB);

    QGCKeychain::remove(keyA);
    QVERIFY(QGCKeychain::readBinary(keyA).isEmpty());
    QCOMPARE(QGCKeychain::readBinary(keyB), dataB);

    QGCKeychain::remove(keyB);
}

void QGCKeychainTest::_testBinaryDataWithNulls()
{
    const QString key = QStringLiteral("test/binary_nulls");
    QByteArray data;
    data.reserve(256);
    for (int i = 0; i < 256; ++i) {
        data.append(static_cast<char>(i));  // every byte value, including 0x00
    }

    QVERIFY(QGCKeychain::writeBinary(key, data));
    const QByteArray result = QGCKeychain::readBinary(key);
    QCOMPARE(result.size(), data.size());
    QCOMPARE(result, data);

    QGCKeychain::remove(key);
}

void QGCKeychainTest::_testBackendProbeAndRoundTrip()
{
    const bool available = QKeychain::isAvailable();
    Q_UNUSED(available);

    const QString key = QStringLiteral("test/probe");
    const QByteArray data = QByteArray::fromHex("0123456789abcdef");

    QVERIFY(QGCKeychain::writeBinary(key, data));
    QCOMPARE(QGCKeychain::readBinary(key), data);
    QVERIFY(QGCKeychain::remove(key));
    QVERIFY(QGCKeychain::readBinary(key).isEmpty());
}

UT_REGISTER_TEST(QGCKeychainTest, TestLabel::Unit, TestLabel::Utilities)
