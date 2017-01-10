import QtQuick          2.2
import QtQuick.Controls 1.2
import QtQuick.Dialogs  1.2
import QtQuick.Layouts  1.2

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

    property real   _margin:        ScreenTools.defaultFontPixelWidth / 2
    property int    _cameraIndex:   1

    readonly property int _gridTypeManual:          0
    readonly property int _gridTypeCustomCamera:    1
    readonly property int _gridTypeCamera:          2

    ListModel {
        id: cameraModelList

        Component.onCompleted: {
            cameraModelList.setProperty(_gridTypeCustomCamera, "sensorWidth", missionItem.cameraSensorWidth.rawValue)
            cameraModelList.setProperty(_gridTypeCustomCamera, "sensorHeight", missionItem.cameraSensorHeight.rawValue)
            cameraModelList.setProperty(_gridTypeCustomCamera, "imageWidth", missionItem.cameraResolutionWidth.rawValue)
            cameraModelList.setProperty(_gridTypeCustomCamera, "imageHeight", missionItem.cameraResolutionHeight.rawValue)
            cameraModelList.setProperty(_gridTypeCustomCamera, "focalLength", missionItem.cameraFocalLength.rawValue)
        }

        ListElement {
            text:           qsTr("Manual Grid (no camera specs)")
        }
        ListElement {
            text:           qsTr("Custom Camera Grid")
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
            sensorWidth:    22.3
            sensorHeight:   14.9
            imageWidth:     5184
            imageHeight:    3456
            focalLength:    22
        }
        ListElement {
            text:           qsTr("Sony a6000 16mm") //http://www.sony.co.uk/electronics/interchangeable-lens-cameras/ilce-6000-body-kit#product_details_default
            sensorWidth:    23.5
            sensorHeight:   15.6
            imageWidth:     6000
            imageHeight:    4000
            focalLength:    16
        }
    }

    function recalcFromCameraValues() {
        var focalLength = missionItem.cameraFocalLength.rawValue
        var sensorWidth = missionItem.cameraSensorWidth.rawValue
        var sensorHeight = missionItem.cameraSensorHeight.rawValue
        var imageWidth = missionItem.cameraResolutionWidth.rawValue
        var imageHeight = missionItem.cameraResolutionHeight.rawValue

        var altitude = missionItem.gridAltitude.rawValue
        var groundResolution = missionItem.groundResolution.rawValue
        var frontalOverlap = missionItem.frontalOverlap.rawValue
        var sideOverlap = missionItem.sideOverlap.rawValue

        if (focalLength <= 0 || sensorWidth <= 0 || sensorHeight <= 0 || imageWidth <= 0 || imageHeight <= 0 || groundResolution <= 0) {
            return
        }

        var imageSizeSideGround     //size in side (non flying) direction of the image on the ground
        var imageSizeFrontGround    //size in front (flying) direction of the image on the ground
        var gridSpacing
        var cameraTriggerDistance

        if (missionItem.fixedValueIsAltitude) {
            groundResolution = (altitude * sensorWidth * 100) /  (imageWidth * focalLength)
        } else {
            altitude = (imageWidth * groundResolution * focalLength) / (sensorWidth * 100)
        }

        if (cameraOrientationLandscape.checked) {
            imageSizeSideGround = (imageWidth * groundResolution) / 100
            imageSizeFrontGround = (imageHeight * groundResolution) / 100
        } else {
            imageSizeSideGround = (imageHeight * groundResolution) / 100
            imageSizeFrontGround = (imageWidth * groundResolution) / 100
        }

        gridSpacing = imageSizeSideGround * ( (100-sideOverlap) / 100 )
        cameraTriggerDistance = imageSizeFrontGround * ( (100-frontalOverlap) / 100 )

        if (missionItem.fixedValueIsAltitude) {
            missionItem.groundResolution.rawValue = groundResolution
        } else {
            missionItem.gridAltitude.rawValue = altitude
        }
        missionItem.gridSpacing.rawValue = gridSpacing
        missionItem.cameraTriggerDistance.rawValue = cameraTriggerDistance
    }

    /*
    function recalcFromMissionValues() {
        var focalLength = missionItem.cameraFocalLength.rawValue
        var sensorWidth = missionItem.cameraSensorWidth.rawValue
        var sensorHeight = missionItem.cameraSensorHeight.rawValue
        var imageWidth = missionItem.cameraResolutionWidth.rawValue
        var imageHeight = missionItem.cameraResolutionHeight.rawValue

        var altitude = missionItem.gridAltitude.rawValue
        var gridSpacing = missionItem.gridSpacing.rawValue
        var cameraTriggerDistance = missionItem.cameraTriggerDistance.rawValue

        if (focalLength <= 0.0 || sensorWidth <= 0.0 || sensorHeight <= 0.0 || imageWidth < 0 || imageHeight < 0 || altitude < 0.0 || gridSpacing < 0.0 || cameraTriggerDistance < 0.0) {
            missionItem.groundResolution.rawValue = 0
            missionItem.sideOverlap = 0
            missionItem.frontalOverlap = 0
            return
        }

        var groundResolution
        var imageSizeSideGround     //size in side (non flying) direction of the image on the ground
        var imageSizeFrontGround    //size in front (flying) direction of the image on the ground

        groundResolution = (altitude * sensorWidth * 100) / (imageWidth * focalLength)

        if (cameraOrientationLandscape.checked) {
            imageSizeSideGround = (imageWidth * gsd) / 100
            imageSizeFrontGround = (imageHeight * gsd) / 100
        } else {
            imageSizeSideGround = (imageHeight * gsd) / 100
            imageSizeFrontGround = (imageWidth * gsd) / 100
        }

        var sideOverlap = (imageSizeSideGround == 0 ? 0 : 100 - (gridSpacing*100 / imageSizeSideGround))
        var frontOverlap = (imageSizeFrontGround == 0 ? 0 : 100 - (cameraTriggerDistance*100 / imageSizeFrontGround))

        missionItem.groundResolution.rawValue = groundResolution
        missionItem.sideOverlap.rawValue = sideOverlap
        missionItem.frontalOverlap.rawValue = frontOverlap
    }
    */

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

    property bool _noCameraValueRecalc: false   ///< Prevents uneeded recalcs

    Connections {
        target: missionItem

        onCameraValueChanged: {
            if (gridTypeCombo.currentIndex >= _gridTypeCustomCamera && !_noCameraValueRecalc) {
                recalcFromCameraValues()
            }
        }
    }

    Connections {
        target: missionItem.gridAltitude

        onValueChanged: {
            if (gridTypeCombo.currentIndex >= _gridTypeCustomCamera && missionItem.fixedValueIsAltitude && !_noCameraValueRecalc) {
                recalcFromCameraValues()
            }
        }
    }

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    ExclusiveGroup {
        id: cameraOrientationGroup

        onCurrentChanged: {
            if (gridTypeCombo.currentIndex >= _gridTypeCustomCamera) {
                recalcFromCameraValues()
            }
        }
    }

    ExclusiveGroup { id: fixedValueGroup }

    Column {
        id:                 editorColumn
        anchors.margins:    _margin
        anchors.top:        parent.top
        anchors.left:       parent.left
        anchors.right:      parent.right
        spacing:            _margin

        QGCLabel {
            anchors.left:   parent.left
            anchors.right:  parent.right
            wrapMode:       Text.WordWrap
            font.pointSize: ScreenTools.smallFontPointSize
            text:           gridTypeCombo.currentIndex == 0 ?
                                qsTr("Create a flight path which covers a polygonal area by specifying all grid parameters.") :
                                qsTr("Create a flight path which fully covers a polygonal area using camera specifications.")
        }

        QGCLabel { text: qsTr("Camera:") }

        Rectangle {
            anchors.left:   parent.left
            anchors.right:  parent.right
            height:         1
            color:          qgcPal.text
        }

        QGCComboBox {
            id:             gridTypeCombo
            anchors.left:   parent.left
            anchors.right:  parent.right
            model:          cameraModelList
            currentIndex:   -1

            Component.onCompleted: {
                if (missionItem.manualGrid) {
                    gridTypeCombo.currentIndex = _gridTypeManual
                } else {
                    var index = gridTypeCombo.find(missionItem.camera)
                    if (index == -1) {
                        console.log("Couldn't find camera", missionItem.camera)
                        gridTypeCombo.currentIndex = _gridTypeManual
                    } else {
                        gridTypeCombo.currentIndex = index
                    }
                }
            }

            onActivated: {
                if (index == _gridTypeManual) {
                    missionItem.manualGrid = true
                } else {
                    missionItem.manualGrid = false
                    missionItem.camera = gridTypeCombo.textAt(index)
                    _noCameraValueRecalc = true
                    missionItem.cameraSensorWidth.rawValue = cameraModelList.get(index).sensorWidth
                    missionItem.cameraSensorHeight.rawValue = cameraModelList.get(index).sensorHeight
                    missionItem.cameraResolutionWidth.rawValue = cameraModelList.get(index).imageWidth
                    missionItem.cameraResolutionHeight.rawValue = cameraModelList.get(index).imageHeight
                    missionItem.cameraFocalLength.rawValue = cameraModelList.get(index).focalLength
                    _noCameraValueRecalc = false
                    recalcFromCameraValues()
                }
            }
        }

        // Camera based grid ui
        Column {
            anchors.left:   parent.left
            anchors.right:  parent.right
            spacing:        _margin
            visible:        gridTypeCombo.currentIndex != _gridTypeManual

            Row {
                spacing: _margin

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
            }

            Column {
                anchors.left:   parent.left
                anchors.right:  parent.right
                spacing:        _margin
                visible:        gridTypeCombo.currentIndex == _gridTypeCustomCamera

                GridLayout {
                    columns:        3
                    columnSpacing:  _margin
                    rowSpacing:     _margin

                    property real _fieldWidth: ScreenTools.defaultFontPixelWidth * 10

                    QGCLabel { }
                    QGCLabel { text: qsTr("Width") }
                    QGCLabel { text: qsTr("Height") }

                    QGCLabel { text: qsTr("Sensor:") }
                    FactTextField {
                        Layout.preferredWidth:  parent._fieldWidth
                        fact:                   missionItem.cameraSensorWidth
                    }
                    FactTextField {
                        Layout.preferredWidth:  parent._fieldWidth
                        fact:                   missionItem.cameraSensorHeight
                    }

                    QGCLabel { text: qsTr("Image:") }
                    FactTextField {
                        Layout.preferredWidth:  parent._fieldWidth
                        fact:                   missionItem.cameraResolutionWidth
                    }
                    FactTextField {
                        Layout.preferredWidth:  parent._fieldWidth
                        fact:                   missionItem.cameraResolutionHeight
                    }
                }

                FactTextFieldRow {
                    spacing: _margin
                    fact:       missionItem.cameraFocalLength
                }
            } // Column - custom camera

            QGCLabel { text: qsTr("Image Overlap") }

            Row {
                spacing:        _margin

                Item {
                    width:  ScreenTools.defaultFontPixelWidth * 2
                    height: 1
                }

                QGCLabel {
                    anchors.baseline:   frontalOverlapField.baseline
                    text:               qsTr("Frontal:")
                }

                FactTextField {
                    id:     frontalOverlapField
                    width:  ScreenTools.defaultFontPixelWidth * 7
                    fact:   missionItem.frontalOverlap
                }

                QGCLabel {
                    anchors.baseline:   frontalOverlapField.baseline
                    text:               qsTr("Side:")
                }

                FactTextField {
                    width:  frontalOverlapField.width
                    fact:   missionItem.sideOverlap
                }
            }

            QGCLabel { text: qsTr("Grid:") }

            Rectangle {
                anchors.left:   parent.left
                anchors.right:  parent.right
                height:         1
                color:          qgcPal.text
            }

            FactTextFieldGrid {
                anchors.left:   parent.left
                anchors.right:  parent.right
                columnSpacing:  _margin
                rowSpacing:     _margin
                factList:       [ missionItem.gridAngle, missionItem.turnaroundDist ]
            }

            QGCLabel {
                anchors.left:   parent.left
                anchors.right:  parent.right
                wrapMode:       Text.WordWrap
                font.pointSize: ScreenTools.smallFontPointSize
                text:           qsTr("Which value would you like to keep constant as you adjust other settings:")
            }

            RowLayout {
                anchors.left:   parent.left
                anchors.right:  parent.right
                spacing:        _margin

                QGCRadioButton {
                    id:                 fixedAltitudeRadio
                    anchors.baseline:   gridAltitudeField.baseline
                    text:               qsTr("Altitude:")
                    checked:            missionItem.fixedValueIsAltitude
                    exclusiveGroup:     fixedValueGroup
                    onClicked:          missionItem.fixedValueIsAltitude = true
                }

                FactTextField {
                    id:                 gridAltitudeField
                    Layout.fillWidth:   true
                    fact:               missionItem.gridAltitude
                    enabled:            fixedAltitudeRadio.checked
                }
            }

            RowLayout {
                anchors.left:   parent.left
                anchors.right:  parent.right
                spacing:        _margin

                QGCRadioButton {
                    id:                 fixedGroundResolutionRadio
                    anchors.baseline:   groundResolutionField.baseline
                    text:               qsTr("Ground res:")
                    checked:            !missionItem.fixedValueIsAltitude
                    exclusiveGroup:     fixedValueGroup
                    onClicked:          missionItem.fixedValueIsAltitude = false
                }

                FactTextField {
                    id:                 groundResolutionField
                    Layout.fillWidth:   true
                    fact:               missionItem.groundResolution
                    enabled:            fixedGroundResolutionRadio.checked
                }
            }
        }

        // Manual grid ui
        Column {
            anchors.left:   parent.left
            anchors.right:  parent.right
            spacing:        _margin
            visible:        gridTypeCombo.currentIndex == _gridTypeManual

            QGCLabel { text: qsTr("Grid:") }

            Rectangle {
                anchors.left:   parent.left
                anchors.right:  parent.right
                height:         1
                color:          qgcPal.text
            }

            FactTextFieldGrid {
                anchors.left:   parent.left
                anchors.right:  parent.right
                columnSpacing:  _margin
                rowSpacing:     _margin
                factList:       [ missionItem.gridAngle, missionItem.gridSpacing, missionItem.gridAltitude, missionItem.turnaroundDist ]
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

            RowLayout {
                anchors.left:   parent.left
                anchors.right:  parent.right
                spacing:        _margin

                QGCCheckBox {
                    id:                 cameraTrigger
                    anchors.baseline:   cameraTriggerDistanceField.baseline
                    text:               qsTr("Trigger Distance:")
                    checked:            missionItem.cameraTrigger
                    onClicked:          missionItem.cameraTrigger = checked
                }

                FactTextField {
                    id:                 cameraTriggerDistanceField
                    Layout.fillWidth:   true
                    fact:               missionItem.cameraTriggerDistance
                    enabled:            missionItem.cameraTrigger
                }
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
            columns:        2
            columnSpacing:  ScreenTools.defaultFontPixelWidth

            QGCLabel { text: qsTr("Survey area:") }
            QGCLabel { text: QGroundControl.squareMetersToAppSettingsAreaUnits(missionItem.coveredArea).toFixed(2) + " " + QGroundControl.appSettingsAreaUnitsString }

            QGCLabel { text: qsTr("# shots:") }
            QGCLabel { text: missionItem.cameraShots }

            QGCLabel { text: qsTr("Shot interval:") }
            QGCLabel { text: missionItem.timeBetweenShots.toFixed(1) + " " + qsTr("secs")}
        }
    }
}
