import QtQuick

import QGroundControl
import QGroundControl.Controls

QGCFileDialog {
    id:             kmlOrSHPLoadDialog
    folder:         QGroundControl.settingsManager.appSettings.missionSavePath
    title:          qsTr("Select File")
    nameFilters:    ShapeFileHelper.fileDialogKMLOrSHPFilters
}
