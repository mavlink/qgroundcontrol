#include "VehicleVideoStreams.h"

#include <utility>

#include <QtPositioning/QGeoCoordinate>

#include "MavlinkCameraControlInterface.h"
#include "MultiVehicleManager.h"
#include "QGCCameraManager.h"
#include "QGCLoggingCategory.h"
#include "QGCVideoStreamInfo.h"
#include "QmlObjectListModel.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"

QGC_LOGGING_CATEGORY(VehicleVideoStreamsLog, "Video.VehicleVideoStreams")

namespace {

/// The cameras of @p cameraManager.
QList<MavlinkCameraControlInterface*> vehicleCameras(QGCCameraManager* cameraManager)
{
    QList<MavlinkCameraControlInterface*> cameras;
    QmlObjectListModel* const model = cameraManager->cameras();
    for (int i = 0; i < model->count(); ++i) {
        if (auto* const camera = qobject_cast<MavlinkCameraControlInterface*>(model->get(i))) {
            cameras.append(camera);
        }
    }
    return cameras;
}

}  // namespace

VehicleVideoStreams::VehicleVideoStreams(QObject* parent)
    : QObject(parent)
{}

void VehicleVideoStreams::followActiveVehicle()
{
    MultiVehicleManager* const multiVehicleManager = MultiVehicleManager::instance();
    (void) connect(multiVehicleManager, &MultiVehicleManager::activeVehicleChanged, this,
                   &VehicleVideoStreams::setVehicle);
    setVehicle(multiVehicleManager->activeVehicle());
}

void VehicleVideoStreams::setVehicle(Vehicle* vehicle)
{
    qCDebug(VehicleVideoStreamsLog) << "new vehicle" << vehicle << "old vehicle" << _vehicle;

    if (_vehicle) {
        (void) disconnect(_vehicle, nullptr, this, nullptr);
        (void) disconnect(_vehicle->vehicleLinkManager(), nullptr, this, nullptr);
        QGCCameraManager* const cameraManager = _vehicle->cameraManager();
        if (cameraManager) {
            (void) disconnect(cameraManager, nullptr, this, nullptr);
            // The streams in tiles leave the display first, the current stream after them.
            const QList<MavlinkCameraControlInterface*> cameras = vehicleCameras(cameraManager);
            for (MavlinkCameraControlInterface* const camera : cameras) {
                camera->setExtraStreams({});
            }
            MavlinkCameraControlInterface* const camera = cameraManager->currentCameraInstance();
            // Told already when the video is off.
            if (camera && (camera != _stoppedCamera)) {
                camera->stopStream();
            }
        }
    }
    _stoppedCamera = nullptr;

    _vehicle = vehicle;
    if (_vehicle) {
        (void) connect(_vehicle->vehicleLinkManager(), &VehicleLinkManager::communicationLostChanged, this,
                       &VehicleVideoStreams::communicationLostChanged);
        (void) connect(_vehicle, &Vehicle::armedChanged, this, &VehicleVideoStreams::armedChanged);
        if (QGCCameraManager* const cameraManager = _vehicle->cameraManager()) {
            (void) connect(cameraManager, &QGCCameraManager::streamChanged, this, &VehicleVideoStreams::_readStreams);
        }
    }
    emit vehicleChanged();

    _readStreams();
}

bool VehicleVideoStreams::hasVehicle() const
{
    return !_vehicle.isNull();
}

bool VehicleVideoStreams::disarmed() const
{
    return _vehicle && !_vehicle->armed();
}

bool VehicleVideoStreams::connected() const
{
    return _vehicle && !_vehicle->vehicleLinkManager()->communicationLost();
}

QGeoCoordinate VehicleVideoStreams::position() const
{
    return _vehicle ? _vehicle->coordinate() : QGeoCoordinate();
}

void VehicleVideoStreams::resumeCurrentStream()
{
    QGCCameraManager* const cameraManager = _vehicle ? _vehicle->cameraManager() : nullptr;
    if (MavlinkCameraControlInterface* const camera =
            cameraManager ? cameraManager->currentCameraInstance() : nullptr) {
        camera->resumeStream();
    }
}

