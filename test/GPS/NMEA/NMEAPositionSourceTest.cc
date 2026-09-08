#include "NMEAPositionSourceTest.h"

#include <QtCore/QEvent>
#include <QtCore/QIODevice>
#include <QtPositioning/QNmeaPositionInfoSource>
#include <QtTest/QSignalSpy>

#include <cstring>

#include "NMEAPositionSource.h"
#include "NMEAUtils.h"

namespace {
const QByteArray kFix =
    "$GPRMC,092750.000,A,5321.6802,N,00630.3372,W,0.02,31.66,280511,,,A*43\r\n"
    "$GPGGA,092750.000,5321.6802,N,00630.3372,W,1,8,1.03,61.7,M,55.2,M,,*76\r\n";

class NMEAInput : public QIODevice
{
public:
    NMEAInput() { open(QIODevice::ReadOnly); }

    bool isSequential() const override { return true; }

    qint64 bytesAvailable() const override { return _data.size() + QIODevice::bytesAvailable(); }

    bool canReadLine() const override { return _data.contains('\n') || QIODevice::canReadLine(); }

    void feed(const QByteArray& data)
    {
        _data.append(data);
        emit readyRead();
    }

protected:
    qint64 readData(char* data, qint64 maxSize) override
    {
        const qint64 count = qMin<qint64>(maxSize, _data.size());
        memcpy(data, _data.constData(), count);
        _data.remove(0, count);
        return count;
    }

    qint64 writeData(const char*, qint64) override { return -1; }

private:
    QByteArray _data;
};
}  // namespace

void NMEAPositionSourceTest::_restartClearsParserState()
{
    NMEAInput device;
    NMEAPositionSource source(&device);
    QSignalSpy updates(&source, &QGeoPositionInfoSource::positionUpdated);
    source.startUpdates();
    device.feed(kFix);
    QTRY_VERIFY_WITH_TIMEOUT(!updates.isEmpty(), TestTimeout::mediumMs());
    QVERIFY(source.lastKnownPosition().isValid());
    source.stopUpdates();
    device.feed(kFix);
    source.startUpdates();
    QVERIFY(!source.lastKnownPosition().isValid());
    updates.clear();
    // An RMC without HDOP must not inherit the accuracy cached before the restart.
    device.feed(NMEAUtils::repairChecksum("$GPRMC,092751.000,A,5321.6802,N,00630.3372,W,2.0,31.66,280511,,,A"));
    QTRY_VERIFY_WITH_TIMEOUT(!updates.isEmpty(), TestTimeout::mediumMs());
    const auto fix = updates.last().first().value<QGeoPositionInfo>();
    QVERIFY(!fix.hasAttribute(QGeoPositionInfo::HorizontalAccuracy));
    QVERIFY(device.isOpen());
}

void NMEAPositionSourceTest::_pendingRequestSurvivesStopAndStart()
{
    NMEAInput device;
    NMEAPositionSource source(&device);
    QSignalSpy updates(&source, &QGeoPositionInfoSource::positionUpdated);
    source.requestUpdate(1000);
    source.stopUpdates();
    source.startUpdates();
    source.stopUpdates();
    device.feed(kFix);
    QTRY_COMPARE_WITH_TIMEOUT(updates.size(), 1, TestTimeout::mediumMs());
    QVERIFY(source.lastKnownPosition().isValid());
    QCOMPARE(source.error(), QGeoPositionInfoSource::NoError);
}

void NMEAPositionSourceTest::_requestTimeoutAndRecovery()
{
    NMEAInput device;
    NMEAPositionSource source(&device);
    QSignalSpy errors(&source, &QGeoPositionInfoSource::errorOccurred);
    source.requestUpdate(50);
    source.requestUpdate(1000);
    source.requestUpdate(-1);
    QTRY_COMPARE_WITH_TIMEOUT(errors.size(), 1, TestTimeout::mediumMs());
    QCOMPARE(source.error(), QGeoPositionInfoSource::UpdateTimeoutError);
    source.requestUpdate(1000);
    QSignalSpy updates(&source, &QGeoPositionInfoSource::positionUpdated);
    device.feed(kFix);
    QTRY_COMPARE_WITH_TIMEOUT(updates.size(), 1, TestTimeout::mediumMs());
    QCOMPARE(source.error(), QGeoPositionInfoSource::NoError);
}

void NMEAPositionSourceTest::_queuedUpdateCannotSurviveRestart()
{
    NMEAInput device;
    NMEAPositionSource source(&device);
    QSignalSpy updates(&source, &QGeoPositionInfoSource::positionUpdated);
    source.startUpdates();
    QSignalSpy decoded(source._decoder.get(), &QGeoPositionInfoSource::positionUpdated);
    device.feed(kFix);
    if (decoded.isEmpty()) {
        QVERIFY(decoded.wait(TestTimeout::mediumMs()));
    }
    QVERIFY(source.lastKnownPosition().isValid());
    QVERIFY(updates.isEmpty());
    source.stopUpdates();
    source.startUpdates();
    QCoreApplication::sendPostedEvents(&source, QEvent::MetaCall);
    QVERIFY(updates.isEmpty());
    QVERIFY(!source.lastKnownPosition().isValid());
    device.feed(kFix);
    QTRY_VERIFY_WITH_TIMEOUT(!updates.isEmpty(), TestTimeout::mediumMs());
}

UT_REGISTER_TEST(NMEAPositionSourceTest, TestLabel::Unit)
