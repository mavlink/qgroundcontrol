#include "NMEAStreamSplitterTest.h"

#include <QtCore/QIODevice>
#include <QtCore/QPointer>

#include <algorithm>

#include "NMEAPositionSource.h"
#include "NMEAStreamSplitter.h"
#include "NMEAUtils.h"

namespace {
class StreamInput : public QIODevice
{
public:
    StreamInput() { open(ReadOnly); }

    bool isSequential() const override { return true; }

    qint64 bytesAvailable() const override { return QIODevice::bytesAvailable() + _data.size(); }

    void feed(const QByteArray& data)
    {
        _data.append(data);
        emit readyRead();
    }

protected:
    qint64 readData(char* data, qint64 maxSize) override
    {
        const qint64 size = std::min<qint64>(_data.size(), maxSize);
        std::copy_n(_data.constData(), size, data);
        _data.remove(0, size);
        return size;
    }

    qint64 writeData(const char*, qint64) override { return -1; }

private:
    QByteArray _data;
};
}  // namespace

void NMEAStreamSplitterTest::_independentReads()
{
    StreamInput input;
    {
        NMEAStreamSplitter stream(&input);
        auto* position = stream.positionDevice();
        auto* satellite = stream.satelliteDevice();
        const QByteArray first = NMEAUtils::repairChecksum("$GNTXT,first");
        const QByteArray next = NMEAUtils::repairChecksum("$GNTXT,next");
        input.feed(first.first(5));
        QCOMPARE(position->bytesAvailable(), 0);
        QVERIFY(!satellite->canReadLine());
        input.feed(first.mid(5) + next);
        QCOMPARE(position->read(2), first.first(2));
        QCOMPARE(position->readAll(), first.mid(2) + next);
        QCOMPARE(satellite->peek(first.size()), first);
        QVERIFY(satellite->canReadLine());
        QCOMPARE(satellite->readLine(), first);
        QCOMPARE(satellite->readAll(), next);
        input.feed(first);
        position->close();
        QVERIFY(position->open(QIODevice::ReadOnly));
        QVERIFY(position->readAll().isEmpty());
        QCOMPARE(satellite->readAll(), first);
    }
    QVERIFY(input.isOpen());
}

void NMEAStreamSplitterTest::_slowConsumerIsBounded()
{
    StreamInput input;
    NMEAStreamSplitter stream(&input);
    QByteArray received;
    connect(stream.positionDevice(), &QIODevice::readyRead, this,
            [&]() { received += stream.positionDevice()->readAll(); });
    const QByteArray line = NMEAUtils::repairChecksum("$GNTXT,buffer");
    const QByteArray lines = line.repeated(8000);
    input.feed(lines);
    QCOMPARE(received, lines);
    QVERIFY(stream.satelliteDevice()->bytesAvailable() <= 64 * 1024);
    const QByteArray buffered = stream.satelliteDevice()->readAll();
    QVERIFY(!buffered.isEmpty());
    QVERIFY(lines.endsWith(buffered));
    QVERIFY(buffered.startsWith(line));
    input.feed("$" + QByteArray(80 * 1024, 'x'));
    QVERIFY(stream.satelliteDevice()->readAll().isEmpty());
    input.feed("rest\n" + line);
    QCOMPARE(stream.satelliteDevice()->readAll(), line);
}

void NMEAStreamSplitterTest::_sourceDestructionClosesOutputs()
{
    auto input = std::make_unique<StreamInput>();
    NMEAStreamSplitter stream(input.get());
    input->feed(NMEAUtils::repairChecksum("$GNTXT,pending"));
    input.reset();
    QVERIFY(!stream.positionDevice()->isOpen());
    QVERIFY(!stream.satelliteDevice()->isOpen());
    QCOMPARE(stream.positionDevice()->bytesAvailable(), 0);
    QCOMPARE(stream.satelliteDevice()->bytesAvailable(), 0);
}

void NMEAStreamSplitterTest::_destructionDuringDelivery()
{
    StreamInput input;
    auto stream = std::make_unique<NMEAStreamSplitter>(&input);
    const QPointer<QIODevice> satellite(stream->satelliteDevice());
    connect(stream->positionDevice(), &QIODevice::readyRead, this, [&]() { stream.reset(); });
    input.feed(NMEAUtils::repairChecksum("$GNTXT,one") + NMEAUtils::repairChecksum("$GNTXT,two"));
    QVERIFY(!stream);
    QVERIFY(!satellite);
    QVERIFY(input.isOpen());
}

void NMEAStreamSplitterTest::_mixedBinaryAndFragmentedSentences()
{
    StreamInput input;
    NMEAStreamSplitter stream(&input);
    NMEAPositionSource position(stream.positionDevice());
    QSignalSpy fixes(&position, &QGeoPositionInfoSource::positionUpdated);
    position.startUpdates();
    const QByteArray rmc = NMEAUtils::repairChecksum("$GNRMC,120000.00,A,3724.000,N,07918.000,W,0.0,0.0,090926,,,A");
    const QByteArray gga = NMEAUtils::repairChecksum("$GNGGA,120000.00,3724.000,N,07918.000,W,1,12,0.8,100,M,0,M,,");
    const QByteArray garbage = QByteArray("$GNRMC,broken*00\r\n") + QByteArray::fromHex("b56201070c000001242aff00");
    for (const char byte : garbage + rmc + gga) {
        input.feed(QByteArray(1, byte));
    }
    QCOMPARE(stream.satelliteDevice()->readAll(), rmc + gga);
    QTRY_VERIFY_WITH_TIMEOUT(!fixes.isEmpty(), TestTimeout::shortMs());
    QVERIFY(fixes.first().first().value<QGeoPositionInfo>().isValid());
}

UT_REGISTER_TEST(NMEAStreamSplitterTest, TestLabel::Unit)
