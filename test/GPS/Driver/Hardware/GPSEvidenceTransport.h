#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>

#include "GPSTransport.h"

struct GPSReceiverConfig;

/// Passive wire observation. ACKs are observations, not proof of the receiver's stored values.
class GPSEvidenceTransport final : public GPSTransport
{
public:
    GPSEvidenceTransport(GPSTransport& transport, const std::atomic_bool& stop);

    GPSOpenResult open() override;
    bool fatalError() const override;
    unsigned fixedBaudrate() const override;
    bool setBaudrate(unsigned baudrate) override;
    GPSReadResult read(uint8_t* buffer, int length, int timeoutMs) override;
    std::chrono::milliseconds configurationWriteTimeout() const override;

    QJsonObject evidence() const;
    QJsonArray requestedSettings(const GPSReceiverConfig& config) const;

protected:
    GPSWriteResult writeData(const uint8_t* buffer, int length, QDeadlineTimer deadline) override;

private:
    void _observe(QByteArray& pending, const uint8_t* data, int length, bool incoming);
    void _recordWrite(const uint8_t* data, int length, const GPSWriteResult& result);

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
