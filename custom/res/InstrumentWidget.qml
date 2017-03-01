/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/**
 * @file
 *   @brief Fly View Instrument Widget
 *   @author Gus Grubba <mavlink@grubba.com>
 */

import QtQuick          2.4
import QtPositioning    5.2
import QtQuick.Layouts  1.2

import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FlightMap     1.0
import QGroundControl.Palette       1.0
import QGroundControl.CameraControl 1.0

import TyphoonHQuickInterface           1.0
import TyphoonHQuickInterface.Widgets   1.0

Item {
    id:     instrumentWidget
    height: mainRect.height
    width:  getPreferredInstrumentWidth() * 0.75

    property real _spacers:         ScreenTools.defaultFontPixelHeight * 0.5
    property real _distance:        0.0
    property real _editFieldWidth:  ScreenTools.defaultFontPixelWidth * 30

    //-- Position from System GPS
    PositionSource {
        id:             positionSource
        updateInterval: 1000
        active:         !TyphoonHQuickInterface.hardwareGPS && activeVehicle && activeVehicle.homePositionAvailable
        onPositionChanged: {
            var gcs = positionSource.position.coordinate;
            var veh = activeVehicle ? activeVehicle.coordinate : QtPositioning.coordinate(0,0);
            _distance = activeVehicle ? gcs.distanceTo(veh) : 0.0;
            //console.log("Qt PositionSource: " + gcs + veh + _distance)
        }
    }

    //-- Position from Controller GPS (M4)
    Connections {
        target: TyphoonHQuickInterface
        onControllerLocationChanged: {
            if(activeVehicle) {
                if(TyphoonHQuickInterface.latitude == 0.0 && TyphoonHQuickInterface.longitude == 0.0) {
                    _distance = 0.0
                } else {
                    var gcs = QtPositioning.coordinate(TyphoonHQuickInterface.latitude, TyphoonHQuickInterface.longitude, TyphoonHQuickInterface.altitude)
                    var veh = activeVehicle.coordinate;
                    _distance = gcs.distanceTo(veh);
                    //console.log("M4 PositionSource: GCS:" + gcs + " VEH:" + veh + " D:" + _distance)
                }
            }
        }
    }

    /*
    Connections {
        target: TyphoonHQuickInterface.cameraControl
        onCameraModeChanged: {
            console.log('QML: Camera Mode Changed: ' + TyphoonHQuickInterface.cameraControl.cameraMode)
        }
        onRecordTimeChanged: {
            console.log('QML: Record Time: ' + TyphoonHQuickInterface.cameraControl.recordTime)
        }
        onVideoStatusChanged: {
            console.log('QML: Video Status: ' + TyphoonHQuickInterface.cameraControl.videoStatus + ' ' + CameraControl.VIDEO_CAPTURE_STATUS_STOPPED)
        }
    }
    */

    Rectangle {
        id:             mainRect
        width:          parent.width
        height:         instrumentColumn.height
        radius:         8
        color:          Qt.rgba(0.15,0.15,0.25,0.75)
        border.width:   1
        border.color:   "black"
        Column {
            id:         instrumentColumn
            width:      parent.width
            spacing:    ScreenTools.defaultFontPixelHeight * 0.25
            anchors.verticalCenter: parent.verticalCenter
            Item {
                height: _spacers
                width:  1
            }
            //-- Attitude Indicator
            AttitudeWidget {
                id:             attitudeIndicator
                size:           parent.width * 0.95
                vehicle:        activeVehicle
                visible:        true
                anchors.horizontalCenter: parent.horizontalCenter
                MouseArea {
                    anchors.fill:   parent
                    enabled:        attitudeIndicator.visible
                    onClicked: {
                        attitudeIndicator.visible = false
                        compass.visible = true
                    }
                }
            }
            QGCCompassWidget {
                id:             compass
                size:           parent.width * 0.95
                vehicle:        activeVehicle
                visible:        false
                anchors.horizontalCenter: parent.horizontalCenter
                MouseArea {
                    anchors.fill:   parent
                    enabled:        compass.visible
                    onClicked: {
                        compass.visible = false
                        attitudeIndicator.visible = true
                    }
                }
            }
            Row {
                spacing:        ScreenTools.defaultFontPixelHeight * 0.25
                anchors.horizontalCenter: parent.horizontalCenter
                Image {
                    height:             ScreenTools.defaultFontPixelHeight
                    width:              height
                    sourceSize.width:   width
                    source:             "qrc:/typhoonh/UpArrow.svg"
                    fillMode:           Image.PreserveAspectFit
                    smooth:         true
                    mipmap:         true
                    antialiasing:   true
                    anchors.verticalCenter: parent.verticalCenter
                }
                QGCLabel {
                    text:           activeVehicle ? (isNaN(activeVehicle.altitudeRelative.rawValue) ? 0 : activeVehicle.altitudeRelative.rawValue.toFixed(0)) : 0
                    width:          ScreenTools.defaultFontPixelWidth * 4
                    font.family:    ScreenTools.demiboldFontFamily
                    horizontalAlignment: Text.AlignHCenter
                    anchors.verticalCenter: parent.verticalCenter
                }
                Image {
                    height:             ScreenTools.defaultFontPixelHeight
                    width:              height
                    sourceSize.width:   width
                    source:             "qrc:/typhoonh/RightArrow.svg"
                    fillMode:           Image.PreserveAspectFit
                    smooth:         true
                    mipmap:         true
                    antialiasing:   true
                    anchors.verticalCenter: parent.verticalCenter
                }
                QGCLabel {
                    text:           isNaN(_distance) ? 0.0 : _distance.toFixed(0)
                    width:          ScreenTools.defaultFontPixelWidth * 4
                    font.family:    ScreenTools.demiboldFontFamily
                    horizontalAlignment: Text.AlignHCenter
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
            QGCLabel {
                text:           activeVehicle ? activeVehicle.groundSpeed.rawValue.toFixed(1) : "--"
                font.family:    ScreenTools.demiboldFontFamily
                font.pointSize: ScreenTools.largeFontPointSize
                anchors.horizontalCenter: parent.horizontalCenter
            }
            //-----------------------------------------------------------------
            Rectangle {
                height:         1
                width:          parent.width * 0.9
                color:          Qt.rgba(1,1,1,0.5)
                anchors.horizontalCenter: parent.horizontalCenter
            }
            Item {
                height: _spacers * 2
                width:  1
            }
            //-- Camera Mode
            Rectangle {
                width:          ScreenTools.defaultFontPixelWidth  * 12
                height:         ScreenTools.defaultFontPixelHeight * 2
                radius:         width * 0.5
                color:          "black"
                anchors.horizontalCenter: parent.horizontalCenter
                Rectangle {
                    height:             parent.height
                    width:              parent.width * 0.5
                    radius:             width * 0.5
                    color:              TyphoonHQuickInterface.cameraControl.cameraMode === CameraControl.CAMERA_MODE_PHOTO ? toolBar.colorGreen : "black"
                    anchors.left:       parent.left
                    QGCColoredImage {
                        height:             parent.height * 0.75
                        width:              height
                        sourceSize.width:   width
                        source:             "qrc:/typhoonh/camera.svg"
                        fillMode:           Image.PreserveAspectFit
                        color:              TyphoonHQuickInterface.cameraControl.cameraMode === CameraControl.CAMERA_MODE_PHOTO ? "black" : toolBar.colorGrey
                        anchors.centerIn:   parent
                    }
                }
                Rectangle {
                    height:             parent.height
                    width:              parent.width * 0.5
                    radius:             width * 0.5
                    color:              TyphoonHQuickInterface.cameraControl.cameraMode === CameraControl.CAMERA_MODE_VIDEO ? toolBar.colorGreen : "black"
                    anchors.right:      parent.right
                    QGCColoredImage {
                        height:             parent.height * 0.75
                        width:              height
                        sourceSize.width:   width
                        source:             "qrc:/typhoonh/video.svg"
                        fillMode:           Image.PreserveAspectFit
                        color:              TyphoonHQuickInterface.cameraControl.cameraMode === CameraControl.CAMERA_MODE_VIDEO ? "black" : toolBar.colorGrey
                        anchors.centerIn:   parent
                    }
                }
                MouseArea {
                    anchors.fill:   parent
                    enabled:        TyphoonHQuickInterface.cameraControl.videoStatus !== CameraControl.VIDEO_CAPTURE_STATUS_UNDEFINED
                    onClicked: {
                        TyphoonHQuickInterface.cameraControl.toggleMode()
                    }
                }
            }
            Item {
                height: _spacers * 2
                width:  1
            }
            Rectangle {
                height:             ScreenTools.defaultFontPixelHeight * 4
                width:              height
                radius:             width * 0.5
                color:              Qt.rgba(0.0,0.0,0.0,0.0)
                border.width:       1
                border.color:       toolBar.colorWhite
                anchors.horizontalCenter: parent.horizontalCenter
                QGCColoredImage {
                    id:                 startVideoButton
                    height:             parent.height * 0.5
                    width:              height
                    sourceSize.width:   width
                    source:             "qrc:/typhoonh/video.svg"
                    fillMode:           Image.PreserveAspectFit
                    color:              TyphoonHQuickInterface.cameraControl.cameraMode === CameraControl.CAMERA_MODE_VIDEO ? toolBar.colorGreen : toolBar.colorGrey
                    visible:            TyphoonHQuickInterface.cameraControl.cameraMode === CameraControl.CAMERA_MODE_VIDEO && TyphoonHQuickInterface.cameraControl.videoStatus !== CameraControl.VIDEO_CAPTURE_STATUS_RUNNING
                    anchors.centerIn:   parent
                    MouseArea {
                        anchors.fill:   parent
                        enabled:        TyphoonHQuickInterface.cameraControl.videoStatus === CameraControl.VIDEO_CAPTURE_STATUS_STOPPED
                        onClicked: {
                            TyphoonHQuickInterface.cameraControl.startVideo()
                        }
                    }
                }
                Rectangle {
                    id:                 stopVideoButton
                    height:             parent.height * 0.5
                    width:              height
                    color:              TyphoonHQuickInterface.cameraControl.cameraMode === CameraControl.CAMERA_MODE_VIDEO ? toolBar.colorRed : toolBar.colorGrey
                    visible:            TyphoonHQuickInterface.cameraControl.cameraMode === CameraControl.CAMERA_MODE_VIDEO && TyphoonHQuickInterface.cameraControl.videoStatus === CameraControl.VIDEO_CAPTURE_STATUS_RUNNING
                    anchors.centerIn:   parent
                    MouseArea {
                        anchors.fill:   parent
                        enabled:        TyphoonHQuickInterface.cameraControl.videoStatus === CameraControl.VIDEO_CAPTURE_STATUS_RUNNING
                        onClicked: {
                            TyphoonHQuickInterface.cameraControl.stopVideo()
                        }
                    }
                }
                QGCColoredImage {
                    height:             parent.height * 0.5
                    width:              height
                    sourceSize.width:   width
                    source:             "qrc:/typhoonh/camera.svg"
                    fillMode:           Image.PreserveAspectFit
                    color:              TyphoonHQuickInterface.cameraControl.cameraMode !== CameraControl.CAMERA_MODE_UNDEFINED ? toolBar.colorGreen : toolBar.colorGrey
                    visible:            !startVideoButton.visible && !stopVideoButton.visible
                    anchors.centerIn:   parent
                    MouseArea {
                        anchors.fill:   parent
                        enabled:        TyphoonHQuickInterface.cameraControl.cameraMode !== CameraControl.CAMERA_MODE_UNDEFINED
                        onClicked: {
                            TyphoonHQuickInterface.cameraControl.takePhoto()
                        }
                    }
                }
            }
            /* Disabled for now
            Item {
                height: _spacers
                width:  1
            }
            Rectangle {
                height:             ScreenTools.defaultFontPixelHeight * 4
                width:              height
                radius:             width * 0.5
                color:              Qt.rgba(0.0,0.0,0.0,0.0)
                border.width:       1
                border.color:       toolBar.colorWhite
                anchors.horizontalCenter: parent.horizontalCenter
                QGCColoredImage {
                    height:             parent.height * 0.5
                    width:              height
                    sourceSize.width:   width
                    source:             "qrc:/typhoonh/camera.svg"
                    fillMode:           Image.PreserveAspectFit
                    color:              TyphoonHQuickInterface.cameraControl.cameraMode !== CameraControl.CAMERA_MODE_UNDEFINED ? toolBar.colorGreen : toolBar.colorGrey
                    anchors.centerIn:   parent
                }
                MouseArea {
                    anchors.fill:   parent
                    enabled:        TyphoonHQuickInterface.cameraControl.cameraMode !== CameraControl.CAMERA_MODE_UNDEFINED
                    onClicked: {
                        TyphoonHQuickInterface.cameraControl.takePhoto()
                    }
                }
            }
            */
            Item {
                height: _spacers * 2
                width:  1
            }
            Rectangle {
                height:             ScreenTools.defaultFontPixelHeight * 2.5
                width:              height
                radius:             width * 0.5
                color:              Qt.rgba(0.0,0.0,0.0,0.0)
                border.width:       1
                border.color:       toolBar.colorWhite
                anchors.horizontalCenter: parent.horizontalCenter
                QGCColoredImage {
                    height:             parent.height * 0.65
                    width:              height
                    sourceSize.width:   width
                    source:             "qrc:/typhoonh/CogWheel.svg"
                    fillMode:           Image.PreserveAspectFit
                    color:              TyphoonHQuickInterface.cameraControl.cameraMode !== CameraControl.CAMERA_MODE_UNDEFINED ? toolBar.colorWhite : toolBar.colorGrey
                    anchors.centerIn:   parent
                }
                MouseArea {
                    anchors.fill:       parent
                    enabled:            TyphoonHQuickInterface.cameraControl.cameraMode !== CameraControl.CAMERA_MODE_UNDEFINED
                    onClicked: {
                        rootLoader.sourceComponent = cameraSettingsComponent
                    }
                }
            }
            Item {
                height:         _spacers * 2
                width:          1
            }
        }
    }

    Component {
        id: cameraSettingsComponent
        Rectangle {
            id:     cameraSettingsRect
            width:  mainWindow.width  * 0.45
            height: mainWindow.height * 0.75
            radius: ScreenTools.defaultFontPixelWidth
            color:  Qt.rgba(0,0,0,0.75)
            anchors.centerIn: parent
            QGCLabel {
                id:                 cameraSettingsLabel
                text:               "Camera Settings"
                font.family:        ScreenTools.demiboldFontFamily
                font.pointSize:     ScreenTools.mediumFontPointSize
                anchors.margins:    ScreenTools.defaultFontPixelHeight * 0.5
                anchors.top:        parent.top
                anchors.left:       parent.left
            }
            Column {
                id:                 cameraSettingsCol
                spacing:            ScreenTools.defaultFontPixelHeight * 0.5
                width:              cameraGrid.width
                anchors.margins:    ScreenTools.defaultFontPixelHeight
                anchors.centerIn:   parent
                GridLayout {
                    id:             cameraGrid
                    columnSpacing:  ScreenTools.defaultFontPixelWidth
                    rowSpacing:     columnSpacing
                    columns:        2
                    anchors.horizontalCenter: parent.horizontalCenter
                    Rectangle {
                        color:      qgcPal.button
                        height:     1
                        width:      mainWindow.width * 0.4
                        Layout.columnSpan: 2
                        Layout.maximumHeight: 2
                    }
                    QGCLabel {
                        text:       "Video Resolution"
                        Layout.fillWidth: true
                    }
                    QGCComboBox {
                        width:       _editFieldWidth
                        model:       TyphoonHQuickInterface.cameraControl.videoResList
                        currentIndex:TyphoonHQuickInterface.cameraControl.currentVideoRes
                        onActivated: {
                            TyphoonHQuickInterface.cameraControl.currentVideoRes = index
                        }
                        Layout.preferredWidth:  _editFieldWidth
                    }
                    Rectangle {
                        color:      qgcPal.button
                        height:     1
                        width:      mainWindow.width * 0.4
                        Layout.columnSpan: 2
                        Layout.maximumHeight: 2
                    }
                    QGCLabel {
                        text:       "White Balance"
                        Layout.fillWidth: true
                    }
                    QGCComboBox {
                        width:       _editFieldWidth
                        model:       TyphoonHQuickInterface.cameraControl.wbList
                        currentIndex:TyphoonHQuickInterface.cameraControl.currentWb
                        onActivated: {
                            TyphoonHQuickInterface.cameraControl.currentWb = index
                        }
                        Layout.preferredWidth:  _editFieldWidth
                    }
                    Rectangle {
                        color:      qgcPal.button
                        height:     1
                        width:      mainWindow.width * 0.4
                        Layout.columnSpan: 2
                        Layout.maximumHeight: 2
                    }
                    QGCLabel {
                        text:       "ISO"
                        Layout.fillWidth: true
                    }
                    QGCComboBox {
                        width:       _editFieldWidth
                        model:       TyphoonHQuickInterface.cameraControl.isoList
                        currentIndex:TyphoonHQuickInterface.cameraControl.currentIso
                        onActivated: {
                            TyphoonHQuickInterface.cameraControl.currentIso = index
                        }
                        Layout.preferredWidth:  _editFieldWidth
                        //-- Not Yet
                        enabled:    false
                    }
                    Rectangle {
                        color:      qgcPal.button
                        height:     1
                        width:      mainWindow.width * 0.4
                        Layout.columnSpan: 2
                        Layout.maximumHeight: 2
                    }
                    QGCLabel {
                        text:       "Shutter Speed"
                        Layout.fillWidth: true
                    }
                    QGCComboBox {
                        width:       _editFieldWidth
                        model:       TyphoonHQuickInterface.cameraControl.shutterList
                        currentIndex:TyphoonHQuickInterface.cameraControl.currentShutter
                        onActivated: {
                            TyphoonHQuickInterface.cameraControl.currentShutter = index
                        }
                        Layout.preferredWidth:  _editFieldWidth
                        //-- Not Yet
                        enabled:    false
                    }
                    Rectangle {
                        color:      qgcPal.button
                        height:     1
                        width:      mainWindow.width * 0.4
                        Layout.columnSpan: 2
                        Layout.maximumHeight: 2
                    }
                    QGCLabel {
                        text:       "Color Mode"
                        Layout.fillWidth: true
                    }
                    QGCComboBox {
                        width:       _editFieldWidth
                        model:       TyphoonHQuickInterface.cameraControl.colorModeList
                        //-- Not Yet
                        enabled:    false
                        Layout.preferredWidth:  _editFieldWidth
                    }
                    Rectangle {
                        color:      qgcPal.button
                        height:     1
                        width:      mainWindow.width * 0.4
                        Layout.columnSpan: 2
                        Layout.maximumHeight: 2
                    }
                    QGCLabel {
                        text:       "Photo Format"
                        Layout.fillWidth: true
                    }
                    QGCComboBox {
                        width:       _editFieldWidth
                        model:       ["Raw", "Jpeg", "Raw+Jpeg"]
                        //-- Not Yet
                        enabled:    false
                        Layout.preferredWidth:  _editFieldWidth
                    }
                    Rectangle {
                        color:      qgcPal.button
                        height:     1
                        width:      mainWindow.width * 0.4
                        Layout.columnSpan: 2
                        Layout.maximumHeight: 2
                    }
                    /* Not Supported by MAVLink
                    QGCLabel {
                        text:       "Metering Mode"
                        Layout.fillWidth: true
                    }
                    QGCComboBox {
                        width:       _editFieldWidth
                        model:       ["Spot", "Center", "Average"]
                        Layout.preferredWidth:  _editFieldWidth
                    }
                    Rectangle {
                        color:      qgcPal.button
                        height:     1
                        width:      mainWindow.width * 0.4
                        Layout.columnSpan: 2
                        Layout.maximumHeight: 2
                    }
                    */
                    QGCLabel {
                        text:       "Screen Grid"
                        Layout.fillWidth: true
                    }
                    OnOffSwitch {
                        checked:     QGroundControl.settingsManager.videoSettings.gridLines.rawValue
                        Layout.alignment: Qt.AlignRight
                        onClicked:  QGroundControl.settingsManager.videoSettings.gridLines.rawValue = checked
                    }
                    Rectangle {
                        color:      qgcPal.button
                        height:     1
                        width:      mainWindow.width * 0.4
                        Layout.columnSpan: 2
                        Layout.maximumHeight: 2
                    }
                    QGCLabel {
                        text:       "Micro SD Card"
                        Layout.fillWidth: true
                    }
                    QGCButton {
                        text:       "Format"
                        //-- Not Yet
                        enabled:    false
                        Layout.preferredWidth:  _editFieldWidth
                    }
                    Rectangle {
                        color:      qgcPal.button
                        height:     1
                        width:      mainWindow.width * 0.4
                        Layout.columnSpan: 2
                        Layout.maximumHeight: 2
                    }
                }
            }
            //-- Dismiss Window
            Image {
                anchors.margins:    ScreenTools.defaultFontPixelHeight * 0.5
                anchors.top:        parent.top
                anchors.right:      parent.right
                width:              ScreenTools.defaultFontPixelHeight * 1.5
                height:             width
                sourceSize.height:  width
                source:             "/res/XDelete.svg"
                fillMode:           Image.PreserveAspectFit
                mipmap:             true
                smooth:             true
                MouseArea {
                    anchors.fill:   parent
                    onClicked: {
                        rootLoader.sourceComponent = null
                    }
                }
            }
            Component.onCompleted: {
                rootLoader.width  = cameraSettingsRect.width
                rootLoader.height = cameraSettingsRect.height
            }
        }
    }

}
