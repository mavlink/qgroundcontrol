import QtQuick                  2.2
import QtQuick.Controls         1.2

import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Vehicle       1.0
import QGroundControl.Controls      1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Palette       1.0

// Editor for Survery mission items
Rectangle {
    id:         _root
    height:     visible ? (editorColumn.height + (_margin * 2)) : 0
    width:      availableWidth
    color:      qgcPal.windowShadeDark
    radius:     _radius

    // The following properties must be available up the hierachy chain
    //property real   availableWidth    ///< Width for control
    //property var    missionItem       ///< Mission Item for editor

    property real _margin: ScreenTools.defaultFontPixelWidth / 2

    property var _cameraInfoCanonSX260: { "focalLength": 4.5, "sensorHeight": 4.55, "sensorWidth": 6.17 }

    function recalcFromCameraValues() {
        var focalLength = Number(focalLengthField.text)
        var sensorWidth = Number(sensorWidthField.text)
        var sensorHeight = Number(sensorHeightField.text)
        var overlap = Number(imageOverlapField.text)

        if (focalLength <= 0.0 || sensorWidth <= 0.0 || sensorHeight <= 0.0) {
            return
        }

        var scaledFocalLengthMM = (1000.0 * missionItem.gridAltitude.rawValue) / focalLength
        var imageWidthM = (sensorWidth * scaledFocalLengthMM) / 1000.0;
        var imageHeightM = (sensorHeight * scaledFocalLengthMM) / 1000.0;

        var gridSpacing
        var cameraTriggerDistance
        if (cameraOrientationLandscape.checked) {
            gridSpacing = imageWidthM
            cameraTriggerDistance = imageHeightM
        } else {
            gridSpacing = imageHeightM
            cameraTriggerDistance = imageWidthM
        }
        gridSpacing = (1.0 - (overlap / 100.0)) * gridSpacing
        cameraTriggerDistance = (1.0 - (overlap / 100.0)) * cameraTriggerDistance

        missionItem.gridSpacing.rawValue = gridSpacing
        missionItem.cameraTriggerDistance.rawValue = cameraTriggerDistance
    }

    Connections {
        target: editorMap.polygonDraw

        onPolygonCaptureStarted: {
            missionItem.clearPolygon()
        }

        onPolygonCaptureFinished: {
            for (var i=0; i<coordinates.length; i++) {
                missionItem.addPolygonCoordinate(coordinates[i])
            }
        }

        onPolygonAdjustVertex: missionItem.adjustPolygonCoordinate(vertexIndex, vertexCoordinate)
    }

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    ExclusiveGroup {
        id:                 cameraOrientationGroup
        onCurrentChanged:   recalcFromCameraValues()
    }

    Column {
        id:                 editorColumn
        anchors.margins:    _margin
        anchors.top:        parent.top
        anchors.left:       parent.left
        anchors.right:      parent.right
        spacing:            _margin

        QGCLabel {
            wrapMode:       Text.WordWrap
            font.pointSize: ScreenTools.smallFontPointSize
            text:           qsTr("Work in progress, be careful!")
        }

        Repeater {
            model: [ missionItem.gridAngle, missionItem.gridSpacing, missionItem.gridAltitude ]

            Item {
                anchors.left:   parent.left
                anchors.right:  parent.right
                height:         textField.height

                QGCLabel {
                    anchors.baseline:   textField.baseline
                    anchors.left:       parent.left
                    text:               modelData.name
                }

                FactTextField {
                    id:             textField
                    anchors.right:  parent.right
                    width:          _editFieldWidth
                    showUnits:      true
                    fact:           modelData
                }
            }
        }

        QGCCheckBox {
            anchors.left:   parent.left
            text:           qsTr("Relative altitude")
            checked:        missionItem.gridAltitudeRelative
            onClicked:      missionItem.gridAltitudeRelative = checked
        }

        QGCLabel { text: qsTr("Camera:") }

        Rectangle {
            anchors.left:   parent.left
            anchors.right:  parent.right
            height:         1
            color:          qgcPal.text
        }

        Row {
            spacing: ScreenTools.defaultFontPixelWidth

            QGCRadioButton {
                id:             cameraOrientationLandscape
                text:           "Landscape"
                checked:        true
                exclusiveGroup: cameraOrientationGroup
            }

            QGCRadioButton {
                id:             cameraOrientationPortrait
                text:           "Portrait"
                exclusiveGroup: cameraOrientationGroup
            }
        }

        Grid {
            columns: 2
            spacing: ScreenTools.defaultFontPixelWidth
            verticalItemAlignment: Grid.AlignVCenter

            QGCCheckBox {
                id:         cameraTrigger
                text:       qsTr("Trigger:")
                checked:    missionItem.cameraTrigger
                onClicked:  missionItem.cameraTrigger = checked
            }

            FactTextField {
                width:      _editFieldWidth
                showUnits:  true
                fact:       missionItem.cameraTriggerDistance
                enabled:    missionItem.cameraTrigger
            }

            QGCLabel { text: qsTr("Focal length:") }
            QGCTextField {
                id:         focalLengthField
                unitsLabel: "mm"
                showUnits:  true
                text:       _cameraInfoCanonSX260.focalLength.toString()

                onEditingFinished: recalcFromCameraValues()
            }

            QGCLabel { text: qsTr("Sensor Width:") }
            QGCTextField {
                id:         sensorWidthField
                unitsLabel: "mm"
                showUnits:  true
                text:       _cameraInfoCanonSX260.sensorWidth.toString()

                onEditingFinished: recalcFromCameraValues()
            }

            QGCLabel { text: qsTr("Sensor height:") }
            QGCTextField {
                id:         sensorHeightField
                unitsLabel: "mm"
                showUnits:  true
                text:       _cameraInfoCanonSX260.sensorHeight.toString()

                onEditingFinished: recalcFromCameraValues()
            }

            QGCLabel { text: qsTr("Image overlap:") }
            QGCTextField {
                id:         imageOverlapField
                unitsLabel: "%"
                showUnits:  true
                text:       "0"

                onEditingFinished: recalcFromCameraValues()
            }
        }

        QGCLabel { text: qsTr("Polygon:") }

        Rectangle {
            anchors.left:   parent.left
            anchors.right:  parent.right
            height:         1
            color:          qgcPal.text
        }

        Row {
            spacing: ScreenTools.defaultFontPixelWidth

            QGCButton {
                text:       editorMap.polygonDraw.drawingPolygon ? qsTr("Finish Draw") : qsTr("Draw")
                visible:    !editorMap.polygonDraw.adjustingPolygon
                enabled:    ((editorMap.polygonDraw.drawingPolygon && editorMap.polygonDraw.polygonReady) || !editorMap.polygonDraw.drawingPolygon)

                onClicked: {
                    if (editorMap.polygonDraw.drawingPolygon) {
                        editorMap.polygonDraw.finishCapturePolygon()
                    } else {
                        editorMap.polygonDraw.startCapturePolygon()
                    }
                }
            }

            QGCButton {
                text:       editorMap.polygonDraw.adjustingPolygon ? qsTr("Finish Adjust") : qsTr("Adjust")
                visible:    missionItem.polygonPath.length > 0 && !editorMap.polygonDraw.drawingPolygon

                onClicked: {
                    if (editorMap.polygonDraw.adjustingPolygon) {
                        editorMap.polygonDraw.finishAdjustPolygon()
                    } else {
                        editorMap.polygonDraw.startAdjustPolygon(missionItem.polygonPath)
                    }
                }
            }
        }

        QGCLabel { text: qsTr("Statistics:") }

        Rectangle {
            anchors.left:   parent.left
            anchors.right:  parent.right
            height:         1
            color:          qgcPal.text
        }

        Grid {
            columns: 2
            spacing: ScreenTools.defaultFontPixelWidth

            QGCLabel { text: qsTr("Survey area:") }
            QGCLabel { text: QGroundControl.squareMetersToAppSettingsAreaUnits(missionItem.coveredArea).toFixed(2) + " " + QGroundControl.appSettingsAreaUnitsString }

            QGCLabel { text: qsTr("# shots:") }
            QGCLabel { text: missionItem.cameraShots }
        }
    }
}
