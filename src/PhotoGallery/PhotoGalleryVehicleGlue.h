/****************************************************************************
 *
 *   (c) 2019 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <list>
#include <memory>

#include <QObject>

class AbstractPhotoTrigger;
class AsyncDownloadPhotoTrigger;
class MultiVehicleManager;
class PhotoFileStore;
class QGCCameraManager;
class Vehicle;

/// Bridging code to interface photo gallery with rest of QGCC
///
/// Provides the bridging between the cameras (attached to vehicles, discovered
/// and managed by QGC code) and the photo gallery functionality which is
/// otherwise fully isolated.
///
/// An instance of this class is created through the QML code and wired to the
/// internal QGC bits.
class PhotoGalleryVehicleGlue : public QObject {
    Q_OBJECT

    Q_PROPERTY(AbstractPhotoTrigger* trigger READ trigger);
    Q_PROPERTY(MultiVehicleManager* manager READ multiVehicleManager WRITE setMultiVehicleManager);
    Q_PROPERTY(PhotoFileStore* store READ store WRITE setStore);

public:
    ~PhotoGalleryVehicleGlue() override;

    explicit PhotoGalleryVehicleGlue(QObject * parent = nullptr);

    AbstractPhotoTrigger* trigger() const;
    MultiVehicleManager* multiVehicleManager() const;
    void setMultiVehicleManager(MultiVehicleManager* manager);
    PhotoFileStore* store() const;
    void setStore(PhotoFileStore* store);

public slots:
    /// Connected to vehicle manager to detect new vehicles connecting.
    void addVehicle(Vehicle* vehicle);

    /// Connected to vehicle manager to detect vehicles disconnecting.
    void removeVehicle(Vehicle* vehicle);

private:
    struct VehicleInfo {
        Vehicle* vehicle;
        QGCCameraManager* camera_manager;
    };
    using VehicleInfos = std::list<VehicleInfo>;

    /// Camera manager on vehicle changed.
    ///
    /// \param vehicle The vehicle affected.
    void vehicleCameraManagerChanged(Vehicle * vehicle);

    /// Find vehicle info in data structure.
    ///
    /// \param vehicle The vehicle requested.
    ///
    /// Returns iterator to vehicle info, or end() if not found.
    VehicleInfos::iterator findVehicle(Vehicle * vehicle);

    /// Triggers taking a photo.
    ///
    /// Tries to start taking a photo using one of the vehicles attached.
    bool triggerPhoto();

    std::unique_ptr<AsyncDownloadPhotoTrigger> _trigger;
    MultiVehicleManager* _multi_vehicle_manager = nullptr;

    VehicleInfos _vehicle_infos;
};
