#pragma once

#include <functional>

#include <QtCore/QByteArrayView>
#include <QtCore/QList>

#include "RTCMMAVLinkPacket.h"

class RTCMMAVLink
{
public:
    /// Admits one packet to a vehicle link; admission only, not physical delivery or receiver acknowledgement.
    using Output = std::function<bool(const GPSRTCMPacket&)>;
    using OutputProvider = std::function<QList<Output>()>;

    /// Submits to vehicleLinkOutputs() until setOutputProvider() replaces them.
    RTCMMAVLink();
    ~RTCMMAVLink();
    Q_DISABLE_COPY_MOVE(RTCMMAVLink)

    /// The distinct primary links of live vehicles, excluding log replay. Outputs hold their link weakly.
    [[nodiscard]] static OutputProvider vehicleLinkOutputs();

    quint64 totalBytesSubmitted() const { return _submittedBytes; }

    void setOutputProvider(OutputProvider provider);
    /// Packs @a data once and submits its packets to each output, stopping at the first packet an output refuses.
    /// Returns the bytes submitted.
    quint64 submitToOutputs(QByteArrayView data);

private:
    OutputProvider _outputProvider;
    quint64 _submittedBytes = 0;
    uint8_t _sequenceId = 0;
};
