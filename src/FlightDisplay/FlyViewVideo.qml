/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick 2.12

import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.Controllers   1.0
import QGroundControl.ScreenTools   1.0

Item {
    id:         _root
    visible:    QGroundControl.videoManager.hasVideo

    property int    _track_rec_x:       0
    property int    _track_rec_y:       0

    property Item pipState: videoPipState
    QGCPipState {
        id:         videoPipState
        pipOverlay: _pipOverlay
        isDark:     true

        onWindowAboutToOpen: {
            QGroundControl.videoManager.stopVideo()
            videoStartDelay.start()
        }

        onWindowAboutToClose: {
            QGroundControl.videoManager.stopVideo()
            videoStartDelay.start()
        }

        onStateChanged: {
            if (pipState.state !== pipState.fullState) {
                QGroundControl.videoManager.fullScreen = false
            }
        }
    }

    Timer {
        id:           videoStartDelay
        interval:     2000;
        running:      false
        repeat:       false
        onTriggered:  QGroundControl.videoManager.startVideo()
    }

    //-- Video Streaming
    FlightDisplayViewVideo {
        id:             videoStreaming
        anchors.fill:   parent
        useSmallFont:   _root.pipState.state !== _root.pipState.fullState
        visible:        QGroundControl.videoManager.isGStreamer
    }
    //-- UVC Video (USB Camera or Video Device)
    Loader {
        id:             cameraLoader
        anchors.fill:   parent
        visible:        !QGroundControl.videoManager.isGStreamer
        source:         QGroundControl.videoManager.uvcEnabled ? "qrc:/qml/FlightDisplayViewUVC.qml" : "qrc:/qml/FlightDisplayViewDummy.qml"
    }

    QGCLabel {
        text: qsTr("Double-click to exit full screen")
        font.pointSize: ScreenTools.largeFontPointSize
        visible: QGroundControl.videoManager.fullScreen && flyViewVideoMouseArea.containsMouse
        anchors.centerIn: parent

        onVisibleChanged: {
            if (visible) {
                labelAnimation.start()
            }
        }

        PropertyAnimation on opacity {
            id: labelAnimation
            duration: 10000
            from: 1.0
            to: 0.0
            easing.type: Easing.InExpo
        }
    }

    OnScreenGimbalController {
        id:                      onScreenGimbalController
        anchors.fill:            parent
        screenX:                 flyViewVideoMouseArea.mouseX
        screenY:                 flyViewVideoMouseArea.mouseY
        cameraTrackingEnabled:   videoStreaming._camera && videoStreaming._camera.trackingEnabled
    }

    MouseArea {
        id:                         flyViewVideoMouseArea
        anchors.fill:               parent
        enabled:                    pipState.state === pipState.fullState
        hoverEnabled:               true
        scrollGestureEnabled:       true
        acceptedButtons:            Qt.LeftButton | Qt.RightButton

        property double x0:         0
        property double x1:         0
        property double y0:         0
        property double y1:         0
        property double offset_x:   0
        property double offset_y:   0
        property double radius:     20
        property var trackingStatus: trackingStatusComponent.createObject(flyViewVideoMouseArea, {})

        // Define constants representing rectangle scaling modes
        readonly property int scaleModeHorizontal: 0
        readonly property int scaleModeVertical: 1
        readonly property int scaleModeBoth: 2
        readonly property int numScaleModes: 3


        property int currentScaleMode: scaleModeBoth


        onDoubleClicked: QGroundControl.videoManager.fullScreen = !QGroundControl.videoManager.fullScreen

        onClicked: (mouse) => {
            // If right button is clicked, change the selection mode
            if (mouse.button == Qt.RightButton) {
                // If mode reached the highest value, put it back to zero
                currentScaleMode = (currentScaleMode + 1) % numScaleModes;
            }

            let x0 = Math.floor(Math.max(0, target_canvas.rect_center_x - target_canvas.rect_width / 2));
            let y0 = Math.floor(Math.max(0, target_canvas.rect_center_y - target_canvas.rect_height / 2));
            let x1 = Math.floor(Math.min(width, target_canvas.rect_center_x + target_canvas.rect_width / 2));
            let y1 = Math.floor(Math.min(height, target_canvas.rect_center_y + target_canvas.rect_height / 2));

            //calculate offset between video stream rect and background (black stripes)
            let offset_x = (parent.width - videoStreaming.getWidth()) / 2
            let offset_y = (parent.height - videoStreaming.getHeight()) / 2

            //calculate offset between video stream rect and background (black stripes)
            x0 -= offset_x
            x1 -= offset_x
            y0 -= offset_y
            y1 -= offset_y

            //convert absolute to relative coordinates and limit range to 0...1
            x0 = Math.max(Math.min(x0 / videoStreaming.getWidth(), 1.0), 0.0)
            x1 = Math.max(Math.min(x1 / videoStreaming.getWidth(), 1.0), 0.0)
            y0 = Math.max(Math.min(y0 / videoStreaming.getHeight(), 1.0), 0.0)
            y1 = Math.max(Math.min(y1 / videoStreaming.getHeight(), 1.0), 0.0)


            //use point message if rectangle is very small
            if (target_canvas.rect_width < 10 && target_canvas.rect_height < 10) {
                let pt  = Qt.point((rec_start_x + x1) / 2, (y0 + y1) / 2)
                videoStreaming._camera.startTracking(pt, radius / videoStreaming.getWidth())
            } else {
                let rec = Qt.rect(x0, y0, x1 - rect_start_x, y1 - y0)
                videoStreaming._camera.startTracking(rec)
            }
            videoStreaming._camera._requestTrackingStatus()
        }

        onWheel: (wheel) => {
            const WHEEL_DIVIDER = 19;

            console.log(wheel)
            console.log(wheel.angleDelta)
            if (currentScaleMode == scaleModeHorizontal || currentScaleMode == scaleModeBoth) {
                target_canvas.rect_width += Math.floor(Number( wheel.angleDelta.y) / WHEEL_DIVIDER);
            }
            if (currentScaleMode == scaleModeVertical || currentScaleMode == scaleModeBoth) {
                target_canvas.rect_height += Math.floor(Number( wheel.angleDelta.y) / WHEEL_DIVIDER);
            }

            target_canvas.requestPaint();
        }


        onPositionChanged: (mouse) => {
            target_canvas.rect_center_x = mouse.x;
            target_canvas.rect_center_y = mouse.y;
            target_canvas.requestPaint();
        }

        Canvas {
            property int rect_center_x
            property int rect_center_y

            property int rect_width: 100
            property int rect_height: 100

            anchors.fill: parent

            id: target_canvas
            onPaint: {
                const BORDER_WIDTH = 3;
                const BORDER_COLOR = '#11BB11';

                var ctx = getContext("2d");
                ctx.reset();

               if (!videoStreaming._camera || !videoStreaming._camera.trackingEnabled) {
                   return;
               }

                let rect_start_x = Math.floor(Math.max(0, rect_center_x - rect_width / 2));
                let rect_start_y = Math.floor(Math.max(0, rect_center_y - rect_height / 2));
                let rect_end_x = Math.floor(Math.min(width, rect_center_x + rect_width / 2));
                let rect_end_y = Math.floor(Math.min(height, rect_center_y + rect_height / 2));

                ctx.strokeStyle = BORDER_COLOR;
                ctx.lineWidth = BORDER_WIDTH;
                ctx.beginPath();
                ctx.moveTo(rect_start_x, rect_start_y);
                ctx.lineTo(rect_start_x, rect_end_y);
                ctx.lineTo(rect_end_x, rect_end_y);
                ctx.lineTo(rect_end_x, rect_start_y);
                ctx.lineTo(rect_start_x, rect_start_y);
                ctx.stroke();

            }
        }




        Component {
            id: trackingStatusComponent

            Rectangle {
                color:              "transparent"
                border.color:       "red"
                border.width:       5
                radius:             5
            }
        }

        Timer {
            id: trackingStatusTimer
            interval:               50
            repeat:                 true
            running:                true
            onTriggered: {
                if (videoStreaming._camera) {
                    if (videoStreaming._camera.trackingEnabled && videoStreaming._camera.trackingImageStatus) {
                        var margin_hor = (parent.parent.width - videoStreaming.getWidth()) / 2
                        var margin_ver = (parent.parent.height - videoStreaming.getHeight()) / 2
                        var left = margin_hor + videoStreaming.getWidth() * videoStreaming._camera.trackingImageRect.left
                        var top = margin_ver + videoStreaming.getHeight() * videoStreaming._camera.trackingImageRect.top
                        var right = margin_hor + videoStreaming.getWidth() * videoStreaming._camera.trackingImageRect.right
                        var bottom = margin_ver + !isNaN(videoStreaming._camera.trackingImageRect.bottom) ? videoStreaming.getHeight() * videoStreaming._camera.trackingImageRect.bottom : top + (right - left)
                        var width = right - left
                        var height = bottom - top

                        flyViewVideoMouseArea.trackingStatus.x = left
                        flyViewVideoMouseArea.trackingStatus.y = top
                        flyViewVideoMouseArea.trackingStatus.width = width
                        flyViewVideoMouseArea.trackingStatus.height = height
                    } else {
                        flyViewVideoMouseArea.trackingStatus.x = 0
                        flyViewVideoMouseArea.trackingStatus.y = 0
                        flyViewVideoMouseArea.trackingStatus.width = 0
                        flyViewVideoMouseArea.trackingStatus.height = 0
                    }
                }
            }
        }
    }

    ProximityRadarVideoView{
        anchors.fill:   parent
        vehicle:        QGroundControl.multiVehicleManager.activeVehicle
    }

    ObstacleDistanceOverlayVideo {
        id: obstacleDistance
        showText: pipState.state === pipState.fullState
    }
}
