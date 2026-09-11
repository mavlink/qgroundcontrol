#include <QtNetwork/QSslSocket>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "ManualScheduler.h"
#include "NTRIPGgaProvider.h"
#include "NTRIPHttpDecoder.h"
#include "NTRIPHttpTransport.h"
#include "NTRIPSourceTable.h"
#include "NTRIPTlsPolicy.h"

class NTRIPLibraryTest : public QObject
{
    Q_OBJECT
private slots:

    void ggaRecoveryReplacementAndCancellation()
    {
        ManualScheduler clock;
        NTRIPGgaProvider provider(nullptr, &clock);
        PositionResult position;
        provider.setPositionProvider(NTRIPGgaProvider::PositionSource::GCSPosition, [&] { return position; });
        int first = 0;
        int replacement = 0;
        provider.start([&](const QByteArray&) { ++first; });
        QCOMPARE(first, 0);
        position.observation.position = QGeoPositionInfo(QGeoCoordinate(47, 8, 500), QDateTime::currentDateTimeUtc());
        position.observation.position.setAttribute(QGeoPositionInfo::HorizontalAccuracy, 1);
        position.observation.monotonicTimestampUs = clock.nowUs();
        position.observation.fixQuality = GPSObservation::FixQuality::Fix3D;
        position.observation.altitudeDatum = GPSObservation::AltitudeDatum::MeanSeaLevel;
        position.source = QStringLiteral("Ground station");
        position.fixedReference = true;
        QVERIFY(clock.advanceBy(std::chrono::seconds(1)));
        QCOMPARE(first, 1);
        provider.configure({NTRIPGgaProvider::PositionSource::GCSPosition, std::chrono::seconds(2)});
        provider.start([&](const QByteArray& sentence) {
            QVERIFY(sentence.startsWith("$GPGGA,"));
            ++replacement;
        });
        QCOMPARE(replacement, 1);
        QVERIFY(clock.advanceBy(std::chrono::seconds(2)));
        QCOMPARE(replacement, 2);
        QCOMPARE(first, 1);
        provider.stop();
        QVERIFY(clock.advanceBy(std::chrono::hours(1)));
        QCOMPARE(replacement, 2);
        QCOMPARE(clock.pendingCount(), 0);
    }

    void tlsFailureAndCancellation()
    {
        QVERIFY(!NTRIPTlsPolicy::canIgnore({QSslError(QSslError::SelfSignedCertificate)}, false));
        QVERIFY(NTRIPTlsPolicy::canIgnore({QSslError(QSslError::SelfSignedCertificate)}, true));
        QVERIFY(!NTRIPTlsPolicy::canIgnore({QSslError(QSslError::HostNameMismatch)}, true));
        QVERIFY(!NTRIPTlsPolicy::canIgnore({QSslError(QSslError::CertificateExpired)}, true));
        if (!QSslSocket::supportsSsl())
            QSKIP("TLS backend unavailable");
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost));
        ManualScheduler clock;
        NTRIPTransportConfig config;
        config.host = QStringLiteral("127.0.0.1");
        config.port = server.serverPort();
        config.mountpoint = QStringLiteral("TEST");
        config.useTls = true;
        NTRIPHttpTransport transport(config, nullptr, &clock);
        QSignalSpy failures(&transport, &NTRIPStream::failed);
        QSignalSpy connected(&transport, &NTRIPStream::connected);
        transport.start();
        QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), 2000);
        auto* peer = server.nextPendingConnection();
        peer->write("HTTP/1.1 400 Bad Request\r\n\r\n");
        peer->disconnectFromHost();
        QTRY_COMPARE_WITH_TIMEOUT(failures.count(), 1, 2000);
        QCOMPARE(connected.count(), 0);
        transport.stop();
        QVERIFY(clock.advanceBy(std::chrono::minutes(1)));
        QCOMPARE(failures.count(), 1);
        QCOMPARE(clock.pendingCount(), 0);
    }

    void casterFramingAndCatalog()
    {
        NTRIPHttpDecoder decoder;
        auto result = decoder.feed("HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n3\r\nabc\r\n0\r\n\r\n");
        QVERIFY(!result.failure);
        QCOMPARE(result.body, QByteArray("abc"));
        QVERIFY(result.complete);
        decoder.reset();
        result = decoder.feed(QByteArray(NTRIPHttpDecoder::MAX_LINE_BYTES + 1, 'x'));
        QVERIFY(result.failure);
        NTRIPMountpoint point;
        QVERIFY(NTRIPMountpoint::fromSourceTableLine(
            "STR;BASE;Station;RTCM 3.2;;2;GPS;NET;CHE;47;8;1;0;Caster;none;B;N;9600;", point));
        QCOMPARE(point.mountpoint, QStringLiteral("BASE"));
        QCOMPARE(point.distanceFrom({47, 8}), 0.0);
    }
};
QTEST_GUILESS_MAIN(NTRIPLibraryTest)
#include "NTRIPLibraryTest.moc"
