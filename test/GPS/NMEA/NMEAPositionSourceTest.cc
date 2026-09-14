#include "NMEAPositionSourceTest.h"

#include <QtCore/QBuffer>
#include <QtCore/QEvent>
#include <QtCore/QIODevice>
#include <QtCore/QTimeZone>
#include <QtPositioning/QNmeaPositionInfoSource>
#include <QtTest/QSignalSpy>

#include "GPSSourceHealth.h"
#include "MonotonicClock.h"
#include "NMEAPositionSource.h"
#include "NMEAUtils.h"
#include "QtRuntimeScheduler.h"
#include "SequentialTestDevice.h"

namespace {
const QByteArray kFix =
    "$GPRMC,092750.000,A,5321.6802,N,00630.3372,W,0.02,31.66,280511,,,A*43\r\n"
    "$GPGGA,092750.000,5321.6802,N,00630.3372,W,1,8,1.03,61.7,M,55.2,M,,*76\r\n";
}  // namespace

void NMEAPositionSourceTest::_dateOrdering_data()
{
    QTest::addColumn<bool>("dateFirst");
    QTest::addColumn<bool>("useZda");
    QTest::newRow("rmc-gga") << true << false;
    QTest::newRow("gga-rmc") << false << false;
    QTest::newRow("zda-gga") << true << true;
    QTest::newRow("gga-zda") << false << true;
}

void NMEAPositionSourceTest::_dateOrdering()
{
    QFETCH(bool, dateFirst);
    QFETCH(bool, useZda);
    SequentialTestDevice device;
    SequentialTestDevice qtDevice;
    NMEAPositionSource source(&device);
    QNmeaPositionInfoSource qtSource(QNmeaPositionInfoSource::RealTimeMode);
    qtSource.setDevice(&qtDevice);
    QSignalSpy updates(&source, &QGeoPositionInfoSource::positionUpdated);
    QSignalSpy qtUpdates(&qtSource, &QGeoPositionInfoSource::positionUpdated);
    source.startUpdates();
    qtSource.startUpdates();
    const auto fix = [&](const QDateTime& timestamp) {
        const auto time = timestamp.time().toString(u"hhmmss.zzz").toLatin1();
        const auto date = timestamp.date().toString(useZda ? u"dd,MM,yyyy" : u"ddMMyy").toLatin1();
        const auto dated = NMEAUtils::repairChecksum(
            useZda ? "$GPZDA," + time + ',' + date + ",00,00"
                   : "$GPRMC," + time + ",A,5321.6802,N,00630.3372,W,0.02,31.66," + date + ",,,A");
        const auto gga =
            NMEAUtils::repairChecksum("$GPGGA," + time + ",5321.6802,N,00630.3372,W,1,8,1.03,61.7,M,55.2,M,,");
        return dateFirst ? dated + gga : gga + dated;
    };
    const QDateTime midnight(QDate(2012, 1, 1), QTime(0, 0), QTimeZone::UTC);
    for (const auto& timestamp : {midnight.addMSecs(-1), midnight.addMSecs(1)}) {
        updates.clear();
        qtUpdates.clear();
        const auto sentences = fix(timestamp);
        device.feed(sentences);
        qtDevice.feed(sentences);
        if (useZda && !dateFirst && timestamp > midnight) {
            // Qt discards the undated GGA across midnight before ZDA establishes the new date.
            // A repeated fix must recover without restarting either source.
            const auto repeatedGga = sentences.first(sentences.indexOf('\n') + 1);
            device.feed(repeatedGga);
            qtDevice.feed(repeatedGga);
        }
        QTRY_VERIFY_WITH_TIMEOUT(!updates.isEmpty() && !qtUpdates.isEmpty(), TestTimeout::shortMs());
        const auto observation = source.lastObservation();
        const auto reference = qtUpdates.last().first().value<QGeoPositionInfo>();
        QCOMPARE(observation.position.timestamp(), timestamp);
        QCOMPARE(observation.position.timestamp(), reference.timestamp());
        QCOMPARE(observation.position.coordinate(), reference.coordinate());
        QCOMPARE(observation.fixQuality, GPSObservation::FixQuality::Unknown);
        QCOMPARE(observation.satellitesUsed, std::optional<unsigned>(8));
    }
}

