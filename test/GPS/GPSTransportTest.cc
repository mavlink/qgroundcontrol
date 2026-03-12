#include "GPSTransportTest.h"
#include "GPSTransport.h"
#include "UnitTest.h"

#include <QtCore/QByteArray>

class MockGPSTransport : public GPSTransport
{
    Q_OBJECT
public:
    using GPSTransport::GPSTransport;

    bool open() override { _isOpen = true; return true; }
    void close() override { _isOpen = false; }
    bool isOpen() const override { return _isOpen; }

    qint64 read(char *data, qint64 maxSize) override
    {
        const qint64 n = qMin(maxSize, static_cast<qint64>(_readBuffer.size()));
        memcpy(data, _readBuffer.constData(), n);
        _readBuffer.remove(0, n);
        return n;
    }

    qint64 write(const char *data, qint64 size) override
    {
        _writeBuffer.append(data, size);
        return size;
    }

    bool waitForReadyRead(int) override { return !_readBuffer.isEmpty(); }
    bool waitForBytesWritten(int) override { return true; }
    qint64 bytesAvailable() const override { return _readBuffer.size(); }
    bool setBaudRate(qint32 baud) override { _baudRate = baud; return true; }

    QByteArray _readBuffer;
    QByteArray _writeBuffer;
    qint32 _baudRate = 0;

private:
    bool _isOpen = false;
};

void GPSTransportTest::testMockTransport()
{
    MockGPSTransport transport;

    QCOMPARE(transport.isOpen(), false);
    QVERIFY(transport.open());
    QCOMPARE(transport.isOpen(), true);

    const char testData[] = "hello";
    QCOMPARE(transport.write(testData, 5), static_cast<qint64>(5));
    QCOMPARE(transport._writeBuffer, QByteArrayLiteral("hello"));

    transport._readBuffer = QByteArrayLiteral("world");
    QCOMPARE(transport.bytesAvailable(), static_cast<qint64>(5));

    char buf[16] = {};
    QCOMPARE(transport.read(buf, 16), static_cast<qint64>(5));
    QCOMPARE(QByteArray(buf, 5), QByteArrayLiteral("world"));

    QVERIFY(transport.setBaudRate(115200));
    QCOMPARE(transport._baudRate, 115200);

    transport.close();
    QCOMPARE(transport.isOpen(), false);
}

void GPSTransportTest::testOpenCloseLifecycle()
{
    MockGPSTransport transport;

    // Double open
    QVERIFY(transport.open());
    QVERIFY(transport.open());
    QVERIFY(transport.isOpen());

    // Double close
    transport.close();
    QVERIFY(!transport.isOpen());
    transport.close();
    QVERIFY(!transport.isOpen());
}

void GPSTransportTest::testReadEmptyBuffer()
{
    MockGPSTransport transport;
    transport.open();

    char buf[16] = {};
    QCOMPARE(transport.read(buf, 16), static_cast<qint64>(0));
    QCOMPARE(transport.bytesAvailable(), static_cast<qint64>(0));
    QVERIFY(!transport.waitForReadyRead(0));
}

void GPSTransportTest::testWriteWhenClosed()
{
    MockGPSTransport transport;
    // Not opened — mock still writes since it's an in-memory mock
    QCOMPARE(transport.write("test", 4), static_cast<qint64>(4));
}

void GPSTransportTest::testSetBaudRate()
{
    MockGPSTransport transport;
    QVERIFY(transport.setBaudRate(9600));
    QCOMPARE(transport._baudRate, 9600);
    QVERIFY(transport.setBaudRate(115200));
    QCOMPARE(transport._baudRate, 115200);
    QVERIFY(transport.setBaudRate(921600));
    QCOMPARE(transport._baudRate, 921600);
}

void GPSTransportTest::testBytesAvailableAccuracy()
{
    MockGPSTransport transport;
    transport.open();

    QCOMPARE(transport.bytesAvailable(), static_cast<qint64>(0));

    transport._readBuffer = QByteArrayLiteral("12345");
    QCOMPARE(transport.bytesAvailable(), static_cast<qint64>(5));

    char buf[3];
    transport.read(buf, 3);
    QCOMPARE(transport.bytesAvailable(), static_cast<qint64>(2));
}

void GPSTransportTest::testPartialRead()
{
    MockGPSTransport transport;
    transport.open();

    transport._readBuffer = QByteArrayLiteral("ABCDEFGH");

    char buf[4] = {};
    QCOMPARE(transport.read(buf, 4), static_cast<qint64>(4));
    QCOMPARE(QByteArray(buf, 4), QByteArrayLiteral("ABCD"));

    QCOMPARE(transport.read(buf, 4), static_cast<qint64>(4));
    QCOMPARE(QByteArray(buf, 4), QByteArrayLiteral("EFGH"));

    QCOMPARE(transport.read(buf, 4), static_cast<qint64>(0));
}

UT_REGISTER_TEST(GPSTransportTest, TestLabel::Unit)

#include "GPSTransportTest.moc"
