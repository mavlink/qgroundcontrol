#include <QtCore/QScopeGuard>
#include <QtCore/QSemaphore>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "GPSPositionService.h"
#include "GPSReceiverAutoConnect.h"
#include "GPSReceiverFamily.h"
#include "GPSReceiverState.h"
#include "GPSTransport.h"
#include "ManualScheduler.h"
#include "NTRIPGgaProvider.h"

class ReceiverStateLibraryTest : public QObject
{
    Q_OBJECT

private slots:

    void headlessStateFeedsPositionAndNtrip()
    {
        ManualScheduler scheduler;
        GPSReceiverSession session;
        GPSReceiverState state(session, nullptr, &scheduler);
        GPSReceiverAutoConnect connection(&session, state.health(), nullptr, &scheduler);
        QSemaphore entered;
        QSemaphore release;
        const auto cleanup = qScopeGuard([&]() {
            connection.stop();
            release.release();
            session.shutdown();
        });
        QVERIFY(connection.connectNetwork(gpsReceiverFamilies().front().type, [&](const std::atomic_bool&) {
            entered.release();
            release.acquire();
            return std::unique_ptr<GPSTransport>();
        }));
        QVERIFY(entered.tryAcquire(1, 5000));
        auto* worker = session.findChild<GPSProvider*>();
        QVERIFY(worker);
        emit worker->receiverReady();
        QTRY_VERIFY_WITH_TIMEOUT(session.attempt().ready(), 5000);

        GPSPositionService positions(nullptr, &scheduler);
        auto registration = positions.registerPositionSource(GPSPositionService::SelectedSource::Receiver, &state,
                                                             state.health(), session.sessionId());
        GPSObservation observation;
        observation.sessionId = session.sessionId();
        observation.monotonicTimestampUs = scheduler.nowUs();
        observation.position = QGeoPositionInfo(QGeoCoordinate(47, 8, 500), QDateTime::currentDateTimeUtc());
        observation.position.setAttribute(QGeoPositionInfo::HorizontalAccuracy, 1);
        observation.position.setAttribute(QGeoPositionInfo::VerticalAccuracy, 1);
        observation.fixQuality = GPSObservation::FixQuality::Fix3D;
        observation.altitudeDatum = GPSObservation::AltitudeDatum::MeanSeaLevel;
        emit session.positionReceived(observation);
        QCOMPARE(positions.gcsPosition(), observation.position.coordinate());

        NTRIPGgaProvider gga(nullptr, &scheduler);
        gga.configure({NTRIPGgaProvider::PositionSource::GCSPosition, std::chrono::seconds(1)});
        gga.setPositionProvider(NTRIPGgaProvider::PositionSource::GCSPosition, [&]() {
            const auto accepted = positions.acceptedObservation(GPSObservation::PositionUse::NTRIP);
            return accepted ? PositionResult{*accepted, QStringLiteral("receiver")} : PositionResult{};
        });
        QList<QByteArray> sentences;
        gga.start([&](const QByteArray& sentence) { sentences.append(sentence); });
        QVERIFY(!sentences.isEmpty());
        QVERIFY(sentences.first().contains("4700.0000,N,00800.0000,E"));

        QVERIFY(scheduler.advanceBy(std::chrono::seconds(5)));
        QVERIFY(!state.health()->usable());
        QSignalSpy positionsChanged(state.health(), &GPSSourceHealth::positionChanged);
        GPSIntegrityObservation integrity;
        integrity.monotonicTimestampUs = scheduler.nowUs();
        integrity.sessionId = session.sessionId();
        integrity.jammingState = 2;
        emit session.integrityReceived(integrity);
        QVERIFY(state.integrity()->available());
        QCOMPARE(state.integrity()->observation().jammingState.value(), 2);
        QVERIFY(positionsChanged.isEmpty());
        integrity.sessionId += 1;
        integrity.jammingState = 3;
        emit session.integrityReceived(integrity);
        QCOMPARE(state.integrity()->observation().jammingState.value(), 2);
        connection.stop();
        QVERIFY(!state.integrity()->available());
        QVERIFY(!positions.gcsPosition().isValid());
    }
};

QTEST_GUILESS_MAIN(ReceiverStateLibraryTest)
#include "ReceiverStateLibraryTest.moc"
