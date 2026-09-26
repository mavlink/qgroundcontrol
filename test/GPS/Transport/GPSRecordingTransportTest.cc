#include <memory>
#include <stop_token>

#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QRegularExpression>

#include "Fixtures/RAIIFixtures.h"
#include "GPSRecordingTransport.h"
#include "MemoryGPSTransport.h"
#include "UnitTest.h"

using namespace std::chrono_literals;

namespace {
constexpr const char* RECORDING_LOG = "GPS.Transport.GPSRecordingTransport";

QByteArray bytePattern(int size, int seed = 0)
{
    QByteArray bytes(size, Qt::Uninitialized);
    for (int index = 0; index < size; ++index) {
        bytes[index] = static_cast<char>((index * 31 + seed) & 0xff);
    }
    return bytes;
}

QByteArray fileContents(const QString& path)
{
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}

/// Reads until the transport has nothing left and returns what it delivered.
QByteArray readAvailable(GPSTransport& transport)
{
    QByteArray received;
    uint8_t buffer[512];
    for (;;) {
        const auto result = transport.read(buffer, sizeof(buffer), 0ms);
        if (result.status != GPSReadStatus::Data) {
            return received;
        }
        received.append(reinterpret_cast<const char*>(buffer), result.bytesRead);
    }
}

GPSWriteResult writeBytes(GPSTransport& transport, const QByteArray& bytes)
{
    return transport.write(reinterpret_cast<const uint8_t*>(bytes.constData()), static_cast<int>(bytes.size()),
                           QDeadlineTimer(TestTimeout::shortDuration()));
}

const QDateTime SESSION_START(QDate(2026, 1, 2), QTime(3, 4, 5));
}  // namespace

class GPSRecordingTransportTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _teesBothDirections();
    void _sessionFileNames_data();
    void _sessionFileNames();
    void _fileErrorKeepsLink_data();
    void _fileErrorKeepsLink();
    void _filesStopAtSizeCap();
};

void GPSRecordingTransportTest::_teesBothDirections()
{
    TestFixtures::TempDirFixture directory;
    const auto files =
        GPSRecordingTransport::sessionFiles(directory.path() + QStringLiteral("/GPS"), GPSType::ublox, SESSION_START);
    const QByteArray input = bytePattern(300);
    auto inner = std::make_unique<MemoryGPSTransport>(input, 7);
    MemoryGPSTransport* const link = inner.get();
    const QByteArray command = QByteArrayLiteral("\xb5\x62\x0a\x04\x00\x00\x0e\x34");
    const QByteArray partial = QByteArrayLiteral("\xb5\x62\x06\x8a\x09");
    {
        GPSRecordingTransport recorder(std::move(inner), files);
        QVERIFY(!QFileInfo::exists(files.received));
        QCOMPARE(recorder.open().status, GPSOpenStatus::Opened);
        QVERIFY(recorder.recording());
        QCOMPARE(readAvailable(recorder), input);
        QCOMPARE(writeBytes(recorder, command).status, GPSWriteStatus::Completed);
        // Only the bytes the link accepted can have reached the receiver.
        link->acceptLimit = 2;
        const auto limited = writeBytes(recorder, partial);
        QCOMPARE(limited.status, GPSWriteStatus::TimedOut);
        QCOMPARE(limited.acceptedBytes, 2);
        QCOMPARE(link->written(), command + partial.left(2));
    }
    QCOMPARE(fileContents(files.received), input);
    QCOMPARE(fileContents(files.sent), command + partial.left(2));
}

void GPSRecordingTransportTest::_sessionFileNames_data()
{
    QTest::addColumn<GPSType>("type");
    QTest::addColumn<QString>("received");
    QTest::newRow("ublox") << GPSType::ublox << QStringLiteral("gps-ublox-20260102-030405-rx.ubx");
    QTest::newRow("septentrio") << GPSType::septentrio << QStringLiteral("gps-septentrio-20260102-030405-rx.sbf");
    QTest::newRow("passive") << GPSType::passive << QStringLiteral("gps-passive-20260102-030405-rx.nmea");
    QTest::newRow("trimble") << GPSType::trimble << QStringLiteral("gps-trimble-20260102-030405-rx.nmea");
    QTest::newRow("quectel") << GPSType::quectel << QStringLiteral("gps-quectel-20260102-030405-rx.nmea");
    QTest::newRow("femto") << GPSType::femto << QStringLiteral("gps-femto-20260102-030405-rx.bin");
    QTest::newRow("unicore") << GPSType::unicore << QStringLiteral("gps-unicore-20260102-030405-rx.bin");
}

