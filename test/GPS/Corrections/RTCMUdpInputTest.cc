#include "RTCMUdpInputTest.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QEvent>
#include <QtCore/QPointer>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtNetwork/QUdpSocket>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "GpsTestHelpers.h"
#include "LogManager.h"
#include "RTCMUdpInput.h"

namespace {

bool sendDatagram(quint16 port, const QByteArray& payload)
{
    QUdpSocket sender;
    return sender.writeDatagram(payload, QHostAddress::LocalHost, port) == payload.size();
}

}  // namespace

void RTCMUdpInputTest::_testStartStop()
{
    RTCMUdpInput input(0);
    QVERIFY(input.start());
    QVERIFY(input.isRunning());
    QVERIFY(input.port() != 0);  // ephemeral port resolved on bind

    input.stop();
    QVERIFY(!input.isRunning());
}

void RTCMUdpInputTest::_testSocketErrors_data()
{
    QTest::addColumn<bool>("restart");
    QTest::newRow("retired-after-stop") << false;
    QTest::newRow("retired-after-restart") << true;
}

void RTCMUdpInputTest::_testSocketErrors()
{
    QFETCH(bool, restart);
    qRegisterMetaType<QAbstractSocket::SocketError>();
    RTCMUdpInput input(0);
    input.setValidation(true);
    QVERIFY(input.start());
    const QPointer<QUdpSocket> socket = input.findChild<QUdpSocket*>();
    QVERIFY(socket);
    QSignalSpy frames(&input, &RTCMUdpInput::frameReceived);
    QSignalSpy rejected(&input, &RTCMUdpInput::frameRejected);
    QSignalSpy runningChanges(&input, &RTCMUdpInput::runningChanged);
    QSignalSpy reads(socket, &QUdpSocket::readyRead);
    QUdpSocket sender;
    const auto frame = GpsTestHelpers::buildRtcmFrame(1005, 20);
    const auto prefix = frame.first(5);
    QCOMPARE(sender.writeDatagram(prefix, QHostAddress::LocalHost, input.port()), prefix.size());
    QTRY_VERIFY_WITH_TIMEOUT(!reads.isEmpty(), TestTimeout::mediumMs());
    QVERIFY(frames.isEmpty());
    expectLogMessage("GPS.Corrections.RTCMUdpInput", QtWarningMsg,
                     QRegularExpression(QStringLiteral("UDP socket error on port %1.*%2")
                                            .arg(input.port())
                                            .arg(QRegularExpression::escape(socket->errorString()))));
    QVERIFY(QMetaObject::invokeMethod(socket, "errorOccurred", Qt::DirectConnection,
                                      Q_ARG(QAbstractSocket::SocketError, QAbstractSocket::NetworkError)));
    verifyExpectedLogMessage();
    QVERIFY(input.isRunning());
    QCOMPARE(socket->state(), QAbstractSocket::BoundState);
    QVERIFY(runningChanges.isEmpty());
    QVERIFY(frames.isEmpty());
    QVERIFY(rejected.isEmpty());
    const auto tail = frame.sliced(5);
    QCOMPARE(sender.writeDatagram(tail, QHostAddress::LocalHost, input.port()), tail.size());
    QTRY_COMPARE_WITH_TIMEOUT(frames.size(), 1, TestTimeout::mediumMs());
    QCOMPARE(qvariant_cast<GPSCorrectionFrame>(frames.first().first()).data, frame);
    frames.clear();

    QSignalSpy errors(socket, &QUdpSocket::errorOccurred);
    QVERIFY(QMetaObject::invokeMethod(
        socket, [socket]() { emit socket->errorOccurred(QAbstractSocket::TemporaryError); }, Qt::QueuedConnection));
    input.stop();
    if (restart) {
        QVERIFY(input.start());
    }
    const QString category = QStringLiteral("GPS.Corrections.RTCMUdpInput");
    const auto messageCount = LogManager::capturedMessages(category).size();
    const auto runningChangeCount = runningChanges.size();
    QVERIFY(socket);
    QCoreApplication::sendPostedEvents(socket, QEvent::MetaCall);
    QCOMPARE(errors.size(), 1);
    QCOMPARE(LogManager::capturedMessages(category).size(), messageCount);
    QCOMPARE(runningChanges.size(), runningChangeCount);
    QCOMPARE(input.isRunning(), restart);
    QVERIFY(frames.isEmpty());
    QVERIFY(rejected.isEmpty());
    if (restart) {
        QVERIFY(sendDatagram(input.port(), frame));
        QTRY_COMPARE_WITH_TIMEOUT(frames.size(), 1, TestTimeout::mediumMs());
    }
}

