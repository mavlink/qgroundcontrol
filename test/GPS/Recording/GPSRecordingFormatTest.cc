#include <QtCore/QBuffer>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtTest/QTest>

#include "GPSReceiverProfile.h"
#include "GPSRecordingBuffer.h"
#include "GPSRecordingFormat.h"

class GPSRecordingFormatTest : public QObject
{
    Q_OBJECT
private slots:

    void provenanceRoundTrip()
    {
        auto buffer = std::make_shared<GPSRecordingBuffer>();
        const GPSRecordingProvenance provenance{QStringLiteral("QGroundControl"), QStringLiteral("abc123-dirty"), 7};
        QVERIFY(buffer->setProvenance(provenance));
        QVERIFY(!buffer->setProvenance({QStringLiteral("host:password@example.org"), {}, 0}));
        QVERIFY(buffer->start());
        QVERIFY(!buffer->setProvenance({}));
        buffer->append(buffer->allocateStream(), {}, false, GPSRecordingEvent::Kind::Open);
        buffer->stop();
        GPSRecordingDocument decoded;
        QString error;
        QVERIFY2(GPSRecordingDocument::decode(buffer->exportJson(), decoded, error), qPrintable(error));
        QCOMPARE(decoded.events.first().metadata.provenance, provenance);
    }

    void streamingCancellationAndErrors()
    {
        GPSRecordingDocument document;
        for (quint64 i = 0; i < 200; ++i)
            document.events.append({.atUs = i, .kind = GPSRecordingEvent::Kind::Rx, .bytes = "data"});
        QBuffer output;
        QVERIFY(output.open(QIODevice::WriteOnly));
        QString error;
        qsizetype completed = 0;
        QVERIFY(!document.writeTo(output, error, [&](qsizetype count) {
            completed = count;
            return count < 70;
        }));
        QCOMPARE(completed, 70);
        QVERIFY(!error.isEmpty());
        GPSRecordingDocument decoded;
        QVERIFY(!GPSRecordingDocument::decode(output.data(), decoded, error));
        output.buffer().clear();
        output.seek(0);
        QVERIFY(document.writeTo(output, error, [&](qsizetype count) {
            completed = count;
            return true;
        }));
        QCOMPARE(completed, document.events.size());
        QVERIFY(GPSRecordingDocument::decode(output.data(), decoded, error));
        QCOMPARE(decoded.events.size(), 200);
        QCOMPARE(document.encode(), output.data());

        class FailingDevice : public QIODevice
        {
        public:
            FailingDevice() { open(QIODevice::WriteOnly); }

            qint64 readData(char*, qint64) override { return -1; }

            qint64 writeData(const char*, qint64) override
            {
                setErrorString(QStringLiteral("disk full"));
                return -1;
            }
        } failure;

        QVERIFY(!document.writeTo(failure, error));
        QVERIFY(error.contains(QStringLiteral("disk full")));
        document.events.last().atUs = 0;
        QVERIFY(document.encode(&error).isEmpty());
        QVERIFY(error.contains(QStringLiteral("Out-of-order")));
    }

    void recordingCodecRejectsMalformed_data()
    {
        QTest::addColumn<QByteArray>("json");
        QTest::newRow("fractional-version") << QByteArray(R"({"version":1.5,"events":[]})");
        QTest::newRow("future-version") << QByteArray(R"({"version":4,"events":[]})");
        QTest::newRow("string-version") << QByteArray(R"({"version":"1","events":[]})");
        QTest::newRow("missing-filetype") << QByteArray(R"({"version":2,"events":[]})");
        QTest::newRow("wrong-limit-type") << QByteArray(R"({"version":1,"limit_reached":0,"events":[]})");
        QTest::newRow("event-array") << QByteArray(R"({"version":1,"events":[[]]})");
        QTest::newRow("string-time") << QByteArray(R"({"version":1,"events":[{"at_us":"2","kind":"open"}]})");
        QTest::newRow("fractional-stream")
            << QByteArray(R"({"version":1,"events":[{"at_us":1,"kind":"open","stream":1.5}]})");
        QTest::newRow("string-error") << QByteArray(
            R"({"version":1,"events":[{"at_us":1,"kind":"read_error","value":"-1"}]})");
        QTest::newRow("invalid-hex") << QByteArray(R"({"version":1,"events":[{"at_us":1,"kind":"rx","hex":"gg"}]})");
        QTest::newRow("nonascii-hex")
            << QStringLiteral("{\"version\":1,\"events\":[{\"at_us\":1,\"kind\":\"rx\",\"hex\":\"\u0131\"}]}").toUtf8();
        QTest::newRow("wrong-hex-type") << QByteArray(R"({"version":1,"events":[{"at_us":1,"kind":"rx","hex":1234}]})");
        QTest::newRow("future-start") << QByteArray(
            R"({"version":1,"events":[{"at_us":1,"started_us":2,"kind":"open"}]})");
        QTest::newRow("wrong-resumed") << QByteArray(
            R"({"version":1,"events":[{"at_us":1,"kind":"open","resumed":"yes"}]})");
        QTest::newRow("metadata-type") << QByteArray(
            R"({"version":1,"events":[{"at_us":1,"kind":"session","profile":[]}]})");
        QTest::newRow("metadata-incomplete")
            << QByteArray(R"({"version":1,"events":[{"at_us":1,"kind":"session","profile":{}}]})");
    }

