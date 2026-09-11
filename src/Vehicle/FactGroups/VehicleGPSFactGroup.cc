#include "VehicleGPSFactGroup.h"

#include <QtCore/QPointer>

#include "MAVLinkLib.h"
#include "QGCLoggingCategory.h"
#include "Vehicle.h"
#include "VehicleGPSObservation.h"
#include "development/mavlink_msg_gnss_integrity.h"

VehicleGPSFactGroup::VehicleGPSFactGroup(QObject* parent, VehicleGPSObservationStream* stream, int receiverIndex)
    : GPSPositionFactGroup(parent)
    , _stream(stream ? stream : new VehicleGPSObservationStream(this))
    , _ownsStream(!stream)
    , _receiverIndex(receiverIndex)
{
    integrity()->bindStore(_stream->integrity(receiverIndex));
    connect(_stream, &VehicleGPSObservationStream::gpsReceived, this,
            [this](int receiver, const VehicleGPSObservation& observation) {
                if (receiver != _receiverIndex) {
                    return;
                }
                if (observation.position.monotonicTimestampUs) {
                    updatePosition(observation.position, observation.satellitesVisible, observation.fixType);
                } else {
                    const QPointer<VehicleGPSFactGroup> guard(this);
                    resetPosition();
                    if (guard) {
                        count()->setRawValue(-1);
                    }
                }
            });
    connect(_stream, &VehicleGPSObservationStream::integrityReceived, this, [this](int receiver) {
        if (receiver == _receiverIndex) {
            emit gnssIntegrityReceived();
        }
    });
    _addFactAlias(systemErrors());
    _addFactAlias(spoofingState());
    _addFactAlias(jammingState());
    _addFactAlias(authenticationState());
    _addFactAlias(correctionsQuality());
    _addFactAlias(systemQuality());
    _addFactAlias(gnssSignalQuality());
    _addFactAlias(postProcessingQuality());
}

void VehicleGPSFactGroup::handleMessage(Vehicle* vehicle, const mavlink_message_t& message)
{
    if (_ownsStream && _stream) {
        _stream->handleMessage(message, vehicle ? vehicle->id() : 0, vehicle ? vehicle->defaultComponentId() : 0);
    }
}
