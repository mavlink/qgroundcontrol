#pragma once

#include <functional>

#include <QtCore/QObject>
#include <QtCore/QString>

#include "DataRateTracker.h"
#include "RTCMMAVLinkPacket.h"

class RTCMMAVLink : public QObject
{
    Q_OBJECT
    Q_PROPERTY(quint64 totalBytesSent READ totalBytesSent NOTIFY bandwidthChanged FINAL)
    Q_PROPERTY(quint64 totalBytesSubmitted READ totalBytesSubmitted NOTIFY deliveryStatsChanged FINAL)

public:
    struct Output
    {
        QString id;
        quint64 session = 0;
        /// Admission only, not physical delivery or receiver acknowledgement.
        std::function<bool(const GPSRTCMPacket&)> submit;
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

    explicit RTCMMAVLink(QObject* parent = nullptr);
    ~RTCMMAVLink() override;

    quint64 totalBytesSent() const { return _rateTracker.totalBytes(); }

    quint64 totalBytesSubmitted() const { return _submittedBytes; }

    void setOutputProvider(OutputProvider provider);
    QList<Admission> submitToOutputs(QByteArrayView data);
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
