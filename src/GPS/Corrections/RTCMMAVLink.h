#pragma once

#include <functional>

#include <QtCore/QByteArrayView>
#include <QtCore/QList>

#include "GPSCorrectionSettings.h"
#include "RTCMMAVLinkPacket.h"

class RTCMMAVLink
{
public:
    /// Admits one packet to a vehicle link; admission only, not physical delivery or receiver acknowledgement.
    using Output = std::function<bool(const GPSRTCMPacket&)>;
    using OutputProvider = std::function<QList<Output>()>;

    /// Submits to vehicleLinkOutputs() of the setVehicles() selection until setOutputProvider() replaces them.
    RTCMMAVLink();
    ~RTCMMAVLink();
    Q_DISABLE_COPY_MOVE(RTCMMAVLink)

    /// The distinct primary links of live vehicles, excluding log replay: of every vehicle, or of the active vehicle
    /// alone, which is none while no vehicle is active. Outputs hold their link weakly.
    [[nodiscard]] static OutputProvider vehicleLinkOutputs(
        GPSCorrectionSettings::CorrectionVehicles selection = GPSCorrectionSettings::AllVehicles);

    quint64 totalBytesSubmitted() const { return _submittedBytes; }

    void setVehicles(GPSCorrectionSettings::CorrectionVehicles selection) { _vehicles = selection; }
    void setOutputProvider(OutputProvider provider);
    /// Packs @a data once and submits its packets to each output, stopping at the first packet an output refuses.
    /// Returns the bytes submitted.
    quint64 submitToOutputs(QByteArrayView data);

private:
    OutputProvider _outputProvider;
    GPSCorrectionSettings::CorrectionVehicles _vehicles = GPSCorrectionSettings::AllVehicles;
    quint64 _submittedBytes = 0;
    uint8_t _sequenceId = 0;
};
