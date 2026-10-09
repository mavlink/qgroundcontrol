#include "GPSRecordingTransportTest.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <memory>

#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QRegularExpression>

#include "Fixtures/RAIIFixtures.h"
#include "GPSCancellation.h"
#include "GPSTransport.h"
#include "Protocols/Support/GPSProtocolTestData.h"
#include "Transport/Support/GPSRecordingTransport.h"

using namespace std::chrono_literals;
using namespace GPSTest;

namespace {
/// Serves its input in fixed chunks; once drained, every read times out at once rather than waiting out its timeout.
/// Writes are kept.
class MemoryGPSTransport final : public GPSTransport
{
public:
    explicit MemoryGPSTransport(QByteArray input = {}, int chunkBytes = 64, GPSCancelToken cancelToken = {})
        : GPSTransport(std::move(cancelToken))
        , _input(std::move(input))
        , _chunkBytes(chunkBytes)
    {}

    GPSOpenResult open() override { return {GPSOpenStatus::Opened}; }

    bool fatalError() const override { return false; }

    GPSReadResult read(std::span<uint8_t> buffer, std::chrono::milliseconds) override
    {
        if (isCancelled()) {
            return {GPSReadStatus::Cancelled};
        }
        if (drained()) {
            return {GPSReadStatus::TimedOut};
        }
        const int count = static_cast<int>(
            std::min<qsizetype>({static_cast<qsizetype>(buffer.size()), _chunkBytes, _input.size() - _offset}));
        std::memcpy(buffer.data(), _input.constData() + _offset, static_cast<size_t>(count));
        _offset += count;
        return {GPSReadStatus::Data, count};
    }

    bool setBaudrate(unsigned) override { return true; }

    bool drained() const { return _offset >= _input.size(); }

    const QByteArray& written() const { return _written; }

protected:
    GPSWriteResult writeData(QByteArrayView bytes, QDeadlineTimer) override
    {
        _written.append(bytes);
        const auto length = static_cast<int>(bytes.size());
        return {GPSWriteStatus::Completed, length, length};
    }

private:
    QByteArray _input;
    QByteArray _written;
    qsizetype _offset = 0;
    int _chunkBytes;
};

constexpr const char* RECORDING_LOG = "Test.GPS.Transport.GPSRecordingTransport";

QByteArray bytePattern(int size, int seed = 0)
{
    QByteArray bytes(size, Qt::Uninitialized);
    for (int index = 0; index < size; ++index) {
        bytes[index] = static_cast<char>((index * 31 + seed) & 0xff);
    }
    return bytes;
}

/// Reads until the transport has nothing left and returns what it delivered.
QByteArray readAvailable(GPSTransport& transport)
{
    QByteArray received;
    uint8_t buffer[512];
    for (;;) {
        const auto result = transport.read(buffer, 0ms);
        if (result.status != GPSReadStatus::Data) {
            return received;
        }
        received.append(reinterpret_cast<const char*>(buffer), result.bytesRead);
    }
}

GPSWriteResult writeBytes(GPSTransport& transport, const QByteArray& bytes)
{
    return transport.write(bytes, QDeadlineTimer(TestTimeout::shortDuration()));
}
}  // namespace

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
    const auto files = GPSRecordingTransport::sessionFiles(directory.path(), GPSType::passive,
                                                           QDateTime(QDate(2026, 1, 2), QTime(3, 4, 5)));
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
    QCOMPARE(readFile(files.received).value_or(QByteArray()), input.left(100));
    QCOMPARE(readFile(files.sent).value_or(QByteArray()), (command + command).left(100));
}

UT_REGISTER_TEST_LIGHTWEIGHT(GPSRecordingTransportTest, TestLabel::Unit)
