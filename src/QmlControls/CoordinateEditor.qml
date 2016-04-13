import QtQuick                  2.5
import QtQuick.Controls         1.3
import QtQuick.Dialogs          1.2
import QtPositioning 5.2

import QGroundControl.ScreenTools   1.0
import QGroundControl.Controls      1.0

Column {
    property var coordItem
    id: container

    function setFromLLCoord() {
        if (!lat.acceptableInput || lon.acceptableInput) {
            refreshCoord()
            return
        }
        coordItem.coordinate = QtPositioning.coordinate(lat.text, lon.text)
    }

    function refreshCoord() {
        var coord = coordItem.coordinate
        lat.text = coord.latitude.toFixed(_decimalPlaces)
        lon.text = coord.longitude.toFixed(_decimalPlaces)
    }

    Connections {
        target: coordItem
        onCoordinateChanged: refreshCoord()
    }

    Component.onCompleted: refreshCoord()

    Column {
        spacing: parent.spacing
        Item {
            height: lat.height
            width:  container.width

            QGCLabel {
                text: "Latitude:"
            }
            QGCTextField {
                id:                lat
                anchors.right:     parent.right
                width:             _editFieldWidth
                inputMethodHints:  Qt.ImhFormattedNumbersOnly
                onEditingFinished: setFromLLCoord()
                validator: DoubleValidator {bottom: -90; top: 90;}
                textColor: acceptableInput ? "black" : "red"
            }
        }
        Item {
            height: lon.height
            width:  container.width

            QGCLabel {
                text: "Longitude:"
            }
            QGCTextField {
                id:                lon
                anchors.right:     parent.right
                width:             _editFieldWidth
                inputMethodHints:  Qt.ImhFormattedNumbersOnly
                onEditingFinished: setFromLLCoord()
                validator: DoubleValidator {bottom: -180; top: 180;}
                textColor: acceptableInput ? "black" : "red"
            }
        }
    }
}
