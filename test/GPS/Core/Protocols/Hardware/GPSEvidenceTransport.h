#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>

#include "GPSCancellation.h"
#include "GPSTransport.h"

struct GPSReceiverConfig;

/// Passive wire observation. ACKs are observations, not proof of the receiver's stored values.
class GPSEvidenceTransport final : public GPSTransport
{
public:
    GPSEvidenceTransport(GPSTransport& transport, GPSCancelToken cancelToken);

    GPSOpenResult open() override;
    bool fatalError() const override;
    unsigned fixedBaudrate() const override;
    bool setBaudrate(unsigned baudrate) override;
    GPSReadResult read(std::span<uint8_t> buffer, std::chrono::milliseconds timeout) override;
    std::chrono::milliseconds configurationWriteTimeout() const override;

    QJsonObject evidence() const;
    QJsonArray requestedSettings(const GPSReceiverConfig& config, bool passive) const;

protected:
    GPSWriteResult writeData(QByteArrayView bytes, QDeadlineTimer deadline) override;

private:
    void _observe(QByteArray& pending, QByteArrayView bytes, bool incoming);
    void _recordWrite(QByteArrayView bytes, const GPSWriteResult& result);

    GPSTransport& _transport;
    QByteArray _incoming;
    QByteArray _outgoing;
    QJsonArray _frames;
    qint64 _acceptedBytes = 0;
    qint64 _writtenBytes = 0;
    int _failedWrites = 0;
    int _ackCount = 0;
    int _nakCount = 0;
    int _corruptFrames = 0;
    int _omittedFrames = 0;
};
