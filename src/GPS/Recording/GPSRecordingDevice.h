#pragma once

#include <QtCore/QIODevice>
#include <QtCore/QPointer>

#include <memory>

#include "GPSReadTimestamp.h"
#include "GPSRecordingBuffer.h"

/// Non-owning, unbuffered tap: only the decoder consumes the original passive NMEA stream.
class GPSRecordingDevice : public QIODevice, public GPSReadTimestamp
{
    Q_OBJECT

public:
    GPSRecordingDevice(QIODevice* source, std::shared_ptr<GPSRecordingStream> recording, QObject* parent = nullptr);
    ~GPSRecordingDevice() override;
    bool isSequential() const override;
    qint64 bytesAvailable() const override;
    quint64 lastReadTimestampUs() const override;
    void close() override;

protected:
    qint64 readData(char* data, qint64 maximum) override;
    qint64 writeData(const char* data, qint64 length) override;

private:
    QPointer<QIODevice> _source;
    std::shared_ptr<GPSRecordingStream> _recording;
    quint64 _lastReadUs = 0;
};