void RTCMUdpInputTest::_testStartNotificationReentrancy_data()
{
    QTest::addColumn<bool>("portNotification");
    QTest::addColumn<bool>("destroy");
    QTest::newRow("stop-before-running") << true << false;
    QTest::newRow("delete-before-running") << true << true;
    QTest::newRow("stop-after-running") << false << false;
    QTest::newRow("delete-after-running") << false << true;
}

void RTCMUdpInputTest::_testStartNotificationReentrancy()
{
    QFETCH(bool, portNotification);
    QFETCH(bool, destroy);
    QPointer<RTCMUdpInput> input = new RTCMUdpInput(0);
    const auto cleanup = qScopeGuard([&]() { delete input.data(); });
    bool interrupted = false;
    const auto notification = portNotification ? &RTCMUdpInput::portChanged : &RTCMUdpInput::runningChanged;
    const auto connection = connect(input, notification, this, [&]() {
        if (interrupted) {
            return;
        }
        interrupted = true;
        if (destroy) {
            delete input.data();
        } else {
            input->stop();
        }
    });
    QVERIFY(!input->start());
    QVERIFY(interrupted);
    if (destroy) {
        QVERIFY(input.isNull());
    } else {
        QVERIFY(!input->isRunning());
        disconnect(connection);
        QVERIFY(input->start());
    }
}

void RTCMUdpInputTest::_testPassthroughWithoutValidation()
{
    RTCMUdpInput input(0);
    QVERIFY(input.start());
    QSignalSpy spy(&input, &RTCMUdpInput::frameReceived);

    // Validation off (default): datagram forwarded as-is, valid RTCM or not.
    const QByteArray payload = QByteArrayLiteral("not-rtcm-at-all");
    QVERIFY(sendDatagram(input.port(), payload));

    QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 1, 2000);
    QCOMPARE(qvariant_cast<GPSCorrectionFrame>(spy.at(0).at(0)).data, payload);
}

void RTCMUdpInputTest::_testStartupPortReplacement_data()
{
    QTest::addColumn<bool>("restart");
    QTest::newRow("cancel-original-attempt") << false;
    QTest::newRow("retain-newer-attempt") << true;
}

void RTCMUdpInputTest::_testStartupPortReplacement()
{
    QFETCH(bool, restart);
    RTCMUdpInput input(0);
    quint16 originalPort = 0;
    bool replacementStarted = false;
    connect(&input, &RTCMUdpInput::portChanged, this, [&]() {
        if (originalPort != 0) {
            return;
        }
        originalPort = input.port();
        input.setPort(0);
        if (restart) {
            replacementStarted = input.start();
        }
    });
    QVERIFY(!input.start());
    QVERIFY(originalPort != 0);
    QCOMPARE(input.isRunning(), restart);
    QCOMPARE(replacementStarted, restart);
    if (restart) {
        QSignalSpy frames(&input, &RTCMUdpInput::frameReceived);
        QVERIFY(sendDatagram(input.port(), QByteArrayLiteral("replacement")));
        QTRY_COMPARE_WITH_TIMEOUT(frames.size(), 1, TestTimeout::mediumMs());
    } else {
        QUdpSocket released;
        QVERIFY(released.bind(QHostAddress::AnyIPv4, originalPort, QUdpSocket::DontShareAddress));
        QCOMPARE(input.port(), quint16(0));
    }
}

