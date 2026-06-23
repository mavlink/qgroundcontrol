import QtQuick
import QtLocation
import QtPositioning

import QGroundControl
import QGroundControl.Controls

/// Home position map visual for MissionSettingsItem
MissionItemMapVisualBase {
    id: control

    indicatorComponent: homeIndicatorComponent

    Component {
        id: homeIndicatorComponent

        MapQuickItem {
            coordinate: control._missionItem.coordinate
            visible: control._missionItem.specifiesCoordinate
            z: QGroundControl.zOrderMapItems
            opacity: control.opacity
            anchorPoint.x: _homeImage.width / 2
            anchorPoint.y: _homeImage.height / 2

            sourceItem: Image {
                id: _homeImage
                source: "qrc:///qmlimages/MapHome.svg"

                property real _smallRadiusRaw: Math.ceil((ScreenTools.defaultFontPixelHeight * ScreenTools.smallFontPointRatio) / 2)
                property real _smallRadius:    _smallRadiusRaw + ((_smallRadiusRaw % 2 == 0) ? 1 : 0)

                width: _smallRadius * 2
                height: width
                sourceSize.width: width
                sourceSize.height: height
                fillMode: Image.PreserveAspectFit

                MouseArea {
                    anchors.fill: parent
                    onClicked: if (control.interactive) control.clicked(control._missionItem.sequenceNumber)
                }
            }
        }
    }
}
