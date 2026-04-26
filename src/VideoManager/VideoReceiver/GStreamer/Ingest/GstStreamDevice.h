#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QIODevice>
#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>

/// Sequential device fed by the GStreamer ingest pipeline and read by
/// QtMultimedia's FFmpeg backend through QMediaPlayer::setSourceDevice().
class GstStreamDevice : public QIODevice
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(GstStreamDevice)

public:
    explicit GstStreamDevice(QObject* parent = nullptr);
    ~GstStreamDevice() override;

    [[nodiscard]] bool isSequential() const override { return true; }
    [[nodiscard]] bool atEnd() const override;
    [[nodiscard]] qint64 bytesAvailable() const override;

    void close() override;
    void resetStream();
    void finishStream();
    bool append(const char* data, qint64 size);

protected:
    qint64 readData(char* data, qint64 maxSize) override;
    qint64 writeData(const char* data, qint64 maxSize) override;

private:
    static constexpr qsizetype kMaxBufferedBytes = 4 * 1024 * 1024;

    mutable QMutex _mutex;
    QWaitCondition _hasData;
    QWaitCondition _hasSpace;
    QByteArray _buffer;
    bool _finished = false;
};