void RTCMUdpInputTest::_testValidationResetsStream()
{
    RTCMUdpInput input(0);
    input.setValidation(true);
    QVERIFY(input.start());
    QSignalSpy frames(&input, &RTCMUdpInput::frameReceived);
    QUdpSocket sender;
    const auto frame = GpsTestHelpers::buildRtcmFrame(1005, 20);
    const auto prefix = frame.first(5);
    QCOMPARE(sender.writeDatagram(prefix, QHostAddress::LocalHost, input.port()), prefix.size());
    QVERIFY(QMetaObject::invokeMethod(&input, "_readDatagrams", Qt::DirectConnection));
    QVERIFY(frames.isEmpty());

    input.setValidation(false);
    const auto raw = QByteArrayLiteral("raw");
    QCOMPARE(sender.writeDatagram(raw, QHostAddress::LocalHost, input.port()), raw.size());
    QTRY_COMPARE_WITH_TIMEOUT(frames.size(), 1, TestTimeout::mediumMs());
    QVERIFY(!qvariant_cast<GPSCorrectionFrame>(frames.first().first()).validated);

    input.setValidation(true);
    const auto tailAndFrame = frame.sliced(5) + frame;
    QCOMPARE(sender.writeDatagram(tailAndFrame, QHostAddress::LocalHost, input.port()), tailAndFrame.size());
    QTRY_COMPARE_WITH_TIMEOUT(frames.size(), 2, TestTimeout::mediumMs());
    QCOMPARE(qvariant_cast<GPSCorrectionFrame>(frames.last().first()).data, frame);
    QVERIFY(qvariant_cast<GPSCorrectionFrame>(frames.last().first()).validated);
}

void RTCMUdpInputTest::_testReentrantDrainPreservesOrder()
{
    RTCMUdpInput input(0);
    input.setValidation(true);
    QVERIFY(input.start());
    QSignalSpy frames(&input, &RTCMUdpInput::frameReceived);
    bool reentered = false;
    connect(&input, &RTCMUdpInput::frameReceived, this, [&]() {
        if (!reentered) {
            reentered = true;
            QVERIFY(QMetaObject::invokeMethod(&input, "_readDatagrams", Qt::DirectConnection));
        }
    });
    const auto first = GpsTestHelpers::buildRtcmFrame(1005, 20);
    const auto second = GpsTestHelpers::buildRtcmFrame(1077, 40);
    const auto head = first + second.first(5);
    const auto tail = second.sliced(5);
    QUdpSocket sender;
    QCOMPARE(sender.writeDatagram(head, QHostAddress::LocalHost, input.port()), head.size());
    QCOMPARE(sender.writeDatagram(tail, QHostAddress::LocalHost, input.port()), tail.size());
    QVERIFY(QMetaObject::invokeMethod(&input, "_readDatagrams", Qt::DirectConnection));
    QVERIFY(reentered);
    QTRY_COMPARE_WITH_TIMEOUT(frames.size(), 2, TestTimeout::mediumMs());
    QCOMPARE(qvariant_cast<GPSCorrectionFrame>(frames.first().first()).data, first);
    QCOMPARE(qvariant_cast<GPSCorrectionFrame>(frames.last().first()).data, second);
}

void RTCMUdpInputTest::_testDrainInterruption_data()
{
    QTest::addColumn<int>("action");
    QTest::newRow("stop") << 0;
    QTest::newRow("restart") << 1;
    QTest::newRow("delete") << 2;
    QTest::newRow("validation-change") << 3;
}

void RTCMUdpInputTest::_testDrainInterruption()
{
    QFETCH(int, action);
    QPointer<RTCMUdpInput> input = new RTCMUdpInput(0);
    const auto cleanup = qScopeGuard([&]() { delete input.data(); });
    input->setValidation(true);
    QVERIFY(input->start());
    QSignalSpy frames(input, &RTCMUdpInput::frameReceived);
    bool interrupted = false;
    connect(input, &RTCMUdpInput::frameReceived, this, [&]() {
        if (interrupted) {
            return;
        }
        interrupted = true;
        switch (action) {
            case 0:
                input->stop();
                break;
            case 1:
                QVERIFY(input->start());
                break;
            case 2:
                delete input.data();
                break;
            case 3:
                input->setValidation(false);
                input->setValidation(true);
                break;
        }
    });
    const auto frame = GpsTestHelpers::buildRtcmFrame(1005, 20);
    QVERIFY(sendDatagram(input->port(), frame + frame));
    QVERIFY(QMetaObject::invokeMethod(input, "_readDatagrams", Qt::DirectConnection));
    QVERIFY(interrupted);
    QCOMPARE(frames.size(), 1);
    if (action == 2) {
        QVERIFY(input.isNull());
    } else if (action == 0) {
        QVERIFY(!input->isRunning());
    } else {
        QVERIFY(input->isRunning());
        QVERIFY(sendDatagram(input->port(), frame));
        QTRY_COMPARE_WITH_TIMEOUT(frames.size(), 2, TestTimeout::mediumMs());
    }
}

