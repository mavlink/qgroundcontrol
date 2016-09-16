import QtQuick                  2.2
import QtQuick.Controls         1.2
import QtQuick.Dialogs          1.2

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

    // The following properties must be available up the hierarchy chain
    //property real   availableWidth    ///< Width for control
    //property var    missionItem       ///< Mission Item for editor

    property real _margin: ScreenTools.defaultFontPixelWidth / 2

    property int cameraIndex: 1

    ListModel {
           id: cameraModelList
           ListElement {
               text:           qsTr("Custom")
               sensorWidth:    0
               sensorHeight:   0
               imageWidth:     0
               imageHeight:    0
               focalLength:    0
           }
           ListElement {
               text:           qsTr("Sony ILCE-QX1") //http://www.sony.co.uk/electronics/interchangeable-lens-cameras/ilce-qx1-body-kit/specifications
               sensorWidth:    23.2                  //http://www.sony.com/electronics/camera-lenses/sel16f28/specifications
               sensorHeight:   15.4
               imageWidth:     5456
               imageHeight:    3632
               focalLength:    16
           }
           ListElement {
               text:           qsTr("Canon S100 PowerShot")
               sensorWidth:    7.6
               sensorHeight:   5.7
               imageWidth:     4000
               imageHeight:    3000
               focalLength:    5.2
           }
           ListElement {
               text:           qsTr("Canon SX260 HS PowerShot")
               sensorWidth:    6.17
               sensorHeight:   4.55
               imageWidth:     4000
               imageHeight:    3000
               focalLength:    4.5
           }
           ListElement {
               text:           qsTr("Canon EOS-M 22mm")
               sensorWidth:    23.5
               sensorHeight:   15.6
               imageWidth:     5184
               imageHeight:    3456
               focalLength:    14.36
           }
   }

    function recalcFromCameraValues() {
        var focalLength = cameraModelList.get(cameraIndex).focalLength
        var sensorWidth = cameraModelList.get(cameraIndex).sensorWidth
        var sensorHeight = cameraModelList.get(cameraIndex).sensorHeight
        var imageWidth = cameraModelList.get(cameraIndex).imageWidth
        var imageHeight = cameraModelList.get(cameraIndex).imageHeight

        var gsd = Number(gsdField.text)
        var frontalOverlap = Number(frontalOverlapField.text)
        var sideOverlap = Number(sideOverlapField.text)

        if (focalLength <= 0.0 || sensorWidth <= 0.0 || sensorHeight <= 0.0 || imageWidth < 0 || imageHeight < 0 || gsd < 0.0 || frontalOverlap < 0 || sideOverlap < 0) {
            missionItem.gridAltitude.rawValue = 0
            missionItem.gridSpacing.rawValue = 0
            missionItem.cameraTriggerDistance.rawValue = 0
            return
        }

        var altitude
        var imageSizeSideGround //size in side (non flying) direction of the image on the ground
        var imageSizeFrontGround //size in front (flying) direction of the image on the ground
        var gridSpacing
        var cameraTriggerDistance

        altitude = (imageWidth * gsd * focalLength) / (sensorWidth * 100)

        if (cameraOrientationLandscape.checked) {
            imageSizeSideGround = (imageWidth * gsd) / 100
            imageSizeFrontGround = (imageHeight * gsd) / 100
        } else {
            imageSizeSideGround = (imageHeight * gsd) / 100
            imageSizeFrontGround = (imageWidth * gsd) / 100
        }

        gridSpacing = imageSizeSideGround * ( (100-sideOverlap) / 100 )
        cameraTriggerDistance = imageSizeFrontGround * ( (100-frontalOverlap) / 100 )

        missionItem.gridAltitude.rawValue = altitude
        missionItem.gridSpacing.rawValue = gridSpacing
        missionItem.cameraTriggerDistance.rawValue = cameraTriggerDistance
    }

    function recalcFromMissionValues() {
        var focalLength = cameraModelList.get(cameraIndex).focalLength
        var sensorWidth = cameraModelList.get(cameraIndex).sensorWidth
        var sensorHeight = cameraModelList.get(cameraIndex).sensorHeight
        var imageWidth = cameraModelList.get(cameraIndex).imageWidth
        var imageHeight = cameraModelList.get(cameraIndex).imageHeight

        var altitude = missionItem.gridAltitude.rawValue
        var gridSpacing = missionItem.gridSpacing.rawValue
        var cameraTriggerDistance = missionItem.cameraTriggerDistance.rawValue

        if (focalLength <= 0.0 || sensorWidth <= 0.0 || sensorHeight <= 0.0 || imageWidth < 0 || imageHeight < 0 || altitude < 0.0 || gridSpacing < 0.0 || cameraTriggerDistance < 0.0) {
            gsdField.text = "0.0"
            sideOverlapField.text = "0"
            frontalOverlapField.text = "0"
            return
        }

        var gsd
        var imageSizeSideGround //size in side (non flying) direction of the image on the ground
        var imageSizeFrontGround //size in front (flying) direction of the image on the ground

        gsd = (altitude * sensorWidth * 100) / (imageWidth * focalLength)

        if (cameraOrientationLandscape.checked) {
            imageSizeSideGround = (imageWidth * gsd) / 100
            imageSizeFrontGround = (imageHeight * gsd) / 100
        } else {
            imageSizeSideGround = (imageHeight * gsd) / 100
            imageSizeFrontGround = (imageWidth * gsd) / 100
        }

        var sideOverlap = (imageSizeSideGround == 0 ? 0 : 100 - (gridSpacing*100 / imageSizeSideGround))
        var frontOverlap = (imageSizeFrontGround == 0 ? 0 : 100 - (cameraTriggerDistance*100 / imageSizeFrontGround))

        gsdField.text = gsd.toFixed(1)
        sideOverlapField.text = sideOverlap.toFixed(0)
        frontalOverlapField.text = frontOverlap.toFixed(0)
    }

    function polygonCaptureStarted() {
    	missionItem.clearPolygon()
    }

    function polygonCaptureFinished(coordinates) {
        for (var i=0; i<coordinates.length; i++) {
            missionItem.addPolygonCoordinate(coordinates[i])
        }
    }

    function polygonAdjustVertex(vertexIndex, vertexCoordinate) {
        missionItem.adjustPolygonCoordinate(vertexIndex, vertexCoordinate)
    }

    function polygonAdjustStarted() { }
    function polygonAdjustFinished() { }

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    ExclusiveGroup {
        id:                 cameraOrientationGroup
        onCurrentChanged:   {
            recalcFromMissionValues()
        }
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
            model: [ missionItem.gridAngle, missionItem.gridSpacing, missionItem.gridAltitude, missionItem.turnaroundDist ]

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
                    onEditingFinished: recalcFromMissionValues()
                    validator:      DoubleValidator{bottom:0.0; decimals:2}
                }
            }
        }

        QGCCheckBox {
            anchors.left:   parent.left
            text:           qsTr("Relative altitude")
            checked:        missionItem.gridAltitudeRelative
            onClicked:      missionItem.gridAltitudeRelative = checked
        }

        Grid {
            columns: 2
            columnSpacing:  ScreenTools.defaultFontPixelWidth
            rowSpacing:     _margin
            verticalItemAlignment: Grid.AlignVCenter

            QGCLabel {
                text: qsTr("GSD:")
                width:      _editFieldWidth
            }
            QGCTextField {
                id:         gsdField
                width:      _editFieldWidth
                unitsLabel: "cm/px"
                showUnits:  true
                onEditingFinished:  recalcFromCameraValues()
                validator:  DoubleValidator{bottom:0.0; decimals:2}
            }

            QGCLabel { text: qsTr("Frontal Overlap:") }
            QGCTextField {
                id:         frontalOverlapField
                width:      _editFieldWidth
                unitsLabel: "%"
                showUnits:  true
                onEditingFinished: recalcFromCameraValues()
                validator:  IntValidator {bottom:0}
            }

            QGCLabel { text: qsTr("Side Overlap:") }
            QGCTextField {
                id:         sideOverlapField
                width:      _editFieldWidth
                unitsLabel: "%"
                showUnits:  true
                onEditingFinished: recalcFromCameraValues()
                validator:  IntValidator {bottom:0}
            }

            Component.onCompleted: recalcFromMissionValues()
        }

        QGCLabel { text: qsTr("Camera:") }

        Rectangle {
            anchors.left:   parent.left
            anchors.right:  parent.right
            height:         1
            color:          qgcPal.text
        }

        Grid {
            columns: 2
            spacing: ScreenTools.defaultFontPixelWidth
            verticalItemAlignment: Grid.AlignVCenter

            QGCRadioButton {
                id:             cameraOrientationLandscape
                width:          _editFieldWidth
                text:           "Landscape"
                checked:        true
                exclusiveGroup: cameraOrientationGroup
            }

            QGCRadioButton {
                id:             cameraOrientationPortrait
                text:           "Portrait"
                exclusiveGroup: cameraOrientationGroup
            }

            QGCCheckBox {
                id:         cameraTrigger
                width:      _editFieldWidth
                text:       qsTr("Trigger:")
                checked:    missionItem.cameraTrigger
                onClicked:  missionItem.cameraTrigger = checked
            }

            FactTextField {
                width:      _editFieldWidth
                showUnits:  true
                fact:       missionItem.cameraTriggerDistance
                enabled:    missionItem.cameraTrigger
                onEditingFinished: recalcFromMissionValues()
                validator:  DoubleValidator{bottom:0.0; decimals:2}
            }
        }

        Component {
            id: cameraFields

            QGCViewDialog {

                Column {
                    id:                 dialogColumn
                    anchors.margins:    _margin
                    anchors.top:        parent.top
                    anchors.left:       parent.left
                    anchors.right:      parent.right
                    spacing:            _margin * 5

                    Row {
                        spacing: ScreenTools.defaultFontPixelWidth

                        QGCLabel {
                            id:                 selectCameraModelText
                            text:               qsTr("Select Camera Model:")
                        }

                        QGCComboBox {
                            id:                 cameraModelCombo
                            model:              cameraModelList
                            width:              dialogColumn.width - selectCameraModelText.width - ScreenTools.defaultFontPixelWidth

                            onActivated: {
                                cameraIndex = index
                            }

                            Component.onCompleted: {
                                var index = cameraIndex
                                if (index === -1) {
                                    console.warn("Active camera model name not in combo", cameraIndex)
                                } else {
                                    cameraModelCombo.currentIndex = index
                                }
                            }
                        }
                    }

                    Grid {
                        columns: 2
                        spacing: ScreenTools.defaultFontPixelWidth
                        verticalItemAlignment: Grid.AlignVCenter

                        QGCLabel { text: qsTr("Sensor Width:") }
                        QGCTextField {
                            id:                 sensorWidthField
                            unitsLabel:         "mm"
                            showUnits:          true
                            text:               cameraModelList.get(cameraIndex).sensorWidth.toFixed(2)
                            readOnly:           cameraIndex != 0
                            enabled:            cameraIndex == 0
                            validator:          DoubleValidator{bottom:0.0; decimals:2}
                            onEditingFinished:  {
                                if (cameraIndex == 0) {
                                    cameraModelList.setProperty(cameraIndex, "sensorWidth", Number(text))
                                }
                            }
                        }

                        QGCLabel { text: qsTr("Sensor Height:") }
                        QGCTextField {
                            id:                 sensorHeightField
                            unitsLabel:         "mm"
                            showUnits:          true
                            text:               cameraModelList.get(cameraIndex).sensorHeight.toFixed(2)
                            readOnly:           cameraIndex != 0
                            enabled:            cameraIndex == 0
                            validator:          DoubleValidator{bottom:0.0; decimals:2}
                            onEditingFinished:  {
                                if (cameraIndex == 0) {
                                    cameraModelList.setProperty(cameraIndex, "sensorHeight", Number(text))
                                }
                            }
                        }

                        QGCLabel { text: qsTr("Image Width:") }
                        QGCTextField {
                            id:                 imageWidthField
                            unitsLabel:         "px"
                            showUnits:          true
                            text:               cameraModelList.get(cameraIndex).imageWidth.toFixed(0)
                            readOnly:           cameraIndex != 0
                            enabled:            cameraIndex == 0
                            validator:          IntValidator {bottom:0}
                            onEditingFinished:  {
                                if (cameraIndex == 0) {
                                    cameraModelList.setProperty(cameraIndex, "imageWidth", Number(text))
                                }
                            }
                        }

                        QGCLabel { text: qsTr("Image Height:") }
                        QGCTextField {
                            id:                 imageHeightField
                            unitsLabel:         "px"
                            showUnits:          true
                            text:               cameraModelList.get(cameraIndex).imageHeight.toFixed(0)
                            readOnly:           cameraIndex != 0
                            enabled:            cameraIndex == 0
                            validator:          IntValidator {bottom:0}
                            onEditingFinished:  {
                                if (cameraIndex == 0) {
                                    cameraModelList.setProperty(cameraIndex, "imageHeight", Number(text))
                                }
                            }
                        }

                        QGCLabel { text: qsTr("Focal Length:") }
                        QGCTextField {
                            id:                 focalLengthField
                            unitsLabel:         "mm"
                            showUnits:          true
                            text:               cameraModelList.get(cameraIndex).focalLength.toFixed(2)
                            readOnly:           cameraIndex != 0
                            enabled:            cameraIndex == 0
                            validator:          DoubleValidator{bottom:0.0; decimals:2}
                            onEditingFinished:  {
                                if (cameraIndex == 0) {
                                    cameraModelList.setProperty(cameraIndex, "focalLength", Number(text))
                                }
                            }
                        }
                    }
                }

                function accept() {
                    hideDialog()
                    recalcFromCameraValues()
                }
            }//QGCViewDialog
        }//Component

        Column {
            spacing: ScreenTools.defaultFontPixelHeight*0.01

            Row {
                spacing: ScreenTools.defaultFontPixelWidth

                QGCLabel { text: qsTr("Model:") }
                QGCLabel { text: cameraModelList.get(cameraIndex).text }
            }

            Grid {
                columns: 2
                columnSpacing: ScreenTools.defaultFontPixelWidth
                rowSpacing: ScreenTools.defaultFontPixelHeight*0.01

                QGCLabel {
                    text: qsTr("Sensor Size:")
                    width: _editFieldWidth
                }
                QGCLabel {
                    text: cameraModelList.get(cameraIndex).sensorWidth.toFixed(2) + qsTr(" x ") +  cameraModelList.get(cameraIndex).sensorHeight.toFixed(2)
                    width: _editFieldWidth
                }

                QGCLabel { text: qsTr("Image Size:") }
                QGCLabel { text: cameraModelList.get(cameraIndex).imageWidth.toFixed(0) + qsTr(" x ") +  cameraModelList.get(cameraIndex).imageHeight.toFixed(0) }

                QGCLabel { text: qsTr("Focal length:") }
                QGCLabel { text: cameraModelList.get(cameraIndex).focalLength.toFixed(2) }
            }
        }

        QGCButton {
            id:         cameraModelChange
            text:       qsTr("Change")

            onClicked: {
                qgcView.showDialog(cameraFields, qsTr("Set Camera Model"), qgcView.showDialogDefaultWidth, StandardButton.Save)
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
                        editorMap.polygonDraw.startCapturePolygon(_root)
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
                        editorMap.polygonDraw.startAdjustPolygon(_root, missionItem.polygonPath)
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
