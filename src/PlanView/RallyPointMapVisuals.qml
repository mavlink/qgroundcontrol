import QtQuick
import QtQuick.Controls
import QtLocation
import QtPositioning

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlightMap

/// Rally Point map visuals
Item {
    id: _root
    z: QGroundControl.zOrderMapItems

    property var    map
    property var    myRallyPointController
    property bool   interactive:            false   ///< true: user can interact with items
    property bool   planView:               false   ///< true: visuals showing in plan view

    property bool   _interactive:           interactive
    property var    _rallyPointsComponent
    property bool   _rallyPointsSupported:  myRallyPointController.supported
    property var    _rallyPoints:           myRallyPointController.points

    Component.onCompleted: {
        _rallyPointsComponent = rallyPointsComponent.createObject(map)
    }

    Component.onDestruction: {
        _rallyPointsComponent.destroy()
    }

    Component {
        id: dragAreaComponent

        MissionItemIndicatorDrag {
            mapControl:     _root.map
            itemCoordinate: rallyPointObject.coordinate
            visible:        rallyPointObject === myRallyPointController.currentRallyPoint && _root.interactive

            property var rallyPointObject

            // The drag only moves the point. Its altitude is read live because the drag breaks the itemCoordinate binding
            onItemCoordinateChanged: rallyPointObject.coordinate = QtPositioning.coordinate(itemCoordinate.latitude, itemCoordinate.longitude, rallyPointObject.coordinate.altitude)

            // A PX4 AMSL altitude is the ground height under the point, so it is read again where the point lands
            onDragStop: {
                const vehicle = myRallyPointController.masterController.controllerVehicle
                if (vehicle && vehicle.px4Firmware && rallyPointObject.altitudeFrame === QGroundControl.AltitudeFrameAbsolute) {
                    rallyPointObject.useTerrainAltitude()
                }
            }
        }
    }

    Component {
        id: rallyPointComponent

        MapQuickItem {
            id:             itemIndicator
            anchorPoint.x:  sourceItem.anchorPointX
            anchorPoint.y:  sourceItem.anchorPointY
            z:              QGroundControl.zOrderMapItems
            opacity:        _root.opacity

            property var rallyPointObject

            sourceItem: MissionItemIndexLabel {
                id:                 itemIndexLabel
                label:              qsTr("R", "rally point map item label")
                checked:            _editingLayer == _layerRally ? rallyPointObject === myRallyPointController.currentRallyPoint : false
                highlightSelected:  true
                onClicked: {
                    if (mainWindow.allowViewSwitch()) {
                        myRallyPointController.currentRallyPoint = rallyPointObject
                    }
                }
            }
        }
    }

    // Add all rally points to the map
    Component {
        id: rallyPointsComponent

        Repeater {
            model: _rallyPoints

            delegate: Item {
                opacity:        _root.opacity
                property var _visuals: [ ]

                Component.onCompleted: {
                    var rallyPointIndicator = rallyPointComponent.createObject(map)
                    rallyPointIndicator.coordinate = Qt.binding(function() { return object.coordinate })
                    rallyPointIndicator.rallyPointObject = Qt.binding(function() { return object })
                    map.addMapItem(rallyPointIndicator)
                    _visuals.push(rallyPointIndicator)

                    var dragArea = dragAreaComponent.createObject(map, { "itemIndicator": rallyPointIndicator, "rallyPointObject": object })
                    _visuals.push(dragArea)
                }

                Component.onDestruction: {
                    for (var i=0; i<_visuals.length; i++) {
                        _visuals[i].destroy()
                    }
                    _visuals = [ ]
                }
            }
        }
    }

}