void RTCMUdpInputTest::_testEmitsOneSignalPerFrame()
{
    RTCMUdpInput input(0);
    input.setValidation(true);
    QVERIFY(input.start());
    QSignalSpy spy(&input, &RTCMUdpInput::frameReceived);

    // One datagram carrying two frames plus leading garbage: each frame must be
    // emitted separately so RTCMMavlink assigns it its own sequence.
    const QByteArray frame1 = GpsTestHelpers::buildRtcmFrame(1005, 4);
    const QByteArray frame2 = GpsTestHelpers::buildRtcmFrame(1077, 200);
    const QByteArray garbage = QByteArrayLiteral("\x01\x02\x03");
    QVERIFY(sendDatagram(input.port(), garbage + frame1 + frame2));

    QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 2, 2000);
    QCOMPARE(qvariant_cast<GPSCorrectionFrame>(spy.at(0).at(0)).data, frame1);
    QCOMPARE(qvariant_cast<GPSCorrectionFrame>(spy.at(1).at(0)).data, frame2);
}

void RTCMUdpInputTest::_testDropsBadCrcFrame()
{
    RTCMUdpInput input(0);
    input.setValidation(true);
    QVERIFY(input.start());
    QSignalSpy spy(&input, &RTCMUdpInput::frameReceived);
    QSignalSpy rejected(&input, &RTCMUdpInput::frameRejected);

    const QByteArray frame1 = GpsTestHelpers::buildRtcmFrame(1005, 4);
    QByteArray corrupted = GpsTestHelpers::buildRtcmFrame(1077, 8);
    corrupted[corrupted.size() - 1] = static_cast<char>(corrupted[corrupted.size() - 1] ^ 0xFF);
    const QByteArray frame2 = GpsTestHelpers::buildRtcmFrame(1087, 2);

    expectLogMessage("GPS.Corrections.RTCMUdpInput", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Dropped 1 RTCM frame")));
    QVERIFY(sendDatagram(input.port(), frame1 + corrupted + frame2));

    QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 2, 2000);
    verifyExpectedLogMessage();
    QCOMPARE(qvariant_cast<GPSCorrectionFrame>(spy.at(0).at(0)).data, frame1);
    QCOMPARE(qvariant_cast<GPSCorrectionFrame>(spy.at(1).at(0)).data, frame2);
    QCOMPARE(rejected.size(), 1);
    const auto candidate = qvariant_cast<GPSCorrectionFrame>(rejected.first().first());
    QCOMPARE(candidate.data, corrupted);
    QVERIFY(!candidate.validated);
    QCOMPARE(qvariant_cast<GPSCorrectionReason>(rejected.first().at(1)), GPSCorrectionReason::InvalidFrame);
}

void RTCMUdpInputTest::_testFrameSplitAcrossDatagrams()
{
    RTCMUdpInput input(0);
    input.setValidation(true);
    QVERIFY(input.start());
    QSignalSpy spy(&input, &RTCMUdpInput::frameReceived);

    // Parser state must carry across datagrams so a frame split by the sender
    // still comes out whole.
    const QByteArray frame = GpsTestHelpers::buildRtcmFrame(1005, 6);
    const int split = frame.size() / 2;
    QUdpSocket sender;
    QCOMPARE(sender.writeDatagram(frame.left(split), QHostAddress::LocalHost, input.port()), split);
    QCOMPARE(sender.writeDatagram(frame.mid(split), QHostAddress::LocalHost, input.port()), frame.size() - split);

    QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 1, 2000);
    QCOMPARE(qvariant_cast<GPSCorrectionFrame>(spy.at(0).at(0)).data, frame);
}