    void recordingCodecRejectsMalformed()
    {
        QFETCH(QByteArray, json);
        GPSRecordingDocument unchanged;
        unchanged.limitReached = true;
        QString error;
        QVERIFY(!GPSRecordingDocument::decode(json, unchanged, error));
        QVERIFY(!error.isEmpty());
        QVERIFY(unchanged.limitReached);
    }

    void recordingCodecBoundsAndStreamValidation()
    {
        GPSRecordingDocument document;
        QString error;
        QVERIFY(!GPSRecordingDocument::decode(QByteArray(GPSRecordingDocument::MAX_BYTES + 1, ' '), document, error));
        document.events.append({.atUs = 1,
                                .kind = GPSRecordingEvent::Kind::Rx,
                                .bytes = QByteArray(GPSRecordingDocument::MAX_BYTES, 'x')});
        QVERIFY(document.encode(&error).isEmpty());
        const QByteArray badOtherStream = R"({"version":1,"events":[
            {"at_us":1,"stream":7,"kind":"open"},
            {"at_us":2,"stream":8,"kind":"rx","hex":false}]})";
        QVERIFY(!GPSRecordingDocument::decode(badOtherStream, document, error));
        QVERIFY(!error.isEmpty());
    }

    void recordingCodecMetadataAndTiming()
    {
        GPSReceiverProfile profile;
        profile.endpoint.kind = GPSReceiverProfile::Endpoint::Kind::UdpPeer;
        profile.endpoint.host = QStringLiteral("private.example");
        profile.endpoint.device = QStringLiteral("private-device");
        profile.receiverName = QStringLiteral("private-name");
        profile.configurationPolicy = GPSReceiverProfile::ConfigurationPolicy::Configure;
        profile.receiver.outputProtocol = GPSReceiverConfig::OutputProtocol::Native;
        profile.receiver.base.useFixedBase = true;
        profile.receiver.base.fixedBaseLatitude = 47.4;
        const auto metadata = GPSRecordingMetadata::fromProfile(profile);
        GPSRecordingDocument document;
        document.limitReached = true;
        GPSRecordingEvent session{.atUs = 1, .kind = GPSRecordingEvent::Kind::Session};
        session.stream = 7;
        session.metadata = metadata;
        document.events.append(session);
        GPSRecordingEvent open{.atUs = 25, .kind = GPSRecordingEvent::Kind::Open};
        open.startedAtUs = 2;
        open.stream = 7;
        open.resumed = true;
        document.events.append(open);
        QString error;
        const auto bytes = document.encode(&error);
        QVERIFY2(!bytes.isEmpty(), qPrintable(error));
        QVERIFY(!bytes.contains("private"));
        QVERIFY(bytes.contains("udp_peer"));
        GPSRecordingDocument decoded;
        QVERIFY2(GPSRecordingDocument::decode(bytes, decoded, error), qPrintable(error));
        QCOMPARE(decoded.events[0].metadata, metadata);
        QCOMPARE(decoded.events[0].metadata.fixedBaud, 115200u);
        QCOMPARE(decoded.events[1].startedAtUs, quint64(2));
        QVERIFY(decoded.events[1].resumed);
        QCOMPARE(decoded.encode(), bytes);
        QVector<GPSRecordingEvent> selected;
        std::optional<GPSRecordingMetadata> selectedMetadata;
        quint64 selectedId = 0;
        QVERIFY(decoded.selectStream(7, selected, selectedMetadata, selectedId, error));
        QCOMPARE(selected.size(), 2);
        QCOMPARE(selected[1].startedAtUs, quint64(2));
        QVERIFY(!decoded.selectStream(8, selected, selectedMetadata, selectedId, error));
        auto malformed = QJsonDocument::fromJson(bytes).object();
        auto events = malformed["events"].toArray();
        auto event = events[0].toObject();
        auto config = event["profile"].toObject();
        config.insert("host", "credential-bearing-endpoint");
        event.insert("profile", config);
        events[0] = event;
        malformed.insert("events", events);
        QVERIFY(!GPSRecordingDocument::decode(QJsonDocument(malformed).toJson(), decoded, error));
        config.remove("host");
        config.insert("driver", 0);
        event.insert("profile", config);
        events[0] = event;
        malformed.insert("events", events);
        QVERIFY(!GPSRecordingDocument::decode(QJsonDocument(malformed).toJson(), decoded, error));
        // Version 1 transport/driver/role/protocol ordinals have a frozen compatibility mapping.
        malformed.insert("version", 1);
        malformed.remove("fileType");
        config.insert("transport", 2);
        config.insert("driver", 0);
        config.insert("role", 1);
        config.insert("protocol", 0);
        config.remove("fixed_baud");
        event.insert("profile", config);
        events[0] = event;
        malformed.insert("events", events);
        QVERIFY2(GPSRecordingDocument::decode(QJsonDocument(malformed).toJson(), decoded, error), qPrintable(error));
        QCOMPARE(decoded.events[0].metadata.transport, GPSRecordingMetadata::Transport::Tcp);
        QCOMPARE(decoded.events[0].metadata.fixedBaud, 115200u);
        QCOMPARE(decoded.events[0].metadata.driverType, int(GPSType::u_blox));
        QCOMPARE(decoded.events[0].metadata.receiver.role, GPSReceiverConfig::Role::Position);
    }

    void receiptAndTransportOutcomeCompatibility()
    {
        GPSRecordingDocument document;
        GPSRecordingEvent open{.atUs = 1, .kind = GPSRecordingEvent::Kind::OpenError};
        open.openStatus = GPSOpenStatus::TimedOut;
        GPSRecordingEvent receipt{.atUs = 1000, .kind = GPSRecordingEvent::Kind::Rx, .bytes = "abc"};
        receipt.receivedAtUs = -500000;
        receipt.readStatus = GPSReadStatus::Data;
        GPSRecordingEvent overflow{.atUs = 2000, .kind = GPSRecordingEvent::Kind::ReadError};
        overflow.readStatus = GPSReadStatus::Overflow;
        document.events = {open, receipt, overflow};
        QString error;
        const auto bytes = document.encode(&error);
        QVERIFY2(!bytes.isEmpty(), qPrintable(error));
        GPSRecordingDocument decoded;
        QVERIFY2(GPSRecordingDocument::decode(bytes, decoded, error), qPrintable(error));
        QCOMPARE(decoded.events[0].openStatus, open.openStatus);
        QCOMPARE(decoded.events[1].receivedAtUs, receipt.receivedAtUs);
        QCOMPARE(decoded.events[2].readStatus, overflow.readStatus);
        QCOMPARE(decoded.encode(), bytes);
        auto malformed = QJsonDocument::fromJson(bytes).object();
        auto events = malformed["events"].toArray();
        auto rx = events[1].toObject();
        rx.insert("received_us", "-500000");
        events[1] = rx;
        malformed.insert("events", events);
        QVERIFY(!GPSRecordingDocument::decode(QJsonDocument(malformed).toJson(), decoded, error));
        rx.insert("received_us", 1001);
        events[1] = rx;
        malformed.insert("events", events);
        QVERIFY(!GPSRecordingDocument::decode(QJsonDocument(malformed).toJson(), decoded, error));
        document.events[2].readStatus = GPSReadStatus::TimedOut;
        QVERIFY(document.encode(&error).isEmpty());
        // Version 2 has no receipt field; the replay defaults to the recorded completion time.
        QVERIFY(GPSRecordingDocument::decode(
            R"({"fileType":"GPSRecording","version":2,"events":[{"at_us":1,"kind":"rx","hex":"6162"}]})", decoded,
            error));
        QVERIFY(!decoded.events.first().receivedAtUs);
    }

    void passiveNetworkBaudRemainsUnknown()
    {
        GPSReceiverProfile profile;
        profile.endpoint.kind = GPSReceiverProfile::Endpoint::Kind::UdpListener;
        profile.configurationPolicy = GPSReceiverProfile::ConfigurationPolicy::Passive;
        const auto metadata = GPSRecordingMetadata::fromProfile(profile);
        QVERIFY(!metadata.configured);
        QCOMPARE(metadata.fixedBaud, 0u);
        QCOMPARE(metadata.driverType, -1);
    }
};

QTEST_GUILESS_MAIN(GPSRecordingFormatTest)
#include "GPSRecordingFormatTest.moc"
