#pragma once

#include <QtCore/QObject>

#include <functional>

#include "DataRateTracker.h"
#include "RTCMMavlinkPacket.h"

class RTCMMavlink : public QObject
{
    Q_OBJECT
    Q_PROPERTY(quint64 totalBytesSent READ totalBytesSent NOTIFY bandwidthChanged)
    Q_PROPERTY(double bandwidthKBps READ bandwidthKBps NOTIFY bandwidthChanged)
    Q_PROPERTY(quint64 totalBytesSubmitted READ totalBytesSubmitted NOTIFY deliveryStatsChanged)

public:
    struct Output
    {
        QString id;
        quint64 session = 0;
        /// True means admitted to the output send API, never physical delivery or receiver acknowledgement.
        std::function<bool(const GpsRtcmPacket&)> submit;
    };

    struct Admission
    {
        QString id;
        quint64 session = 0;
        quint64 queuedBytes = 0;
        /// Includes the zero-length terminator when fragmentation requires one.
        bool complete = false;
    };

    using OutputProvider = std::function<QList<Output>()>;
    using PackResult = RTCMMavlinkPacket::PackResult;
    static constexpr qsizetype kFragmentLen = RTCMMavlinkPacket::kFragmentLen;
    static constexpr qsizetype kMaxFragments = RTCMMavlinkPacket::kMaxFragments;
    static constexpr qsizetype kMaxAssembledLen = RTCMMavlinkPacket::kMaxAssembledLen;

    explicit RTCMMavlink(QObject* parent = nullptr);
    ~RTCMMavlink() override;

    quint64 totalBytesSent() const { return _rateTracker.totalBytes(); }
    double bandwidthKBps() const { return _rateTracker.kBps(); }
    quint64 totalBytesSubmitted() const { return _submittedBytes; }

    void setOutputProvider(OutputProvider provider);
    QList<Admission> submitToOutputs(QByteArrayView data);
    /// Compatibility aggregate; diagnostics should consume individual output admissions.
    quint64 submit(QByteArrayView data);

    static PackResult pack(QByteArrayView data, uint8_t sequenceId)
    {
        return RTCMMavlinkPacket::pack(data, sequenceId);
    }

signals:
    void bandwidthChanged();
    void deliveryStatsChanged();

private:
    OutputProvider _outputProvider;
    quint64 _outputRevision = 0;
    quint64 _submittedBytes = 0;
    uint8_t _sequenceId = 0;
    DataRateTracker _rateTracker;
    bool _submitting = false;
};
