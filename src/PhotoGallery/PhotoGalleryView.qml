import QtQuick 2.6
import QtQuick.Controls 2.0

import QGroundControl.Controllers   1.0
import QGroundControl               1.0

PhotoGalleryView {
    property PhotoFileStore store : PhotoFileStore {
        id: photoFileStore
        objectName: "PhotoFileStore"
        location: QGroundControl.settingsManager.appSettings.photoSavePath
    }
    property PhotoGalleryVehicleGlue glue : PhotoGalleryVehicleGlue {
        objectName: "PhotoGalleryVehicleGlue"
        manager: QGroundControl.multiVehicleManager
        store: photoFileStore
    }
    model : PhotoGalleryModel {
        id: photoGalleryModel
        objectName: "PhotoGalleryModel"
        store: photoFileStore
    }
    trigger : glue.trigger
    objectName: "PhotoGalleryView"
}
