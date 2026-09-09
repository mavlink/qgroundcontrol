#include "GPSReceiverSessionTest.h"

#include <QtCore/QEvent>
#include <QtCore/QPointer>
#include <QtCore/QScopeGuard>
#include <QtCore/QSemaphore>
#include <QtPositioning/QGeoPositionInfoSource>

#include <memory>

#include "GPSReceiverSession.h"
#include "GPSTransport.h"
#include "NMEADecoderSession.h"
#include "NMEAStreamSplitter.h"
#include "NMEAUtils.h"

namespace {
const QByteArray kRecordedFix =
    "$GPRMC,092750.000,A,5321.6802,N,00630.3372,W,0.02,31.66,280511,,,A*43\r\n"
    "$GPGGA,092750.000,5321.6802,N,00630.3372,W,1,8,1.03,61.7,M,55.2,M,,*76\r\n";
}

void GPSReceiverSessionTest::_cancelBeforeStart_data()
{
    QTest::addColumn<QString>("action");
    QTest::newRow("disconnect") << QStringLiteral("disconnect");
    QTest::newRow("shutdown") << QStringLiteral("shutdown");
    QTest::newRow("destroy") << QStringLiteral("destroy");
}

void GPSReceiverSessionTest::_cancelBeforeStart()
{
    QFETCH(QString, action);
    auto session = std::make_unique<GPSReceiverSession>();
    QPointer<GPSProvider> provider;
    auto lease = std::make_shared<int>(0);
    const std::weak_ptr<int> reservation = lease;
    bool opened = false;
    auto factory = [lease = std::move(lease), &opened](const std::atomic_bool&) {
        opened = true;
        return std::unique_ptr<GPSTransport>();
    };
    const auto cancellation = connect(session.get(), &GPSReceiverSession::receiverTypeChanged, this, [&]() {
        provider = session->_provider;
        if (action == QStringLiteral("shutdown")) {
            session->shutdown();
        } else if (action == QStringLiteral("destroy")) {
            session.reset();
        } else {
            session->stop();
        }
    });
    session->start(GPSType::u_blox, std::move(factory), {});
    QVERIFY(!opened);
    QVERIFY(provider.isNull());
    QVERIFY(reservation.expired());
    if (session) {
        QVERIFY(!session->hasReceiver());
        QVERIFY(!session->stopping());
        disconnect(cancellation);
        if (action == QStringLiteral("disconnect")) {
            session->start(GPSType::u_blox, {}, {});
            QVERIFY(session->hasReceiver());
            QTRY_VERIFY_WITH_TIMEOUT(!session->hasReceiver(), TestTimeout::mediumMs());
        }
        session->shutdown();
    }
}

void GPSReceiverSessionTest::_nmeaStreamBoundsPendingBytes()
{
    GPSByteStream stream;
    const auto buffer = stream.buffer();
    int notifications = 0;
    for (int index = 0; index < 1024; ++index) {
        notifications += buffer->append(QByteArray(4096, 'x')) ? 1 : 0;
    }
    QCOMPARE(notifications, 1);
    QVERIFY(stream.bytesAvailable() <= 64 * 1024);
    const QByteArray nextSentence("$GPRMC,next*00\r\n");
    QVERIFY(!buffer->append(nextSentence));
    const auto pending = stream.readAll();
    QVERIFY(pending.endsWith(nextSentence));
    QCOMPARE(stream.bytesAvailable(), 0);
    QVERIFY(buffer->append(nextSentence));
    QCOMPARE(stream.readAll(), nextSentence);
}

void GPSReceiverSessionTest::_nmeaStreamPreservesReceiptAge_data()
{
    QTest::addColumn<int>("ageMs");
    QTest::newRow("queued-four-seconds") << 4000;
    QTest::newRow("expired-before-delivery") << 6000;
}

void GPSReceiverSessionTest::_nmeaStreamPreservesReceiptAge()
{
    QFETCH(int, ageMs);
    GPSByteStream stream;
    NMEADecoderSession decoder;
    QVERIFY(decoder.start(&stream));
    decoder.positionSource()->startUpdates();
    const quint64 queuedAt = GPSObservation::monotonicNowUs() - ageMs * 1000u;
    stream.buffer()->append(kRecordedFix, queuedAt);
    stream.buffer()->append(NMEAUtils::repairChecksum("$GPGSV,1,1,00"));
    stream.notifyReadyRead();
    if (ageMs < GPSSourceHealth::FRESHNESS_TIMEOUT_MS) {
        QTRY_VERIFY_WITH_TIMEOUT(decoder.health()->usable(), TestTimeout::shortMs());
        const auto observation = decoder.health()->observation();
        QVERIFY(GPSSourceHealth::ageMilliseconds(observation.monotonicTimestampUs) >= ageMs);
        QTRY_VERIFY_WITH_TIMEOUT(!decoder.health()->usable(), TestTimeout::mediumMs());
    } else {
        QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
        QVERIFY(!decoder.health()->usable());
        QVERIFY(!decoder.positionSource()->lastKnownPosition().isValid());
    }
}

void GPSReceiverSessionTest::_nmeaStreamDiscardsPartialFixAcrossGap()
{
    GPSByteStream stream;
    NMEAStreamSplitter splitter(&stream);
    const qsizetype split = kRecordedFix.indexOf('*');
    stream.buffer()->append(kRecordedFix.first(split));
    stream.notifyReadyRead();
    QVERIFY(splitter.positionDevice()->readAll().isEmpty());
    stream.buffer()->append(QByteArray("lost"), GPSObservation::monotonicNowUs() - 6000000u);
    const QByteArray fresh = NMEAUtils::repairChecksum("$GPGSV,1,1,00");
    stream.buffer()->append(kRecordedFix.mid(split, 5) + fresh);
    stream.notifyReadyRead();
    QCOMPARE(splitter.positionDevice()->readAll(), fresh);
}

void GPSReceiverSessionTest::_destroyDuringStreamClose()
{
    struct Gate
    {
        QSemaphore entered;
        QSemaphore release;
    };

    const auto gate = std::make_shared<Gate>();
    auto session = std::make_unique<GPSReceiverSession>();
    const auto cleanup = qScopeGuard([&]() { gate->release.release(); });
    GPSReceiverConfig config;
    config.role = GPSReceiverConfig::Role::Position;
    config.outputProtocol = GPSReceiverConfig::OutputProtocol::NMEA;
    session->start(
        GPSType::u_blox,
        [gate](const std::atomic_bool&) {
            gate->entered.release();
            gate->release.acquire();
            return std::unique_ptr<GPSTransport>();
        },
        config);
    QTRY_VERIFY_WITH_TIMEOUT(gate->entered.available() > 0, TestTimeout::mediumMs());
    QPointer<GPSProvider> worker = session->_provider;
    connect(session->nmeaDevice(), &QObject::destroyed, this, [&]() { session.reset(); });
    session->stop();
    QVERIFY(!session);
    QVERIFY(worker);
    QVERIFY(!worker->parent());
    gate->release.release();
    QTRY_VERIFY_WITH_TIMEOUT(worker.isNull(), TestTimeout::mediumMs());
}

UT_REGISTER_TEST(GPSReceiverSessionTest, TestLabel::Unit)
