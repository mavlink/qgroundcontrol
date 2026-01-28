import QtQuick

import QGroundControl
import QGroundControl.Controls

QGCFileDialog {
    id:             shapeFileLoadDialog
    folder:         QGroundControl.settingsManager.appSettings.missionSavePath
    title:          qsTr("Select File")
    nameFilters:    GeoFormatRegistryQml.fileDialogFilters
}