void GPSRecordingTransportTest::_sessionFileNames()
{
    QFETCH(GPSType, type);
    QFETCH(QString, received);
    TestFixtures::TempDirFixture directory;
    const auto files = GPSRecordingTransport::sessionFiles(directory.path(), type, SESSION_START);
    QCOMPARE(files.received, QDir(directory.path()).filePath(received));
    QCOMPARE(QFileInfo(files.sent).fileName(),
             received.section(QStringLiteral("-rx."), 0, 0) + QStringLiteral("-tx.bin"));

    // A second session in the same second never reuses either name.
    QVERIFY(!directory.createFile(QFileInfo(files.sent).fileName()).isEmpty());
    const auto next = GPSRecordingTransport::sessionFiles(directory.path(), type, SESSION_START);
    QCOMPARE(QFileInfo(next.received).fileName(),
             QString(received).replace(QStringLiteral("-rx."), QStringLiteral("-2-rx.")));
}

void GPSRecordingTransportTest::_fileErrorKeepsLink_data()
{
    QTest::addColumn<QString>("failure");
    QTest::newRow("unwritable-directory") << QStringLiteral("unwritable-directory");
    QTest::newRow("write-error") << QStringLiteral("write-error");
}

void GPSRecordingTransportTest::_fileErrorKeepsLink()
{
    QFETCH(QString, failure);
    TestFixtures::TempDirFixture directory;
    GPSRecordingTransport::Files files;
    if (failure == QStringLiteral("unwritable-directory")) {
        // A regular file where the recording folder should be.
        const QString blocker = directory.createFile(QStringLiteral("GPS"), "not a folder");
        files = {blocker + QStringLiteral("/rx.ubx"), blocker + QStringLiteral("/tx.bin")};
    } else {
        if (!QFileInfo::exists(QStringLiteral("/dev/full"))) {
            QSKIP("Needs /dev/full to fail file writes");
        }
        files = {QStringLiteral("/dev/full"), QDir(directory.path()).filePath(QStringLiteral("tx.bin"))};
    }
    // More than QFile's write buffer, so a failing device reports the error during the session.
    const QByteArray input = bytePattern(64 * 1024, 5);
    auto inner = std::make_unique<MemoryGPSTransport>(input, 4096);
    MemoryGPSTransport* const link = inner.get();
    GPSRecordingTransport recorder(std::move(inner), files);

    expectLogMessage(RECORDING_LOG, QtWarningMsg, QRegularExpression(QStringLiteral("file error")));
    QCOMPARE(recorder.open().status, GPSOpenStatus::Opened);
    QCOMPARE(readAvailable(recorder), input);
    verifyExpectedLogMessage();
    QVERIFY(!recorder.recording());
    QCOMPARE(writeBytes(recorder, QByteArrayLiteral("$PQTMVERNO*58\r\n")).status, GPSWriteStatus::Completed);
    QCOMPARE(link->written(), QByteArrayLiteral("$PQTMVERNO*58\r\n"));
    QVERIFY(!recorder.fatalError());
}

void GPSRecordingTransportTest::_filesStopAtSizeCap()
{
    TestFixtures::TempDirFixture directory;
    const auto files = GPSRecordingTransport::sessionFiles(directory.path(), GPSType::passive, SESSION_START);
    const QByteArray input = bytePattern(250, 1);
    const QByteArray command = bytePattern(60, 2);
    {
        GPSRecordingTransport recorder(std::make_unique<MemoryGPSTransport>(input, 30), files, 100);
        QCOMPARE(recorder.open().status, GPSOpenStatus::Opened);
        expectLogMessage(RECORDING_LOG, QtWarningMsg, QRegularExpression(QStringLiteral("size limit.*rx\\.nmea")));
        QCOMPARE(readAvailable(recorder), input);
        verifyExpectedLogMessage();
        // The received file stopped; the sent file still records until it reaches its own cap.
        QVERIFY(recorder.recording());
        expectLogMessage(RECORDING_LOG, QtWarningMsg, QRegularExpression(QStringLiteral("size limit.*tx\\.bin")));
        for (int index = 0; index < 3; ++index) {
            QCOMPARE(writeBytes(recorder, command).status, GPSWriteStatus::Completed);
        }
        verifyExpectedLogMessage();
        QVERIFY(!recorder.recording());
    }
    QCOMPARE(fileContents(files.received), input.left(100));
    QCOMPARE(fileContents(files.sent), (command + command).left(100));
}

UT_REGISTER_TEST(GPSRecordingTransportTest, TestLabel::Unit)

#include "GPSRecordingTransportTest.moc"
