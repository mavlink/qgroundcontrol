#include "NMEAStreamSplitterTest.h"

#include <QtCore/QIODevice>
#include <QtCore/QPointer>

#include <algorithm>

#include "NMEAStreamSplitter.h"

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
        input.feed("par");
        QCOMPARE(position->bytesAvailable(), 3);
        QCOMPARE(position->read(2), QByteArray("pa"));
        QCOMPARE(satellite->bytesAvailable(), 3);
        QVERIFY(!satellite->canReadLine());
        input.feed("tial\nnext\n");
        QCOMPARE(position->readAll(), QByteArray("rtial\nnext\n"));
        QCOMPARE(satellite->peek(8), QByteArray("partial\n"));
        QVERIFY(satellite->canReadLine());
        QCOMPARE(satellite->readLine(), QByteArray("partial\n"));
        QCOMPARE(satellite->readAll(), QByteArray("next\n"));
        input.feed("buffered\n");
        position->close();
        QVERIFY(position->open(QIODevice::ReadOnly));
        QVERIFY(position->readAll().isEmpty());
        QCOMPARE(satellite->readAll(), QByteArray("buffered\n"));
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
    const QByteArray oldLines = QByteArray("old\n").repeated(12 * 1024);
    const QByteArray newLines = QByteArray("new\n").repeated(12 * 1024);
    input.feed(oldLines + newLines);
    QCOMPARE(received, oldLines + newLines);
    QCOMPARE(stream.satelliteDevice()->bytesAvailable(), 64 * 1024);
    QCOMPARE(stream.satelliteDevice()->readAll(), oldLines.last(16 * 1024) + newLines);
    input.feed(QByteArray(80 * 1024, 'x'));
    QVERIFY(stream.satelliteDevice()->readAll().isEmpty());
    input.feed("rest\nvalid\n");
    QCOMPARE(stream.satelliteDevice()->readAll(), QByteArray("valid\n"));
}

void NMEAStreamSplitterTest::_sourceDestructionClosesOutputs()
{
    auto input = std::make_unique<StreamInput>();
    NMEAStreamSplitter stream(input.get());
    input->feed("pending\n");
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
    input.feed("one\ntwo\n");
    QVERIFY(!stream);
    QVERIFY(!satellite);
    QVERIFY(input.isOpen());
}

UT_REGISTER_TEST(NMEAStreamSplitterTest, TestLabel::Unit)
