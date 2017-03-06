import QtQuick          2.3
import QtQuick.Controls 1.2
import QtQuick.Dialogs  1.2
import QtQuick.Layouts  1.2

import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Vehicle       1.0
import QGroundControl.Controls      1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Palette       1.0
import QGroundControl.FlightMap     1.0

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

    property real   _margin:            ScreenTools.defaultFontPixelWidth * 0.25
    property int    _cameraIndex:       1
    property real   _fieldWidth:        ScreenTools.defaultFontPixelWidth * 10.5
    property var    _cameraList:        [ qsTr("Manual Grid (no camera specs)"), qsTr("Custom Camera Grid") ]
    property var    _vehicle:           QGroundControl.multiVehicleManager.activeVehicle ? QGroundControl.multiVehicleManager.activeVehicle : QGroundControl.multiVehicleManager.offlineEditingVehicle
    property var    _vehicleCameraList: _vehicle.cameraList

    readonly property int _gridTypeManual:          0
    readonly property int _gridTypeCustomCamera:    1
    readonly property int _gridTypeCamera:          2

    Component.onCompleted: {
        for (var i=0; i<_vehicle.cameraList.length; i++) {
            _cameraList.push(_vehicle.cameraList[i].name)
        }
        gridTypeCombo.model = _cameraList
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

    function recalcFromCameraValues() {
        var focalLength     = missionItem.cameraFocalLength.rawValue
        var sensorWidth     = missionItem.cameraSensorWidth.rawValue
        var sensorHeight    = missionItem.cameraSensorHeight.rawValue
        var imageWidth      = missionItem.cameraResolutionWidth.rawValue
        var imageHeight     = missionItem.cameraResolutionHeight.rawValue

        var altitude        = missionItem.gridAltitude.rawValue
        var groundResolution= missionItem.groundResolution.rawValue
        var frontalOverlap  = missionItem.frontalOverlap.rawValue
        var sideOverlap     = missionItem.sideOverlap.rawValue

        if (focalLength <= 0 || sensorWidth <= 0 || sensorHeight <= 0 || imageWidth <= 0 || imageHeight <= 0 || groundResolution <= 0) {
            return
        }

        var imageSizeSideGround     //size in side (non flying) direction of the image on the ground
        var imageSizeFrontGround    //size in front (flying) direction of the image on the ground
        var gridSpacing
        var cameraTriggerDistance

        if (missionItem.fixedValueIsAltitude) {
            groundResolution = (altitude * sensorWidth * 100) / (imageWidth * focalLength)
        } else {
            altitude = (imageWidth * groundResolution * focalLength) / (sensorWidth * 100)
        }

        if (cameraOrientationLandscape.checked) {
            imageSizeSideGround  = (imageWidth  * groundResolution) / 100
            imageSizeFrontGround = (imageHeight * groundResolution) / 100
        } else {
            imageSizeSideGround  = (imageHeight * groundResolution) / 100
            imageSizeFrontGround = (imageWidth  * groundResolution) / 100
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

        onIsCurrentItemChanged: {
            if (!missionItem.isCurrentItem) {
                polygonEditor.cancelPolygonEdit()
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

        QGCComboBox {
            id:             gridTypeCombo
            anchors.left:   parent.left
            anchors.right:  parent.right
            model:          _cameraList
            currentIndex:   -1

            onActivated: {
                if (index == _gridTypeManual) {
                    missionItem.manualGrid = true
                } else if (index == _gridTypeCustomCamera) {
                    missionItem.manualGrid = false
                    missionItem.camera = gridTypeCombo.textAt(index)
                } else {
                    missionItem.manualGrid = false
                    missionItem.camera = gridTypeCombo.textAt(index)
                    _noCameraValueRecalc = true
                    var listIndex = index - _gridTypeCamera
                    missionItem.cameraSensorWidth.rawValue      = _vehicleCameraList[listIndex].sensorWidth
                    missionItem.cameraSensorHeight.rawValue     = _vehicleCameraList[listIndex].sensorHeight
                    missionItem.cameraResolutionWidth.rawValue  = _vehicleCameraList[listIndex].imageWidth
                    missionItem.cameraResolutionHeight.rawValue = _vehicleCameraList[listIndex].imageHeight
                    missionItem.cameraFocalLength.rawValue      = _vehicleCameraList[listIndex].focalLength
                    _noCameraValueRecalc = false
                    recalcFromCameraValues()
                }
            }
        }

        QGCLabel { text: qsTr("Camera"); visible: gridTypeCombo.currentIndex !== _gridTypeManual}

        Rectangle {
            anchors.left:   parent.left
            anchors.right:  parent.right
            height:         1
            color:          qgcPal.text
            visible:        gridTypeCombo.currentIndex !== _gridTypeManual
        }

        // Camera based grid ui
        Column {
            anchors.left:   parent.left
            anchors.right:  parent.right
            spacing:        _margin
            visible:        gridTypeCombo.currentIndex != _gridTypeManual

            Row {
                spacing: _margin
                anchors.horizontalCenter: parent.horizontalCenter

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
                id:             custCameraCol
                anchors.left:   parent.left
                anchors.right:  parent.right
                spacing:        _margin
                visible:        gridTypeCombo.currentIndex === _gridTypeCustomCamera

                RowLayout {
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    spacing:        _margin
                    Item { Layout.fillWidth: true }
                    QGCLabel {
                        Layout.preferredWidth:  _root._fieldWidth
                        text:                   qsTr("Width")
                    }
                    QGCLabel {
                        Layout.preferredWidth:  _root._fieldWidth
                        text:                   qsTr("Height")
                    }
                }

                RowLayout {
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    spacing:        _margin
                    QGCLabel { text: qsTr("Sensor:"); Layout.fillWidth: true }
                    FactTextField {
                        Layout.preferredWidth:  _root._fieldWidth
                        fact:                   missionItem.cameraSensorWidth
                    }
                    FactTextField {
                        Layout.preferredWidth:  _root._fieldWidth
                        fact:                   missionItem.cameraSensorHeight
                    }
                }

                RowLayout {
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    spacing:        _margin
                    QGCLabel { text: qsTr("Image:"); Layout.fillWidth: true }
                    FactTextField {
                        Layout.preferredWidth:  _root._fieldWidth
                        fact:                   missionItem.cameraResolutionWidth
                    }
                    FactTextField {
                        Layout.preferredWidth:  _root._fieldWidth
                        fact:                   missionItem.cameraResolutionHeight
                    }
                }

                RowLayout {
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    spacing:        _margin
                    QGCLabel {
                        text:                   missionItem.cameraFocalLength.name + ":"
                        Layout.fillWidth:       true
                    }
                    FactTextField {
                        Layout.preferredWidth:  _root._fieldWidth
                        fact:                   missionItem.cameraFocalLength
                    }
                }

            } // Column - custom camera

            RowLayout {
                anchors.left:   parent.left
                anchors.right:  parent.right
                spacing:        _margin
                Item { Layout.fillWidth: true }
                QGCLabel {
                    Layout.preferredWidth:  _root._fieldWidth
                    text:                   qsTr("Frontal")
                }
                QGCLabel {
                    Layout.preferredWidth:  _root._fieldWidth
                    text:                   qsTr("Side")
                }
            }

            RowLayout {
                anchors.left:   parent.left
                anchors.right:  parent.right
                spacing:        _margin
                QGCLabel { text: qsTr("Overlap:"); Layout.fillWidth: true }
                FactTextField {
                    Layout.preferredWidth:  _root._fieldWidth
                    fact:                   missionItem.frontalOverlap
                }
                FactTextField {
                    Layout.preferredWidth:  _root._fieldWidth
                    fact:                   missionItem.sideOverlap
                }
            }

            QGCLabel { text: qsTr("Grid") }

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
                QGCLabel {
                    text:                   missionItem.gridAngle.name + ":"
                    Layout.fillWidth:       true
                    anchors.verticalCenter: parent.verticalCenter
                }
                FactTextField {
                    fact:                   missionItem.gridAngle
                    anchors.verticalCenter: parent.verticalCenter
                    Layout.preferredWidth:  _root._fieldWidth
                }
            }

            RowLayout {
                anchors.left:   parent.left
                anchors.right:  parent.right
                spacing:        _margin
                QGCLabel {
                    text:                   missionItem.turnaroundDist.name + ":"
                    Layout.fillWidth:       true
                    anchors.verticalCenter: parent.verticalCenter
                }
                FactTextField {
                    fact:                   missionItem.turnaroundDist
                    anchors.verticalCenter: parent.verticalCenter
                    Layout.preferredWidth:  _root._fieldWidth
                }
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
                    id:                     fixedAltitudeRadio
                    text:                   qsTr("Altitude:")
                    checked:                missionItem.fixedValueIsAltitude
                    exclusiveGroup:         fixedValueGroup
                    onClicked:              missionItem.fixedValueIsAltitude = true
                    Layout.fillWidth:       true
                    anchors.verticalCenter: parent.verticalCenter
                }

                FactTextField {
                    fact:                   missionItem.gridAltitude
                    enabled:                fixedAltitudeRadio.checked
                    Layout.preferredWidth:  _root._fieldWidth
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            RowLayout {
                anchors.left:   parent.left
                anchors.right:  parent.right
                spacing:        _margin

                QGCRadioButton {
                    id:                     fixedGroundResolutionRadio
                    text:                   qsTr("Ground res:")
                    checked:                !missionItem.fixedValueIsAltitude
                    exclusiveGroup:         fixedValueGroup
                    onClicked:              missionItem.fixedValueIsAltitude = false
                    Layout.fillWidth:       true
                    anchors.verticalCenter: parent.verticalCenter
                }

                FactTextField {
                    fact:                   missionItem.groundResolution
                    enabled:                fixedGroundResolutionRadio.checked
                    Layout.preferredWidth:  _root._fieldWidth
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
        }

        // Manual grid ui
        Column {
            anchors.left:   parent.left
            anchors.right:  parent.right
            spacing:        _margin
            visible:        gridTypeCombo.currentIndex == _gridTypeManual

            QGCLabel { text: qsTr("Grid") }

            Rectangle {
                anchors.left:   parent.left
                anchors.right:  parent.right
                height:         1
                color:          qgcPal.text
            }

            FactTextFieldGrid {
                anchors.left:   parent.left
                anchors.right:  parent.right
                columnSpacing:  _margin * 10
                rowSpacing:     _margin
                factList:       [ missionItem.gridAngle, missionItem.gridSpacing, missionItem.gridAltitude, missionItem.turnaroundDist ]
            }

            QGCCheckBox {
                anchors.left:   parent.left
                text:           qsTr("Relative altitude")
                checked:        missionItem.gridAltitudeRelative
                onClicked:      missionItem.gridAltitudeRelative = checked
            }

            QGCLabel { text: qsTr("Camera") }

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

        QGCLabel { text: qsTr("Polygon") }

        Rectangle {
            anchors.left:   parent.left
            anchors.right:  parent.right
            height:         1
            color:          qgcPal.text
        }

        Row {
            spacing: ScreenTools.defaultFontPixelWidth
            anchors.horizontalCenter: parent.horizontalCenter

            QGCButton {
                width:      _root.width * 0.45
                text:       polygonEditor.drawingPolygon ? qsTr("Finish Draw") : qsTr("Draw")
                visible:    !polygonEditor .adjustingPolygon
                enabled:    ((polygonEditor.drawingPolygon && polygonEditor.polygonReady) || !polygonEditor.drawingPolygon)

                onClicked: {
                    if (polygonEditor.drawingPolygon) {
                        polygonEditor.finishCapturePolygon()
                    } else {
                        polygonEditor.startCapturePolygon()
                    }
                }
            }

            QGCButton {
                width:      _root.width * 0.4
                text:       polygonEditor.adjustingPolygon ? qsTr("Finish Adjust") : qsTr("Adjust")
                visible:    missionItem.polygonPath.length > 0 && !polygonEditor.drawingPolygon

                onClicked: {
                    if (polygonEditor.adjustingPolygon) {
                        polygonEditor.finishAdjustPolygon()
                    } else {
                        polygonEditor.startAdjustPolygon(missionItem.polygonPath)
                    }
                }
            }
        }

        QGCLabel { text: qsTr("Statistics") }

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

            QGCLabel { text: qsTr("Photo count:") }
            QGCLabel { text: missionItem.cameraShots }

            QGCLabel { text: qsTr("Photo interval:") }
            QGCLabel {
                text: {
                    var timeVal = missionItem.timeBetweenShots
                    if(!isFinite(timeVal) || missionItem.cameraShots === 0) {
                        return qsTr("N/A")
                    }
                    return timeVal.toFixed(1) + " " + qsTr("secs")
                }
            }
        }
    }

    PolygonEditor {
        id:             polygonEditor
        map:            editorMap
        callbackObject: parent
    }
}