void RTCMUdpInputTest::_testRecoversBufferedFrames()
{
    RTCMUdpInput input(0);
    input.setValidation(true);
    QVERIFY(input.start());
    QSignalSpy frames(&input, &RTCMUdpInput::frameReceived);
    QSignalSpy rejected(&input, &RTCMUdpInput::frameRejected);
    const auto first = GpsTestHelpers::buildRtcmFrame(1005, 4);
    const auto second = GpsTestHelpers::buildRtcmFrame(1087, 2);
    const auto payload = first + second;
    auto corrupted = GpsTestHelpers::buildRtcmFrame(1006, static_cast<int>(payload.size()));
    corrupted.replace(5, payload.size(), payload);
    corrupted.chop(3);
    const auto crc =
        RTCMFramer::crc24q({reinterpret_cast<const uint8_t*>(corrupted.constData()), size_t(corrupted.size())});
    corrupted.append(QByteArray(3, '\0'));
    if (crc == 0) {
        corrupted.back() = '\1';
    }
    expectLogMessage("GPS.Corrections.RTCMUdpInput", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Dropped 1 RTCM frame")));
    QVERIFY(sendDatagram(input.port(), corrupted));
    QTRY_COMPARE_WITH_TIMEOUT(frames.size(), 2, TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    QCOMPARE(rejected.size(), 1);
    QCOMPARE(qvariant_cast<GPSCorrectionFrame>(rejected.first().first()).data, corrupted);
    const auto firstFrame = qvariant_cast<GPSCorrectionFrame>(frames.first().first());
    const auto secondFrame = qvariant_cast<GPSCorrectionFrame>(frames.last().first());
    QCOMPARE(firstFrame.data, first);
    QCOMPARE(secondFrame.data, second);
    QCOMPARE(firstFrame.sourceInstance, secondFrame.sourceInstance);
    QCOMPARE(firstFrame.receivedAtMs, secondFrame.receivedAtMs);
}

void RTCMUdpInputTest::_testInterleavedSenders()
{
    RTCMUdpInput input(0);
    input.setValidation(true);
    QVERIFY(input.start());
    QSignalSpy frames(&input, &RTCMUdpInput::frameReceived);
    QSignalSpy envelopes(&input, &RTCMUdpInput::frameReceived);
    QUdpSocket senderA;
    QUdpSocket senderB;
    const QByteArray frameA = GpsTestHelpers::buildRtcmFrame(1005, 20);
    const QByteArray frameB = GpsTestHelpers::buildRtcmFrame(1077, 40);
    const int split = 5;
    QCOMPARE(senderA.writeDatagram(frameA.first(split), QHostAddress::LocalHost, input.port()), split);
    QCOMPARE(senderB.writeDatagram(frameB, QHostAddress::LocalHost, input.port()), frameB.size());
    QTRY_COMPARE_WITH_TIMEOUT(frames.count(), 1, TestTimeout::mediumMs());
    QCOMPARE(qvariant_cast<GPSCorrectionFrame>(frames.first().first()).data, frameB);
    QCOMPARE(senderA.writeDatagram(frameA.sliced(split), QHostAddress::LocalHost, input.port()), frameA.size() - split);
    QTRY_COMPARE_WITH_TIMEOUT(frames.count(), 2, TestTimeout::mediumMs());
    QCOMPARE(qvariant_cast<GPSCorrectionFrame>(frames.last().first()).data, frameA);
    QCOMPARE(envelopes.size(), 2);
    const auto envelopeB = qvariant_cast<GPSCorrectionFrame>(envelopes.first().first());
    const auto envelopeA = qvariant_cast<GPSCorrectionFrame>(envelopes.last().first());
    QVERIFY(envelopeB.sourceInstance.endsWith(QLatin1Char(':') + QString::number(senderB.localPort())));
    QVERIFY(envelopeA.sourceInstance.endsWith(QLatin1Char(':') + QString::number(senderA.localPort())));
    QVERIFY(envelopeA.sourceInstance != envelopeB.sourceInstance);
    QVERIFY(envelopeA.receivedAtMs <= envelopeB.receivedAtMs);
    QCOMPARE(envelopeA.data, frameA);
}

void RTCMUdpInputTest::_testBurstYieldsBetweenDrains()
{
    RTCMUdpInput input(0);
    input.setValidation(true);
    QVERIFY(input.start());
    QSignalSpy frames(&input, &RTCMUdpInput::frameReceived);
    QUdpSocket sender;
    const QByteArray payload = GpsTestHelpers::buildRtcmFrame(1005, 20);
    for (int i = 0; i < 40; ++i) {
        QCOMPARE(sender.writeDatagram(payload, QHostAddress::LocalHost, input.port()), payload.size());
    }
    // Invoke one drain without processing the event loop: it must leave work for a continuation.
    QVERIFY(QMetaObject::invokeMethod(&input, "_readDatagrams", Qt::DirectConnection));
    QVERIFY(frames.size() > 0);
    QVERIFY(frames.size() <= 16);
    QTRY_COMPARE_WITH_TIMEOUT(frames.size(), 40, TestTimeout::mediumMs());
    for (const auto& received : frames) {
        QCOMPARE(qvariant_cast<GPSCorrectionFrame>(received.first()).data, payload);
    }
}

UT_REGISTER_TEST(RTCMUdpInputTest, TestLabel::Unit)
