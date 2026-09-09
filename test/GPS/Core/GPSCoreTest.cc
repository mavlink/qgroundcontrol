#include <QtCore/QIODevice>
#include <QtPositioning/QGeoPositionInfoSource>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include <algorithm>

#include "GPSConnectionState.h"
#include "NMEADecoderSession.h"
#include "NMEAUtils.h"

namespace {
class ReceiverInput : public QIODevice
{
public:
    ReceiverInput() { open(ReadOnly); }

    bool isSequential() const override { return true; }

    qint64 bytesAvailable() const override { return QIODevice::bytesAvailable() + _bytes.size(); }

    void feed(const QByteArray& bytes)
    {
        _bytes.append(bytes);
        emit readyRead();
    }

protected:
    qint64 readData(char* data, qint64 maxSize) override
    {
        const qint64 count = std::min<qint64>(maxSize, _bytes.size());
        std::copy_n(_bytes.constData(), count, data);
        _bytes.remove(0, count);
        return count;
    }

    qint64 writeData(const char*, qint64) override { return -1; }

private:
    QByteArray _bytes;
};

const QByteArray FIX =
    "$GPRMC,092750.000,A,5321.6802,N,00630.3372,W,0.02,31.66,280511,,,A*43\r\n"
    "$GPGGA,092750.000,5321.6802,N,00630.3372,W,1,8,1.03,61.7,M,55.2,M,,*76\r\n";
}  // namespace

class GPSCoreTest : public QObject
{
    Q_OBJECT

private slots:
    void _decoderSessionRestart();
    void _pauseDuringConfiguration();
};

void GPSCoreTest::_decoderSessionRestart()
{
    ReceiverInput input;
    NMEADecoderSession session;
    QVERIFY(session.start(&input));
    session.positionSource()->startUpdates();
    // Mixed binary traffic and split sentences must still produce a usable fix.
    input.feed(QByteArray::fromHex("b5620000") + FIX.first(19));
    QVERIFY(!session.health()->usable());
    input.feed(FIX.mid(19));
    // This executable has no application test harness; allow Qt's realtime NMEA timer to fire.
    QTRY_VERIFY_WITH_TIMEOUT(session.health()->usable(), 5000);
    QVERIFY(session.health()->coordinate().isValid());

    session.stop();
    QVERIFY(input.isOpen());
    QCOMPARE(session.health()->state(), GPSSourceHealth::NoData);
    QVERIFY(session.start(&input));
    session.positionSource()->startUpdates();
    input.feed(NMEAUtils::repairChecksum("$GPRMC,092751.000,A,5321.6802,N,00630.3372,W,2.0,31.66,280511,,,A"));
    QSignalSpy fixes(session.positionSource(), &QGeoPositionInfoSource::positionUpdated);
    QTRY_VERIFY_WITH_TIMEOUT(!fixes.isEmpty(), 5000);
    const auto position = fixes.last().first().value<QGeoPositionInfo>();
    QVERIFY(!position.hasAttribute(QGeoPositionInfo::HorizontalAccuracy));
}

void GPSCoreTest::_pauseDuringConfiguration()
{
    GPSConnectionState connection;
    connect(&connection, &GPSConnectionState::changed, this, [&]() {
        if (connection.state() == GPSConnectionState::Configuring) {
            connection.pause();
            connection.stopping();
        }
    });
    connection.requestConnect();
    QVERIFY(connection.beginAttempt());
    connection.configuring();
    connection.ready();
    connection.failed();
    QVERIFY(connection.paused());
    QCOMPARE(connection.state(), GPSConnectionState::Stopping);
    connection.stopped();
    QVERIFY(!connection.updateIntent(true));
    QVERIFY(!connection.beginAttempt());
    connection.requestConnect();
    QVERIFY(connection.beginAttempt());
}

QTEST_GUILESS_MAIN(GPSCoreTest)

#include "GPSCoreTest.moc"
