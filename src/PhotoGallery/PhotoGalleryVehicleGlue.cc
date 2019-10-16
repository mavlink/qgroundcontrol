/****************************************************************************
 *
 *   (c) 2019 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "PhotoGalleryVehicleGlue.h"

#include "AbstractPhotoTrigger.h"
#include "AsyncDownloadPhotoTrigger.h"
#include "MultiVehicleManager.h"
#include "PhotoFileStore.h"
#include "QGCCameraManager.h"
#include "Vehicle.h"

PhotoGalleryVehicleGlue::~PhotoGalleryVehicleGlue()
{
}

PhotoGalleryVehicleGlue::PhotoGalleryVehicleGlue(QObject * parent)
    : QObject(parent),
        _trigger(new AsyncDownloadPhotoTrigger(
            [this] () {return triggerPhoto(); },
            AsyncDownloadPhotoTrigger::Config{}, nullptr, this))
{
}

AbstractPhotoTrigger* PhotoGalleryVehicleGlue::trigger() const
{
    return _trigger.get();
}

MultiVehicleManager* PhotoGalleryVehicleGlue::multiVehicleManager() const
{
    return _multi_vehicle_manager;
}

void PhotoGalleryVehicleGlue::setMultiVehicleManager(MultiVehicleManager* manager)
{
    // We assume that having a new MultiVehicleManager means that the old one
    // is destroyed, as well as all vehicles and cameras transitively attached
    // to it, and that this severes all connections we have established so
    // far.
    _vehicle_infos.clear();
    _multi_vehicle_manager = manager;
    if (_multi_vehicle_manager) {
        connect(
            _multi_vehicle_manager, &MultiVehicleManager::vehicleAdded,
            this, &PhotoGalleryVehicleGlue::addVehicle);
        connect(
            _multi_vehicle_manager, &MultiVehicleManager::vehicleRemoved,
            this, &PhotoGalleryVehicleGlue::removeVehicle);
    }
}

PhotoFileStore* PhotoGalleryVehicleGlue::store() const
{
    return _trigger->store();
}

void PhotoGalleryVehicleGlue::setStore(PhotoFileStore * store)
{
    _trigger->setStore(store);
}

void PhotoGalleryVehicleGlue::addVehicle(Vehicle * vehicle)
{
    _vehicle_infos.push_back(VehicleInfo{ vehicle, nullptr });
    connect(
        vehicle, &Vehicle::dynamicCamerasChanged,
        this,
        [this, vehicle] () {
            vehicleCameraManagerChanged(vehicle);
        });
    vehicleCameraManagerChanged(vehicle);

}

void PhotoGalleryVehicleGlue::removeVehicle(Vehicle * vehicle)
{
    auto i = findVehicle(vehicle);
    if (i == _vehicle_infos.end()) {
        return;
    }
    _vehicle_infos.erase(i);
}

void PhotoGalleryVehicleGlue::vehicleCameraManagerChanged(Vehicle * vehicle)
{
    auto i = findVehicle(vehicle);
    if (i == _vehicle_infos.end()) {
        return;
    }
    VehicleInfo& vehicle_info = *i;

    QGCCameraManager* camera_manager = vehicle->dynamicCameras();
    vehicle_info.camera_manager = camera_manager;

    connect(
        camera_manager, &QGCCameraManager::imageCaptured,
        _trigger.get(), &AsyncDownloadPhotoTrigger::completePhotoWithURI);
    connect(
        camera_manager, &QGCCameraManager::imageCaptureFailure,
        _trigger.get(), &AsyncDownloadPhotoTrigger::completePhotoFailed);
}

PhotoGalleryVehicleGlue::VehicleInfos::iterator PhotoGalleryVehicleGlue::findVehicle(Vehicle * vehicle)
{
    return std::find_if(
        _vehicle_infos.begin(), _vehicle_infos.end(),
        [vehicle](const VehicleInfo& info){ return info.vehicle == vehicle; });
}

bool PhotoGalleryVehicleGlue::triggerPhoto()
{
    for (const auto & vehicle_info : _vehicle_infos) {
        QGCCameraManager* camera_manager = vehicle_info.camera_manager;
        if (!camera_manager) {
            continue;
        }
        QGCCameraControl* camera = camera_manager->currentCameraInstance();
        if (!camera) {
            continue;
        }
        camera->takePhoto();
        return true;
    }
    return false;
}

namespace {

void registerPhotoGalleryVehicleGlueMetaType()
{
    qmlRegisterType<PhotoGalleryVehicleGlue>("QGroundControl.Controllers", 1, 0, "PhotoGalleryVehicleGlue");
}

}  // namespace

Q_COREAPP_STARTUP_FUNCTION(registerPhotoGalleryVehicleGlueMetaType);
