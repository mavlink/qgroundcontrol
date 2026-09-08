#include "PositionManagerTest.h"

#include <cstring>

#include <QtCore/QIODevice>
#include <QtCore/QRegularExpression>
#include <QtTest/QSignalSpy>

#include "PositionManager.h"

namespace {

// Fix at 53.361337, -6.50562 (RMC provides the date, GGA provides HDOP for accuracy)
constexpr const char* kNmeaSentences =
    "$GPRMC,092750.000,A,5321.6802,N,00630.3372,W,0.02,31.66,280511,,,A*43\r\n"
    "$GPGGA,092750.000,5321.6802,N,00630.3372,W,1,8,1.03,61.7,M,55.2,M,,*76\r\n";

constexpr double kExpectedLat = 53.361337;
constexpr double kExpectedLon = -6.50562;
constexpr double kCoordEpsilon = 0.0001;

/// Sequential QIODevice that hands queued NMEA bytes to QNmeaPositionInfoSource on demand.
class NmeaTestDevice : public QIODevice
{
public:
    NmeaTestDevice() { (void) open(QIODevice::ReadOnly); }

    bool isSequential() const override { return true; }
    qint64 bytesAvailable() const override { return _data.size() + QIODevice::bytesAvailable(); }
    bool canReadLine() const override { return _data.contains('\n') || QIODevice::canReadLine(); }

    void feed(const QByteArray &data)
    {
        _data.append(data);
        emit readyRead();
    }

protected:
    qint64 readData(char *data, qint64 maxSize) override
    {
        const qint64 count = qMin<qint64>(maxSize, _data.size());
        (void) memcpy(data, _data.constData(), count);
        _data.remove(0, count);
        return count;
    }

    qint64 writeData(const char *, qint64) override { return -1; }

private:
    QByteArray _data;
};

} // namespace

void PositionManagerTest::init()
{
    UnitTest::init();
    // Headless CI has no real position source, so the internal GPS fallback
    // times out waiting for updates. Expected and benign in this fixture.
    ignoreLogMessage("PositionManager.QGCPositionManager", QtWarningMsg,
                     QRegularExpression(QStringLiteral("UpdateTimeoutError")));
}

void PositionManagerTest::cleanup()
{
    // QGCPositionManager is an application-static singleton — always tear down the NMEA source
    // so a failed test can't leak state into the next one. The device must be deleted after the
    // source, since QNmeaPositionInfoSource holds a raw pointer to it.
    QGCPositionManager::instance()->resetNmeaSourceDevice();
    delete _nmeaDevice;
    _nmeaDevice = nullptr;

    UnitTest::cleanup();
}

void PositionManagerTest::_nmeaSourceProducesGcsPosition()
{
    QGCPositionManager *pm = QGCPositionManager::instance();
    auto *device = new NmeaTestDevice();
    _nmeaDevice = device;

    pm->setNmeaSourceDevice(device);
    device->feed(kNmeaSentences);

    QTRY_VERIFY_WITH_TIMEOUT(pm->gcsPosition().isValid(), TestTimeout::mediumMs());
    QVERIFY(qAbs(pm->gcsPosition().latitude() - kExpectedLat) < kCoordEpsilon);
    QVERIFY(qAbs(pm->gcsPosition().longitude() - kExpectedLon) < kCoordEpsilon);
    QVERIFY(pm->gcsPositionHorizontalAccuracy() < 100.);
}

void PositionManagerTest::_resetNmeaSourceTearsDownAndClearsState()
{
    QGCPositionManager *pm = QGCPositionManager::instance();
    auto *device = new NmeaTestDevice();
    _nmeaDevice = device;

    pm->setNmeaSourceDevice(device);
    device->feed(kNmeaSentences);
    QTRY_VERIFY_WITH_TIMEOUT(pm->gcsPosition().isValid(), TestTimeout::mediumMs());

    QSignalSpy positionInfoSpy(pm, &QGCPositionManager::positionInfoUpdated);
    QVERIFY(positionInfoSpy.isValid());

    pm->resetNmeaSourceDevice();

    // Stale GCS state must be cleared on teardown
    QVERIFY(!pm->gcsPosition().isValid());
    QVERIFY(qIsInf(pm->gcsPositionHorizontalAccuracy()));
    QVERIFY(!positionInfoSpy.isEmpty());

    // The NMEA source is gone: further data must not resurrect the position
    positionInfoSpy.clear();
    device->feed(kNmeaSentences);
    QVERIFY(!positionInfoSpy.wait(TestTimeout::shortMs()));
    QVERIFY(!pm->gcsPosition().isValid());

    // Second reset with no NMEA source is a no-op
    pm->resetNmeaSourceDevice();
}

UT_REGISTER_TEST(PositionManagerTest, TestLabel::Unit)