void VehicleVideoStreams::setPlaying(bool playing)
{
    QGCCameraManager* const cameraManager = _vehicle ? _vehicle->cameraManager() : nullptr;
    // Also the camera that became the current one, or was found, while the video was off.
    MavlinkCameraControlInterface* const camera =
        (!playing && cameraManager) ? cameraManager->currentCameraInstance() : nullptr;
    if (camera == _stoppedCamera) {
        return;
    }
    if (_stoppedCamera) {
        _stoppedCamera->resumeStream();
    }
    _stoppedCamera = camera;
    if (camera) {
        camera->stopStream();
    }
}

void VehicleVideoStreams::setExtraStreams(const QList<Stream>& inTiles,
                                          const QList<const QGCVideoStreamInfo*>& mainStreams)
{
    QGCCameraManager* const cameraManager = _vehicle ? _vehicle->cameraManager() : nullptr;
    if (!cameraManager) {
        return;
    }
    const QList<MavlinkCameraControlInterface*> cameras = vehicleCameras(cameraManager);
    for (MavlinkCameraControlInterface* const camera : cameras) {
        QList<quint8> streamIds;
        for (const Stream& stream : inTiles) {
            if ((stream.camera == camera) && stream.streamInfo) {
                streamIds.append(stream.streamInfo->streamID());
            }
        }
        // A thermal stream is never its camera's current one, whichever source plays it.
        const QGCVideoStreamInfo* const thermal = camera->thermalStreamInstance();
        if (thermal && mainStreams.contains(thermal)) {
            streamIds.append(thermal->streamID());
        }
        camera->setExtraStreams(streamIds);
    }
}

void VehicleVideoStreams::select(Stream stream)
{
    QGCCameraManager* const cameraManager = _vehicle ? _vehicle->cameraManager() : nullptr;
    MavlinkCameraControlInterface* const camera = stream.camera;
    if (!camera || !stream.streamInfo || !cameraManager) {
        return;
    }
    const int streamIndex = camera->streamLabels().indexOf(stream.streamInfo->name());
    // The stream first: the camera then becomes the current one with the stream to show.
    if (streamIndex >= 0) {
        camera->setCurrentStream(streamIndex);
    }
    cameraManager->setCurrentCamera(cameraManager->cameras()->indexOf(camera));
}

void VehicleVideoStreams::_readStreams()
{
    QGCCameraManager* const cameraManager = _vehicle ? _vehicle->cameraManager() : nullptr;
    if (!cameraManager) {
        emit streamsChanged({}, nullptr, {});
        return;
    }

    // The current camera's streams come first, then the other cameras', in the cameras' order.
    QList<MavlinkCameraControlInterface*> cameras = vehicleCameras(cameraManager);
    MavlinkCameraControlInterface* const currentCamera = cameraManager->currentCameraInstance();
    if (cameras.removeOne(currentCamera)) {
        cameras.prepend(currentCamera);
    }
    const auto advertised = [&cameras](QGCVideoStreamInfo* streamInfo, MavlinkCameraControlInterface* camera) {
        const QString name = (cameras.size() > 1)
                                 ? QStringLiteral("%1: %2").arg(camera->modelName(), streamInfo->name())
                                 : streamInfo->name();
        return Stream{streamInfo, camera, name};
    };
    QGCVideoStreamInfo* const currentStreamInfo = cameraManager->currentStreamInstance();
    Stream current;
    // The current camera's thermal stream, else that of the first other camera that has one: a sender that makes a
    // camera of each of its streams, as mavlink-camera-manager does, has the thermal one on a camera of its own.
    QGCVideoStreamInfo* thermal = nullptr;
    QList<Stream> others;
    for (MavlinkCameraControlInterface* const camera : std::as_const(cameras)) {
        if (!thermal) {
            thermal = camera->thermalStreamInstance();
        }
        QmlObjectListModel* const streams = camera->streams();
        for (int i = 0; streams && (i < streams->count()); ++i) {
            auto* const streamInfo = qobject_cast<QGCVideoStreamInfo*>(streams->get(i));
            if (!streamInfo || streamInfo->isThermal()) {
                continue;
            }
            if (streamInfo == currentStreamInfo) {
                current = advertised(streamInfo, camera);
            } else {
                others.append(advertised(streamInfo, camera));
            }
        }
    }
    // A current camera that advertises thermal streams only has its thermal stream as the main picture, with none
    // over it.
    QGCVideoStreamInfo* const ownThermal = currentCamera ? currentCamera->thermalStreamInstance() : nullptr;
    if (!currentStreamInfo && ownThermal) {
        current = advertised(ownThermal, currentCamera);
        thermal = nullptr;
    }
    emit streamsChanged(current, thermal, others);
}
