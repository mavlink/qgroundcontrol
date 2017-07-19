/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick                          2.3
import QtQuick.Controls                 1.2
import QtQuick.Dialogs                  1.2
import QtGraphicalEffects               1.0

import QGroundControl                   1.0
import QGroundControl.ScreenTools       1.0
import QGroundControl.Controls          1.0
import QGroundControl.Palette           1.0
import QGroundControl.Vehicle           1.0
import QGroundControl.Controllers       1.0
import QGroundControl.FactSystem        1.0
import QGroundControl.FactControls      1.0

Rectangle {
    id:             mainRect
    height:         mainRow.height + (ScreenTools.defaultFontPixelWidth * 2)
    width:          mainRow.width  + (ScreenTools.defaultFontPixelWidth * 2)
    radius:         ScreenTools.defaultFontPixelWidth * 0.5
    color:          qgcPal.globalTheme === QGCPalette.Light ? Qt.rgba(1,1,1,0.95) : Qt.rgba(0,0,0,0.75)
    border.width:   1
    border.color:   qgcPal.globalTheme === QGCPalette.Light ? Qt.rgba(0,0,0,0.35) : Qt.rgba(1,1,1,0.35)

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    property var    _activeVehicle:         QGroundControl.multiVehicleManager.activeVehicle
    property var    _dynamicCameras:        _activeVehicle ? _activeVehicle.dynamicCameras : null
    property bool   _isCamera:              _dynamicCameras ? _dynamicCameras.cameras.count > 0 : false
    property bool   _cameraModeUndefined:   _isCamera ? _dynamicCameras.cameras.get(0).cameraMode === QGCCameraControl.CAMERA_MODE_UNDEFINED : true
    property bool   _cameraVideoMode:       _isCamera ? _dynamicCameras.cameras.get(0).cameraMode === QGCCameraControl.CAMERA_MODE_VIDEO : false
    property bool   _cameraPhotoMode:       _isCamera ? _dynamicCameras.cameras.get(0).cameraMode === QGCCameraControl.CAMERA_MODE_PHOTO : false
    property var    _camera:                _isCamera ? _dynamicCameras.cameras.get(0) : null // Single camera support for the time being
    property real   _spacers:               ScreenTools.defaultFontPixelHeight * 0.5
    property real   _labelFieldWidth:       ScreenTools.defaultFontPixelWidth * 30
    property real   _editFieldWidth:        ScreenTools.defaultFontPixelWidth * 30
    property bool   _communicationLost:     _activeVehicle ? _activeVehicle.connectionLost : false
    property bool   _hasModes:              _isCamera && _camera && _camera.hasModes

    MouseArea {
        anchors.fill:   parent
        onWheel:        { wheel.accepted = true; }
        onPressed:      { mouse.accepted = true; }
        onReleased:     { mouse.accepted = true; }
    }

    Connections {
        target: QGroundControl.multiVehicleManager.activeVehicle
        onConnectionLostChanged: {
            if(_communicationLost) {
                if(rootLoader.sourceComponent === cameraSettingsComponent) {
                    rootLoader.sourceComponent = null
                }
            }
        }
    }

    Row {
        id:             mainRow
        spacing:        _spacers
        anchors.centerIn: parent
        Column {
            spacing:        _spacers
            anchors.verticalCenter: parent.verticalCenter
            //-----------------------------------------------------------------
            QGCLabel {
                id:             cameraLabel
                text:           _isCamera ? _dynamicCameras.cameras.get(0).modelName : qsTr("Camera")
                font.pointSize: ScreenTools.smallFontPointSize
                anchors.horizontalCenter: parent.horizontalCenter
            }
            //-- Camera Mode (visible only if camera has modes)
            Rectangle {
                width:      _hasModes ? ScreenTools.defaultFontPixelWidth *  12 : 0
                height:     _hasModes ? ScreenTools.defaultFontPixelWidth *   4 : 0
                color:      qgcPal.window
                radius:     height * 0.5
                visible:    _hasModes
                anchors.horizontalCenter: parent.horizontalCenter
                //-- Video Mode
                Rectangle {
                    width:  parent.height * 0.9
                    height: parent.height * 0.9
                    color:  qgcPal.windowShadeDark
                    radius: height * 0.5
                    anchors.left: parent.left
                    anchors.leftMargin: 4
                    anchors.verticalCenter: parent.verticalCenter
                    QGCColoredImage {
                        anchors.fill:       parent
                        source:             "/qmlimages/camera_video.svg"
                        fillMode:           Image.PreserveAspectFit
                        sourceSize.height:  height
                        color:              _cameraVideoMode ? qgcPal.colorGreen : qgcPal.text
                        MouseArea {
                            anchors.fill:   parent
                            enabled:        _cameraPhotoMode
                            onClicked: {
                                _camera.setVideoMode()
                            }
                        }
                    }
                }
                //-- Photo Mode
                Rectangle {
                    width:  parent.height * 0.9
                    height: parent.height * 0.9
                    color:  qgcPal.window
                    radius: height * 0.5
                    anchors.right: parent.right
                    anchors.rightMargin: 4
                    anchors.verticalCenter: parent.verticalCenter
                    QGCColoredImage {
                        anchors.fill:       parent
                        source:             "/qmlimages/camera_photo.svg"
                        fillMode:           Image.PreserveAspectFit
                        sourceSize.height:  height
                        color:              _cameraPhotoMode ? qgcPal.colorGreen : qgcPal.text
                        MouseArea {
                            anchors.fill:   parent
                            enabled:        _cameraVideoMode
                            onClicked: {
                                _camera.setPhotoMode()
                            }
                        }
                    }
                }
            }
            //-- Settings
            QGCColoredImage {
                width:              ScreenTools.defaultFontPixelWidth * 3
                height:             width
                sourceSize.width:   width
                source:             "/qmlimages/camera_settings.svg"
                fillMode:           Image.PreserveAspectFit
                color:              _cameraModeUndefined ? qgcPal.colorGrey : qgcPal.text
                anchors.horizontalCenter: parent.horizontalCenter
                MouseArea {
                    anchors.fill:   parent
                    enabled:        !_cameraModeUndefined
                    onClicked: {
                        if(rootLoader.sourceComponent === null) {
                            rootLoader.sourceComponent = cameraSettingsComponent
                        } else {
                            rootLoader.sourceComponent = null
                        }
                    }
                }
            }
        }
        //-- Shutter
        Rectangle {
            color:      Qt.rgba(0,0,0,0)
            width:      ScreenTools.defaultFontPixelWidth * 6
            height:     width
            radius:     width * 0.5
            border.color: qgcPal.buttonText
            border.width: 3
            anchors.verticalCenter: parent.verticalCenter
            Rectangle {
                width:      parent.width * 0.75
                height:     width
                radius:     width * 0.5
                color:      _cameraModeUndefined ? qgcPal.colorGrey : qgcPal.colorRed
                anchors.centerIn:   parent
            }
            MouseArea {
                anchors.fill:   parent
                enabled:        !_cameraModeUndefined
                onClicked: {
                    if(_cameraVideoMode) {
                        //-- Start/Stop Video
                    } else {
                        _camera.takePhoto()
                    }
                }
            }
        }
    }

    Component {
        id:         cameraSettingsComponent
        Item {
            id:     cameraSettingsRect
            width:  mainWindow.width
            height: mainWindow.height
            anchors.centerIn: parent
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    rootLoader.sourceComponent = null
                }
            }
            Rectangle {
                id:     camSettingsRect
                width:  _labelFieldWidth + _editFieldWidth + (ScreenTools.defaultFontPixelWidth * 8)
                height: Math.max(mainWindow.height * 0.65, ScreenTools.defaultFontPixelHeight  * 20)
                radius: ScreenTools.defaultFontPixelWidth
                color:  qgcPal.globalTheme === QGCPalette.Light ? Qt.rgba(1,1,1,0.95) : Qt.rgba(0,0,0,0.75)
                border.width:   1
                border.color:   qgcPal.globalTheme === QGCPalette.Light ? Qt.rgba(0,0,0,0.35) : Qt.rgba(1,1,1,0.35)
                anchors.centerIn: parent
                QGCLabel {
                    id:                 cameraSettingsLabel
                    text:               _cameraVideoMode ? qsTr("Video Settings") : qsTr("Camera Settings")
                    font.family:        ScreenTools.demiboldFontFamily
                    font.pointSize:     ScreenTools.mediumFontPointSize
                    anchors.margins:    ScreenTools.defaultFontPixelHeight * 0.5
                    anchors.top:        parent.top
                    anchors.left:       parent.left
                }
                QGCFlickable {
                    clip:               true
                    anchors.top:        cameraSettingsLabel.bottom
                    anchors.topMargin:  ScreenTools.defaultFontPixelHeight
                    anchors.bottom:     parent.bottom
                    width:              cameraSettingsCol.width
                    contentHeight:      cameraSettingsCol.height
                    contentWidth:       cameraSettingsCol.width
                    anchors.horizontalCenter: parent.horizontalCenter
                    Column {
                        id:                 cameraSettingsCol
                        spacing:            ScreenTools.defaultFontPixelHeight * 0.5
                        width:              camSettingsRect.width
                        anchors.margins:    ScreenTools.defaultFontPixelHeight
                        //-------------------------------------------
                        //-- Camera Settings
                        Repeater {
                            model:      _camera ? _camera.activeSettings : []
                            Row {
                                spacing:        ScreenTools.defaultFontPixelWidth
                                anchors.horizontalCenter: parent.horizontalCenter
                                QGCLabel {
                                    text:       _camera.getFact(modelData).shortDescription
                                    width:      _labelFieldWidth
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                                FactComboBox {
                                    width:      _editFieldWidth
                                    fact:       _camera.getFact(modelData)
                                    indexModel: false
                                    visible:    !_camera.getFact(modelData).typeIsBool
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                                Item {
                                    width:      _editFieldWidth
                                    height:     factSwitch.height
                                    visible:    _camera.getFact(modelData).typeIsBool
                                    anchors.verticalCenter: parent.verticalCenter
                                    Switch {
                                        id: factSwitch
                                        anchors.left: parent.left
                                        checked: fact ? fact.value : false
                                        onClicked: fact.value = checked ? 1 : 0
                                        property var fact: _camera.getFact(modelData)
                                    }
                                }
                            }
                        }
                        //-------------------------------------------
                        //-- Reset Camera
                        Row {
                            spacing:        ScreenTools.defaultFontPixelWidth
                            anchors.horizontalCenter: parent.horizontalCenter
                            QGCLabel {
                                text:       qsTr("Reset Camera Defaults")
                                width:      _labelFieldWidth
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            QGCButton {
                                text:       qsTr("Reset")
                                onClicked:  resetPrompt.open()
                                width:      _editFieldWidth
                                anchors.verticalCenter: parent.verticalCenter
                                MessageDialog {
                                    id:                 resetPrompt
                                    title:              qsTr("Reset Camera to Factory Settings")
                                    text:               qsTr("Confirm resetting all settings?")
                                    standardButtons:    StandardButton.Yes | StandardButton.No
                                    onNo: resetPrompt.close()
                                    onYes: {
                                        // TODO
                                        resetPrompt.close()
                                    }
                                }
                            }
                        }
                        //-------------------------------------------
                        //-- Format Storage
                        Row {
                            spacing:        ScreenTools.defaultFontPixelWidth
                            anchors.horizontalCenter: parent.horizontalCenter
                            QGCLabel {
                                text:       qsTr("Storage")
                                width:      _labelFieldWidth
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            QGCButton {
                                text:       qsTr("Format")
                                enabled:    false
                                onClicked:  formatPrompt.open()
                                width:      _editFieldWidth
                                anchors.verticalCenter: parent.verticalCenter
                                MessageDialog {
                                    id:                 formatPrompt
                                    title:              qsTr("Format Camera Storage")
                                    text:               qsTr("Confirm erasing all files?")
                                    standardButtons:    StandardButton.Yes | StandardButton.No
                                    onNo: formatPrompt.close()
                                    onYes: {
                                        // TODO
                                        formatPrompt.close()
                                    }
                                }
                            }
                        }
                    }
                }
            }
            Component.onCompleted: {
                rootLoader.width  = cameraSettingsRect.width
                rootLoader.height = cameraSettingsRect.height
            }
            Keys.onBackPressed: {
                rootLoader.sourceComponent = null
            }
            Keys.onEscapePressed: {
                rootLoader.sourceComponent = null
            }
        }
    }
}
