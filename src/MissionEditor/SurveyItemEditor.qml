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

    property real   _margin:            ScreenTools.defaultFontPixelWidth / 2
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
        if (missionItem.manualGrid.value) {
            gridTypeCombo.currentIndex = _gridTypeManual
        } else {
            var index = -1
            for (index=0; index<_cameraList.length; index++) {
                if (_cameraList[index] == missionItem.camera.value) {
                    break;
                }
            }
            missionItem.cameraOrientationFixed = false
            if (index == -1) {
                gridTypeCombo.currentIndex = _gridTypeManual
            } else {
                gridTypeCombo.currentIndex = index
                if (index != 1) {
                    // Specific camera is selected
                    missionItem.cameraOrientationFixed = _vehicleCameraList[index - _gridTypeCamera].fixedOrientation
                }
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

        if (missionItem.fixedValueIsAltitude.value) {
            groundResolution = (altitude * sensorWidth * 100) / (imageWidth * focalLength)
        } else {
            altitude = (imageWidth * groundResolution * focalLength) / (sensorWidth * 100)
        }

        if (missionItem.cameraOrientationLandscape.value) {
            imageSizeSideGround  = (imageWidth  * groundResolution) / 100
            imageSizeFrontGround = (imageHeight * groundResolution) / 100
        } else {
            imageSizeSideGround  = (imageHeight * groundResolution) / 100
            imageSizeFrontGround = (imageWidth  * groundResolution) / 100
        }

        gridSpacing = imageSizeSideGround * ( (100-sideOverlap) / 100 )
        cameraTriggerDistance = imageSizeFrontGround * ( (100-frontalOverlap) / 100 )

        if (missionItem.fixedValueIsAltitude.value) {
            missionItem.groundResolution.rawValue = groundResolution
        } else {
            missionItem.gridAltitude.rawValue = altitude
        }
        missionItem.gridSpacing.rawValue = gridSpacing
        missionItem.cameraTriggerDistance.rawValue = cameraTriggerDistance
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

    property bool _noCameraValueRecalc: false   ///< Prevents uneeded recalcs

    Connections {
        target: missionItem.camera

        onValueChanged: {
            if (gridTypeCombo.currentIndex >= _gridTypeCustomCamera && !_noCameraValueRecalc) {
                recalcFromCameraValues()
            }
        }
    }

    Connections {
        target: missionItem.gridAltitude

        onValueChanged: {
            if (gridTypeCombo.currentIndex >= _gridTypeCustomCamera && missionItem.fixedValueIsAltitude.value && !_noCameraValueRecalc) {
                recalcFromCameraValues()
            }
        }
    }

    Connections {
        target: missionItem

        onCameraValueChanged: {
            if (gridTypeCombo.currentIndex >= _gridTypeCustomCamera && !_noCameraValueRecalc) {
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

        SectionHeader {
            text:       qsTr("Camera")
            showSpacer: false
        }

        QGCComboBox {
            id:             gridTypeCombo
            anchors.left:   parent.left
            anchors.right:  parent.right
            model:          _cameraList
            currentIndex:   -1

            onActivated: {
                if (index == _gridTypeManual) {
                    missionItem.manualGrid.value = true
                } else if (index == _gridTypeCustomCamera) {
                    missionItem.manualGrid.value = false
                    missionItem.camera.value = gridTypeCombo.textAt(index)
                    missionItem.cameraOrientationFixed = false
                } else {
                    missionItem.manualGrid.value = false
                    missionItem.camera.value = gridTypeCombo.textAt(index)
                    _noCameraValueRecalc = true
                    var listIndex = index - _gridTypeCamera
                    missionItem.cameraSensorWidth.rawValue          = _vehicleCameraList[listIndex].sensorWidth
                    missionItem.cameraSensorHeight.rawValue         = _vehicleCameraList[listIndex].sensorHeight
                    missionItem.cameraResolutionWidth.rawValue      = _vehicleCameraList[listIndex].imageWidth
                    missionItem.cameraResolutionHeight.rawValue     = _vehicleCameraList[listIndex].imageHeight
                    missionItem.cameraFocalLength.rawValue          = _vehicleCameraList[listIndex].focalLength
                    missionItem.cameraOrientationLandscape.rawValue = _vehicleCameraList[listIndex].landscape ? 1 : 0
                    missionItem.cameraOrientationFixed              = _vehicleCameraList[listIndex].fixedOrientation
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
                spacing:                    _margin
                anchors.horizontalCenter:   parent.horizontalCenter
                visible:                    !missionItem.cameraOrientationFixed

                QGCRadioButton {
                    width:          _editFieldWidth
                    text:           "Landscape"
                    checked:        !!missionItem.cameraOrientationLandscape.value
                    exclusiveGroup: cameraOrientationGroup
                    onClicked:      missionItem.cameraOrientationLandscape.value = 1
                }

                QGCRadioButton {
                    id:             cameraOrientationPortrait
                    text:           "Portrait"
                    checked:        !missionItem.cameraOrientationLandscape.value
                    exclusiveGroup: cameraOrientationGroup
                    onClicked:      missionItem.cameraOrientationLandscape.value = 0
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
                    QGCLabel { text: qsTr("Sensor"); Layout.fillWidth: true }
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
                    QGCLabel { text: qsTr("Image"); Layout.fillWidth: true }
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
                        text:                   qsTr("Focal length")
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
                QGCLabel { text: qsTr("Overlap"); Layout.fillWidth: true }
                FactTextField {
                    Layout.preferredWidth:  _root._fieldWidth
                    fact:                   missionItem.frontalOverlap
                }
                FactTextField {
                    Layout.preferredWidth:  _root._fieldWidth
                    fact:                   missionItem.sideOverlap
                }
            }

            SectionHeader { text: qsTr("Grid") }

            GridLayout {
                anchors.left:   parent.left
                anchors.right:  parent.right
                columnSpacing:  _margin
                rowSpacing:     _margin
                columns:        2

                QGCLabel { text: qsTr("Angle") }
                FactTextField {
                    fact:               missionItem.gridAngle
                    Layout.fillWidth:   true
                }

                QGCLabel { text: qsTr("Turnaround dist") }
                FactTextField {
                    fact:                   missionItem.turnaroundDist
                    Layout.fillWidth:       true
                }

                QGCLabel {
                    wrapMode:       Text.WordWrap
                    font.pointSize: ScreenTools.smallFontPointSize
                    text:           qsTr("Which value would you like to keep constant as you adjust other settings")
                    Layout.preferredWidth:  parent.width
                    Layout.columnSpan: 2
                }

                QGCRadioButton {
                    id:                     fixedAltitudeRadio
                    text:                   qsTr("Altitude")
                    checked:                !!missionItem.fixedValueIsAltitude.value
                    exclusiveGroup:         fixedValueGroup
                    onClicked:              missionItem.fixedValueIsAltitude.value = 1
                }

                FactTextField {
                    fact:                   missionItem.gridAltitude
                    enabled:                fixedAltitudeRadio.checked
                    Layout.fillWidth:       true
                }

                QGCRadioButton {
                    id:                     fixedGroundResolutionRadio
                    text:                   qsTr("Ground res")
                    checked:                !missionItem.fixedValueIsAltitude.value
                    exclusiveGroup:         fixedValueGroup
                    onClicked:              missionItem.fixedValueIsAltitude.value = 0
                }

                FactTextField {
                    fact:                   missionItem.groundResolution
                    enabled:                fixedGroundResolutionRadio.checked
                    Layout.fillWidth:       true
                }
            }
        }

        // Manual grid ui
        Column {
            anchors.left:   parent.left
            anchors.right:  parent.right
            spacing:        _margin
            visible:        gridTypeCombo.currentIndex == _gridTypeManual

            SectionHeader { text: qsTr("Grid") }

            FactTextFieldGrid {
                anchors.left:   parent.left
                anchors.right:  parent.right
                columnSpacing:  ScreenTools.defaultFontPixelWidth
                rowSpacing:     _margin
                factList:       [ missionItem.gridAngle, missionItem.gridSpacing, missionItem.gridAltitude, missionItem.turnaroundDist ]
                factLabels:     [ qsTr("Angle"), qsTr("Spacing"), qsTr("Altitude"), qsTr("Turnaround dist")]
            }

            Item { height: _margin;  width: 1; visible: !ScreenTools.isTinyScreen }

            FactCheckBox {
                anchors.left:   parent.left
                text:           qsTr("Relative altitude")
                fact:           missionItem.gridAltitudeRelative
            }

            Item { height: _sectionSpacer;  width: 1; visible: !ScreenTools.isTinyScreen }

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

                FactCheckBox {
                    anchors.baseline:   cameraTriggerDistanceField.baseline
                    text:               qsTr("Trigger Distance")
                    fact:               missionItem.cameraTrigger
                }

                FactTextField {
                    id:                 cameraTriggerDistanceField
                    Layout.fillWidth:   true
                    fact:               missionItem.cameraTriggerDistance
                    enabled:            missionItem.cameraTrigger.value
                }
            }
        }

        SectionHeader { text: qsTr("Statistics") }

        Grid {
            columns:        2
            columnSpacing:  ScreenTools.defaultFontPixelWidth

            QGCLabel { text: qsTr("Survey area") }
            QGCLabel { text: QGroundControl.squareMetersToAppSettingsAreaUnits(missionItem.coveredArea).toFixed(2) + " " + QGroundControl.appSettingsAreaUnitsString }

            QGCLabel { text: qsTr("Photo count") }
            QGCLabel { text: missionItem.cameraShots }

            QGCLabel { text: qsTr("Photo interval") }
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
}