void NMEAPositionSourceTest::_restartClearsParserState()
{
    SequentialTestDevice device;
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
    SequentialTestDevice device;
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
    SequentialTestDevice device;
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
    SequentialTestDevice device;
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

void NMEAPositionSourceTest::_fixDimensionOrdering_data()
{
    QTest::addColumn<QByteArray>("order");
    QTest::addColumn<int>("dimension");
    QTest::addColumn<int>("quality");
    QTest::addColumn<GPSObservation::FixQuality>("expected");
    using Quality = GPSObservation::FixQuality;
    for (const auto& order : {QByteArray("SG"), QByteArray("GS"), QByteArray("GSG")}) {
        QTest::newRow((order + "-2d").constData()) << order << 2 << 1 << Quality::Fix2D;
        QTest::newRow((order + "-3d").constData()) << order << 3 << 1 << Quality::Fix3D;
        QTest::newRow((order + "-rtk").constData()) << order << 3 << 4 << Quality::RTKFixed;
    }
    QTest::newRow("no-dimension") << QByteArray("G") << 0 << 1 << Quality::Unknown;
}

void NMEAPositionSourceTest::_fixDimensionOrdering()
{
    QFETCH(QByteArray, order);
    QFETCH(int, dimension);
    QFETCH(int, quality);
    QFETCH(GPSObservation::FixQuality, expected);
    SequentialTestDevice device;
    NMEAPositionSource source(&device);
    QSignalSpy updates(&source, &QGeoPositionInfoSource::positionUpdated);
    source.startUpdates();
    auto input = NMEAUtils::repairChecksum("$GPRMC,092750.000,A,5321.6802,N,00630.3372,W,0.02,31.66,280511,,,A");
    const auto gga = NMEAUtils::repairChecksum("$GPGGA,092750.000,5321.6802,N,00630.3372,W," +
                                               QByteArray::number(quality) + ",8,1.03,61.7,M,55.2,M,,");
    const auto gsa =
        NMEAUtils::repairChecksum("$GPGSA,A," + QByteArray::number(dimension) + ",02,,,,,,,,,,,,1.0,1.03,0.6");
    for (const auto sentence : order) {
        input += sentence == 'G' ? gga : gsa;
    }
    device.feed(input);
    QTRY_COMPARE_WITH_TIMEOUT(updates.size(), 1, TestTimeout::shortMs());
    QCOMPARE(source.lastObservation().fixQuality, expected);
    QVERIFY(source.lastObservation().usable());
    updates.clear();
    device.feed(NMEAUtils::repairChecksum("$GPRMC,092751.000,A,5321.6802,N,00630.3372,W,0.02,31.66,280511,,,A") +
                NMEAUtils::repairChecksum("$GPGGA,092751.000,5321.6802,N,00630.3372,W,1,8,1.03,61.7,M,55.2,M,,"));
    QTRY_COMPARE_WITH_TIMEOUT(updates.size(), 1, TestTimeout::shortMs());
    QCOMPARE(source.lastObservation().fixQuality, GPSObservation::FixQuality::Unknown);
}

void NMEAPositionSourceTest::_bufferedRequest_data()
{
    QTest::addColumn<bool>("sequential");
    QTest::addColumn<bool>("retry");
    QTest::newRow("buffer") << false << false;
    QTest::newRow("sequential") << true << false;
    QTest::newRow("buffer-after-timeout") << false << true;
    QTest::newRow("sequential-after-timeout") << true << true;
}

void NMEAPositionSourceTest::_bufferedRequest()
{
    QFETCH(bool, sequential);
    QFETCH(bool, retry);
    SequentialTestDevice stream;
    QBuffer buffer;
    QVERIFY(buffer.open(QIODevice::ReadOnly));
    QIODevice* device = sequential ? static_cast<QIODevice*>(&stream) : &buffer;
    NMEAPositionSource source(device);
    QSignalSpy updates(&source, &QGeoPositionInfoSource::positionUpdated);
    QSignalSpy errors(&source, &QGeoPositionInfoSource::errorOccurred);
    if (retry) {
        source.requestUpdate(50);
        QTRY_COMPARE_WITH_TIMEOUT(errors.size(), 1, TestTimeout::shortMs());
    }
    if (sequential) {
        stream.feed(kFix, false);
    } else {
        buffer.buffer().append(kFix);
    }
    source.requestUpdate(TestTimeout::mediumMs());
    QCOMPARE(device->bytesAvailable(), kFix.size());
    emit device->readyRead();
    QTRY_COMPARE_WITH_TIMEOUT(updates.size(), 1, TestTimeout::shortMs());
    QCOMPARE(source.lastObservation().position.timestamp().time(), QTime(9, 27, 50));
    QCOMPARE(source.error(), QGeoPositionInfoSource::NoError);
    QCOMPARE(errors.size(), retry ? 1 : 0);
    QVERIFY(!source._requestTask.active());
}

void NMEAPositionSourceTest::_fixMetadata_data()
{
    QTest::addColumn<int>("quality");
    QTest::addColumn<GPSObservation::FixQuality>("expected");
    QTest::newRow("autonomous") << 1 << GPSObservation::FixQuality::Fix3D;
    QTest::newRow("differential") << 2 << GPSObservation::FixQuality::Differential;
    QTest::newRow("rtk-fixed") << 4 << GPSObservation::FixQuality::RTKFixed;
    QTest::newRow("rtk-float") << 5 << GPSObservation::FixQuality::RTKFloat;
}

void NMEAPositionSourceTest::_fixMetadata()
{
    QFETCH(int, quality);
    QFETCH(GPSObservation::FixQuality, expected);
    SequentialTestDevice device;
    NMEAPositionSource source(&device);
    QSignalSpy updates(&source, &QGeoPositionInfoSource::positionUpdated);
    source.startUpdates();
    device.receivedAtUs = MonotonicClock::nowUs() - 2000000;
    const QByteArray rmc =
        NMEAUtils::repairChecksum("$GPRMC,092750.000,A,5321.6802,N,00630.3372,W,0.02,31.66,280511,,,A");
    const QByteArray gga = NMEAUtils::repairChecksum("$GPGGA,092750.000,5321.6802,N,00630.3372,W," +
                                                     QByteArray::number(quality) + ",8,1.03,61.7,M,55.2,M,,");
    device.feed(rmc + gga + NMEAUtils::repairChecksum("$GPGSA,A,3,02,,,,,,,,,,,,1.0,1.03,0.6"));
    QTRY_VERIFY_WITH_TIMEOUT(!updates.isEmpty(), TestTimeout::mediumMs());
    const auto observation = source.lastObservation();
    QCOMPARE(observation.fixQuality, expected);
    QCOMPARE(observation.satellitesUsed, std::optional<int>(8));
    QCOMPARE(observation.horizontalDop, std::optional<double>(1.03));
    QCOMPARE(observation.verticalDop, std::optional<double>(0.6));
    QCOMPARE(observation.altitudeDatum, GPSObservation::AltitudeDatum::MeanSeaLevel);
    QVERIFY(observation.altitudeEllipsoidMeters.has_value());
    QVERIFY(qAbs(*observation.altitudeEllipsoidMeters - 116.9) < 0.00001);
    QCOMPARE(observation.monotonicTimestampUs, device.receivedAtUs);
}

void NMEAPositionSourceTest::_metadataDoesNotCrossEpochs()
{
    SequentialTestDevice device;
    NMEAPositionSource source(&device);
    QSignalSpy updates(&source, &QGeoPositionInfoSource::positionUpdated);
    source.startUpdates();
    device.feed(kFix);
    QTRY_VERIFY_WITH_TIMEOUT(!updates.isEmpty(), TestTimeout::mediumMs());
    QVERIFY(source.lastObservation().satellitesUsed.has_value());
    updates.clear();
    device.feed(NMEAUtils::repairChecksum("$GPRMC,092751.000,A,5321.6802,N,00630.3372,W,2.0,31.66,280511,,,A"));
    QTRY_VERIFY_WITH_TIMEOUT(!updates.isEmpty(), TestTimeout::mediumMs());
    const auto observation = source.lastObservation();
    QCOMPARE(observation.fixQuality, GPSObservation::FixQuality::Unknown);
    QVERIFY(!observation.satellitesUsed);
    QVERIFY(!observation.horizontalDop);
    QVERIFY(!observation.position.hasAttribute(QGeoPositionInfo::HorizontalAccuracy));
    QVERIFY(!observation.verticalDop);
    QVERIFY(!observation.altitudeEllipsoidMeters);
    QCOMPARE(observation.altitudeDatum, GPSObservation::AltitudeDatum::Unknown);
}

void NMEAPositionSourceTest::_gstAccuracy_data()
{
    QTest::addColumn<bool>("beforeFix");
    QTest::addColumn<bool>("validChecksum");
    QTest::addColumn<QByteArray>("time");
    QTest::newRow("before-fix") << true << true << QByteArray("235959.000");
    QTest::newRow("after-fix") << false << true << QByteArray("235959.000");
    QTest::newRow("bad-checksum") << false << false << QByteArray("235959.000");
    QTest::newRow("fraction-before-fix") << true << true << QByteArray("000001.001");
    QTest::newRow("fraction-after-fix") << false << true << QByteArray("000001.001");
}

void NMEAPositionSourceTest::_gstAccuracy()
{
    QFETCH(bool, beforeFix);
    QFETCH(bool, validChecksum);
    QFETCH(QByteArray, time);
    SequentialTestDevice device;
    NMEAPositionSource source(&device);
    QSignalSpy updates(&source, &QGeoPositionInfoSource::positionUpdated);
    source.startUpdates();
    auto gst = NMEAUtils::repairChecksum("$GPGST," + time + ",1,1,1,0,3,4,6");
    if (!validChecksum) {
        gst[gst.indexOf('*') + 1] = 'X';
    }
    const auto fix = NMEAUtils::repairChecksum("$GPRMC," + time + ",A,5321.6802,N,00630.3372,W,0.02,31.66,280511,,,A") +
                     NMEAUtils::repairChecksum("$GPGGA," + time + ",5321.6802,N,00630.3372,W,1,8,1.03,61.7,M,55.2,M,,");
    device.feed((beforeFix ? gst + fix : fix + gst) +
                NMEAUtils::repairChecksum("$GPGSA,A,3,02,,,,,,,,,,,,1.0,1.03,0.6"));
    QTRY_VERIFY_WITH_TIMEOUT(!updates.isEmpty(), TestTimeout::mediumMs());
    const auto observation = source.lastObservation();
    QCOMPARE(observation.position.timestamp().time(), QTime::fromString(QString::fromLatin1(time), u"hhmmss.z"));
    if (validChecksum) {
        QCOMPARE(observation.position.attribute(QGeoPositionInfo::HorizontalAccuracy), 5.0);
        QCOMPARE(observation.position.attribute(QGeoPositionInfo::VerticalAccuracy), 6.0);
        QVERIFY(observation.accuracyTimestampUs != 0);
    } else {
        QVERIFY(observation.position.attribute(QGeoPositionInfo::HorizontalAccuracy) != 5.0);
        QCOMPARE(observation.accuracyTimestampUs, 0U);
    }
    updates.clear();
    const auto nextEpoch = observation.position.timestamp().addSecs(1);
    device.feed(NMEAUtils::repairChecksum("$GPRMC," + nextEpoch.time().toString(u"hhmmss.zzz").toLatin1() +
                                          ",A,5321.6802,N,00630.3372,W,0.02,31.66," +
                                          nextEpoch.date().toString(u"ddMMyy").toLatin1() + ",,,A"));
    QTRY_VERIFY_WITH_TIMEOUT(!updates.isEmpty(), TestTimeout::mediumMs());
    QVERIFY(!source.lastObservation().position.hasAttribute(QGeoPositionInfo::HorizontalAccuracy));
    QCOMPARE(source.lastObservation().accuracyTimestampUs, 0U);
    QCOMPARE(source.lastObservation().position.timestamp(), nextEpoch);
}

void NMEAPositionSourceTest::_fixLoss_data()
{
    QTest::addColumn<QByteArray>("loss");
    QTest::addColumn<bool>("pending");
    const QList<QPair<QByteArray, QByteArray>> cases = {
        {"gga", "$GPGGA,092751.000,5321.6802,N,00630.3372,W,0,0,99.9,61.7,M,55.2,M,,"},
        {"gga-empty", "$GPGGA,092751.000,,,,,0,0,,,,,,,"},
        {"rmc", "$GPRMC,092751.000,V,,,,,,,280511,,,N"},
        {"gsa", "$GPGSA,A,1,,,,,,,,,,,,,99.9,99.9,99.9"},
    };
    for (const auto& [name, loss] : cases) {
        QTest::newRow(name.constData()) << loss << false;
        QTest::newRow((name + "-pending").constData()) << loss << true;
    }
}

void NMEAPositionSourceTest::_fixLoss()
{
    QFETCH(QByteArray, loss);
    QFETCH(bool, pending);
    SequentialTestDevice device;
    NMEAPositionSource source(&device);
    GPSSourceHealth health;
    source.setUpdateInterval(pending ? 1000 : 0);
    connect(&source, &NMEAPositionSource::observationReceived, &health, &GPSSourceHealth::updateObservation);
    QSignalSpy updates(&source, &QGeoPositionInfoSource::positionUpdated);
    QSignalSpy observations(&source, &NMEAPositionSource::observationReceived);
    source.startUpdates();
    QSignalSpy decoded(source._decoder.get(), &QGeoPositionInfoSource::positionUpdated);
    device.feed(kFix);
    QTRY_VERIFY_WITH_TIMEOUT(!decoded.isEmpty(), TestTimeout::shortMs());
    if (pending) {
        QVERIFY(updates.isEmpty());
        QVERIFY(source._publicationTask.active());
    } else {
        QTRY_VERIFY_WITH_TIMEOUT(health.usable(), TestTimeout::shortMs());
    }
    const auto previousUpdates = updates.size();
    device.feed(NMEAUtils::repairChecksum(loss));
    QTRY_COMPARE_WITH_TIMEOUT(health.state(), GPSSourceHealth::State::Invalid, TestTimeout::shortMs());
    QCOMPARE(source.lastObservation().receiverFixValid, std::optional<bool>(false));
    QVERIFY(!source._publicationTask.active());
    QVERIFY(!source._pendingObservation);
    source.setUpdateInterval(0);
    QCOMPARE(updates.size(), previousUpdates);
    const auto recovery =
        NMEAUtils::repairChecksum("$GPRMC,092752.000,A,5321.6802,N,00630.3372,W,0.02,31.66,280511,,,A") +
        NMEAUtils::repairChecksum("$GPGGA,092752.000,5321.6802,N,00630.3372,W,1,8,1.03,61.7,M,55.2,M,,");
    device.feed(recovery);
    QTRY_VERIFY_WITH_TIMEOUT(health.usable(), TestTimeout::shortMs());
    QCOMPARE(updates.size(), previousUpdates + 1);
    QCOMPARE(source.lastObservation().position.timestamp().time(), QTime(9, 27, 52));
    QVERIFY(observations.size() >= 2);
}

void NMEAPositionSourceTest::_lateFixLossDoesNotRejectRecovery_data()
{
    QTest::addColumn<bool>("expiredMetadata");
    QTest::addColumn<bool>("nextDay");
    QTest::newRow("new-epoch") << false << false;
    QTest::newRow("same-epoch-expired-metadata") << true << false;
    QTest::newRow("same-time-next-day") << false << true;
}

void NMEAPositionSourceTest::_lateFixLossDoesNotRejectRecovery()
{
    QFETCH(bool, expiredMetadata);
    QFETCH(bool, nextDay);
    SequentialTestDevice device;
    device.receivedAtUs = MonotonicClock::nowUs() - (expiredMetadata ? 3000000 : 100000);
    NMEAPositionSource source(&device);
    GPSSourceHealth health;
    connect(&source, &NMEAPositionSource::observationReceived, &health, &GPSSourceHealth::updateObservation);
    source.startUpdates();
    device.feed(kFix);
    // Qt does not republish an identical timestamp; keep the expiring epoch pending.
    if (!expiredMetadata) {
        QTRY_VERIFY_WITH_TIMEOUT(health.usable(), TestTimeout::shortMs());
    }
    device.feed(NMEAUtils::repairChecksum("$GPGGA,092749.000,,,,,0,0,,,,,,,"));
    QVERIFY(!source._lossTask.active());
    if (!expiredMetadata) {
        QVERIFY(health.usable());
    }
    QSignalSpy observations(&source, &NMEAPositionSource::observationReceived);
    // Fix loss and recovery may share a transport receipt; their sentence order still matters.
    device.receivedAtUs = MonotonicClock::nowUs();
    const QTime time = expiredMetadata || nextDay ? QTime(9, 27, 50) : QTime(9, 27, 52);
    const QDate date = nextDay ? QDate(2011, 5, 29) : QDate(2011, 5, 28);
    const auto utc = time.toString(u"hhmmss.zzz").toLatin1();
    const QByteArray loss =
        expiredMetadata || nextDay ? "$GPGSA,A,1,,,,,,,,,,,,,99.9,99.9,99.9" : "$GPGGA,092751.000,,,,,0,0,,,,,,,";
    device.feed(NMEAUtils::repairChecksum(loss) +
                NMEAUtils::repairChecksum("$GPRMC," + utc + ",A,5321.6802,N,00630.3372,W,0.02,31.66," +
                                          date.toString(u"ddMMyy").toLatin1() + ",,,A") +
                NMEAUtils::repairChecksum("$GPGGA," + utc + ",5321.6802,N,00630.3372,W,1,8,1.03,61.7,M,55.2,M,,"));
    QTRY_COMPARE_WITH_TIMEOUT(observations.size(), 2, TestTimeout::shortMs());
    QCOMPARE(source.lastObservation().position.timestamp(), QDateTime(date, time, QTimeZone::UTC));
    QVERIFY(health.usable());
    QCOMPARE(observations.front().front().value<GPSObservation>().receiverFixValid, std::optional<bool>(false));
    QVERIFY(observations.back().front().value<GPSObservation>().receiverFixValid.value_or(true));
}

void NMEAPositionSourceTest::_schedulerCanBeDestroyed()
{
    SequentialTestDevice device;
    auto scheduler = std::make_unique<QtRuntimeScheduler>();
    NMEAPositionSource source(&device, nullptr, scheduler.get());
    QSignalSpy observations(&source, &NMEAPositionSource::observationReceived);
    source.requestUpdate(1000);
    QVERIFY(source._requestTask.active());
    scheduler.reset();
    QVERIFY(!source._requestTask.active());
    source.stopUpdates();
    source.startUpdates();
    device.feed(kFix);
    source.requestUpdate(1000);
    QVERIFY(observations.isEmpty());
}

void NMEAPositionSourceTest::_fixLossPreservesPendingRequest()
{
    SequentialTestDevice device;
    NMEAPositionSource source(&device);
    QSignalSpy updates(&source, &QGeoPositionInfoSource::positionUpdated);
    QSignalSpy observations(&source, &NMEAPositionSource::observationReceived);
    source.requestUpdate(1000);
    device.feed(kFix + NMEAUtils::repairChecksum("$GPGGA,092751.000,,,,,0,0,,,,,,,"));
    QTRY_COMPARE_WITH_TIMEOUT(observations.size(), 1, TestTimeout::shortMs());
    QVERIFY(updates.isEmpty());
    QVERIFY(source._requestTask.active());
    QCOMPARE(source.lastObservation().receiverFixValid, std::optional<bool>(false));
    device.feed(NMEAUtils::repairChecksum("$GPRMC,092752.000,A,5321.6802,N,00630.3372,W,0.02,31.66,280511,,,A") +
                NMEAUtils::repairChecksum("$GPGGA,092752.000,5321.6802,N,00630.3372,W,1,8,1.03,61.7,M,55.2,M,,"));
    QTRY_COMPARE_WITH_TIMEOUT(updates.size(), 1, TestTimeout::shortMs());
    QCOMPARE(observations.size(), 2);
    QVERIFY(!source._requestTask.active());
    QCOMPARE(source.lastObservation().position.timestamp().time(), QTime(9, 27, 52));
}

void NMEAPositionSourceTest::_fixLossCanDestroySource()
{
    SequentialTestDevice device;
    auto source = std::make_unique<NMEAPositionSource>(&device);
    connect(source.get(), &NMEAPositionSource::observationReceived, &device, [&](const GPSObservation& observation) {
        if (observation.receiverFixValid == false) {
            source.reset();
        }
    });
    source->startUpdates();
    device.feed(kFix + NMEAUtils::repairChecksum("$GPGGA,092751.000,,,,,0,0,,,,,,,"));
    QTRY_VERIFY_WITH_TIMEOUT(!source, TestTimeout::shortMs());
    QVERIFY(device.isOpen());
}

void NMEAPositionSourceTest::_gllRecoversFromFixLoss()
{
    SequentialTestDevice device;
    NMEAPositionSource source(&device);
    GPSSourceHealth health;
    connect(&source, &NMEAPositionSource::observationReceived, &health, &GPSSourceHealth::updateObservation);
    source.startUpdates();
    device.feed(kFix);
    QTRY_VERIFY_WITH_TIMEOUT(health.usable(), TestTimeout::shortMs());
    device.feed(NMEAUtils::repairChecksum("$GPGLL,,,,,092751.000,V"));
    QTRY_COMPARE_WITH_TIMEOUT(health.state(), GPSSourceHealth::State::Invalid, TestTimeout::shortMs());
    device.feed(NMEAUtils::repairChecksum("$GPGST,092752.000,1,1,1,0,3,4,6") +
                NMEAUtils::repairChecksum("$GPGLL,5321.6802,N,00630.3372,W,092752.000,A"));
    QTRY_VERIFY_WITH_TIMEOUT(health.usable(), TestTimeout::shortMs());
    QCOMPARE(source.lastObservation().position.timestamp().time(), QTime(9, 27, 52));
}
