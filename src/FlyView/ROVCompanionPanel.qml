import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlyView
import QGC

/*
 * ROVCompanionPanel — MIST MAVIROV companion drawer (right edge of Fly view).
 *
 * One-click "CONNECT ALL": waits for the Pi to boot, starts MAVProxy and the
 * multi-camera RTSP script over SSH, switches QGC video to the selected RTSP
 * camera, while QGC's normal UDP autoconnect picks up the Pixhawk on 14550.
 * Also drives the Teensy servo arm (claw + roll) over UDP with the same key
 * bindings and limits as ROV GCS v3.4, and shows the 3-D arm visualiser.
 *
 * Collapsed by default: only the vertical MAVIROV tab with a status dot is
 * visible, so the stock QGC Fly view stays untouched until it is opened.
 */
Item {
    id: _root

    property bool expanded: false

    readonly property var  _ctl:        ROVCompanionController
    readonly property real _margins:    ScreenTools.defaultFontPixelHeight / 2
    readonly property real _bodyWidth:  ScreenTools.defaultFontPixelWidth * 34
    readonly property real _tabWidth:   ScreenTools.defaultFontPixelHeight * 1.6

    readonly property color _accent:     "#00d2b8"
    readonly property color _okColor:    "#1fd286"
    readonly property color _warnColor:  "#e8b412"
    readonly property color _failColor:  "#e2425c"
    readonly property color _armOrange:  "#ff8c1e"

    width:  _tabWidth + (expanded ? _bodyWidth + _margins / 2 : 0)
    height: parent ? Math.min(parent.height - ScreenTools.defaultFontPixelHeight * 4,
                              ScreenTools.defaultFontPixelHeight * 42)
                   : ScreenTools.defaultFontPixelHeight * 42

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    function _stateColor() {
        if (_ctl.connected)            return _okColor
        if (_ctl.busy)                 return _warnColor
        if (_ctl.linkState === 5)      return _failColor   // Failed
        return qgcPal.colorGrey
    }

    function _applyRtspVideo() {
        var vs = QGroundControl.settingsManager.videoSettings
        if (!vs || !vs.visible) {
            return
        }
        vs.videoSource.rawValue = vs.rtspVideoSource
        vs.rtspUrl.rawValue     = _ctl.currentRtspUrl
    }

    Connections {
        target: _ctl
        function onApplyVideoSourceRequested() { _root._applyRtspVideo() }
        function onActiveCameraChanged() {
            if (_ctl.connected) {
                _root._applyRtspVideo()
            }
        }
    }

    // ── Vertical grab tab (always visible) ───────────────────────────────
    Rectangle {
        id:             tab
        anchors.left:   parent.left
        anchors.verticalCenter: parent.verticalCenter
        width:          _tabWidth
        height:         tabLabel.contentWidth + statusDot.height + ScreenTools.defaultFontPixelHeight * 2.5
        radius:         ScreenTools.defaultBorderRadius
        color:          qgcPal.toolbarBackground
        border.color:   _stateColor()
        border.width:   1

        Rectangle {
            id:                 statusDot
            anchors.top:        parent.top
            anchors.topMargin:  ScreenTools.defaultFontPixelHeight / 2
            anchors.horizontalCenter: parent.horizontalCenter
            width:              ScreenTools.defaultFontPixelHeight * 0.6
            height:             width
            radius:             width / 2
            color:              _stateColor()

            SequentialAnimation on opacity {
                running: _ctl.busy
                loops:   Animation.Infinite
                NumberAnimation { from: 1.0; to: 0.25; duration: 450 }
                NumberAnimation { from: 0.25; to: 1.0; duration: 450 }
            }
            onVisibleChanged: opacity = 1
        }

        QGCLabel {
            id:                 tabLabel
            anchors.centerIn:   parent
            anchors.verticalCenterOffset: statusDot.height / 2
            rotation:           -90
            text:               qsTr("MAVIROV")
            font.bold:          true
            font.pointSize:     ScreenTools.smallFontPointSize
            color:              _root.expanded ? _accent : qgcPal.text
        }

        QGCLabel {
            anchors.bottom:         parent.bottom
            anchors.bottomMargin:   ScreenTools.defaultFontPixelHeight / 3
            anchors.horizontalCenter: parent.horizontalCenter
            text:                   _root.expanded ? "\u25B6" : "\u25C0"
            font.pointSize:         ScreenTools.smallFontPointSize
            color:                  qgcPal.text
        }

        QGCMouseArea {
            anchors.fill: parent
            onClicked:    _root.expanded = !_root.expanded
        }
    }

    // ── Expanded body ─────────────────────────────────────────────────────
    Rectangle {
        id:             body
        anchors.left:   tab.right
        anchors.leftMargin: _margins / 2
        anchors.top:    parent.top
        anchors.bottom: parent.bottom
        width:          _bodyWidth
        visible:        _root.expanded
        radius:         ScreenTools.defaultBorderRadius
        color:          qgcPal.window
        opacity:        0.97
        border.color:   qgcPal.groupBorder
        border.width:   1

        onVisibleChanged: {
            if (!visible) {
                keyboardCheckBox.checked = false
            }
        }

        DeadMouseArea { anchors.fill: parent }

        QGCFlickable {
            id:                 flick
            anchors.fill:       parent
            anchors.margins:    _margins
            contentHeight:      mainColumn.height
            clip:               true

            ColumnLayout {
                id:      mainColumn
                width:   flick.width
                spacing: _margins

                // Header -------------------------------------------------
                QGCLabel {
                    Layout.fillWidth:       true
                    text:                   qsTr("MIST MAVIROV — ROV LINK")
                    font.bold:              true
                    color:                  _accent
                    horizontalAlignment:    Text.AlignHCenter
                }

                // Connect-all toggle ------------------------------------
                QGCButton {
                    Layout.fillWidth:   true
                    pointSize:          ScreenTools.mediumFontPointSize
                    backgroundColor:    _ctl.connected ? _failColor
                                       : _ctl.busy     ? _warnColor
                                                       : _accent
                    textColor:          "black"
                    text:               _ctl.connected ? qsTr("■  DISCONNECT ALL")
                                       : _ctl.busy     ? qsTr("◌  %1  (TAP TO CANCEL)").arg(_ctl.stateText)
                                       : _ctl.linkState === 5 ? qsTr("▶  CONNECT ALL  (RETRY)")
                                                              : qsTr("▶  CONNECT ALL")
                    onClicked:          _ctl.toggleConnect()
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing:          _margins / 2

                    Rectangle {
                        width:  ScreenTools.defaultFontPixelHeight * 0.55
                        height: width
                        radius: width / 2
                        color:  _stateColor()
                    }
                    QGCLabel {
                        Layout.fillWidth: true
                        text:             _ctl.stateText
                        color:            _stateColor()
                        font.bold:        true
                    }
                }

                // Link info ---------------------------------------------
                GridLayout {
                    Layout.fillWidth: true
                    columns:          2
                    columnSpacing:    _margins
                    rowSpacing:       2

                    QGCLabel { text: qsTr("Pi");     color: qgcPal.colorGrey; font.pointSize: ScreenTools.smallFontPointSize }
                    QGCLabel { Layout.fillWidth: true; text: _ctl.piAddress;     font.pointSize: ScreenTools.smallFontPointSize; elide: Text.ElideRight }
                    QGCLabel { text: qsTr("PC");     color: qgcPal.colorGrey; font.pointSize: ScreenTools.smallFontPointSize }
                    QGCLabel { Layout.fillWidth: true; text: _ctl.pcAddress;     font.pointSize: ScreenTools.smallFontPointSize; elide: Text.ElideRight }
                    QGCLabel { text: qsTr("Teensy"); color: qgcPal.colorGrey; font.pointSize: ScreenTools.smallFontPointSize }
                    QGCLabel { Layout.fillWidth: true; text: _ctl.teensyAddress; font.pointSize: ScreenTools.smallFontPointSize; elide: Text.ElideRight }
                    QGCLabel { text: qsTr("SSH");    color: qgcPal.colorGrey; font.pointSize: ScreenTools.smallFontPointSize }
                    QGCLabel { Layout.fillWidth: true; text: _ctl.sshModeHint;   font.pointSize: ScreenTools.smallFontPointSize; elide: Text.ElideRight; wrapMode: Text.WordWrap; maximumLineCount: 2 }
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: qgcPal.groupBorder }

                // Cameras -----------------------------------------------
                QGCLabel {
                    text:           qsTr("CAMERAS")
                    font.bold:      true
                    font.pointSize: ScreenTools.smallFontPointSize
                    color:          _accent
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing:          _margins / 2

                    Repeater {
                        model: _ctl.cameraCount

                        QGCButton {
                            Layout.fillWidth: true
                            text:             (_ctl.activeCamera === index ? "\u25B6 " : "") + qsTr("CAM %1").arg(index + 1)
                            backgroundColor:  _ctl.activeCamera === index ? _armOrange : qgcPal.button
                            textColor:        _ctl.activeCamera === index ? "black" : qgcPal.buttonText
                            onClicked:        _ctl.activeCamera = index
                        }
                    }
                }

                QGCLabel {
                    Layout.fillWidth: true
                    text:             _ctl.connected
                                      ? qsTr("Selected camera shows in QGC video (%1)").arg(_ctl.currentRtspUrl)
                                      : qsTr("Connect to stream the selected camera in QGC video")
                    font.pointSize:   ScreenTools.smallFontPointSize
                    color:            qgcPal.colorGrey
                    wrapMode:         Text.WordWrap
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: qgcPal.groupBorder }

                // Robotic arm -------------------------------------------
                QGCLabel {
                    text:           qsTr("🦾 ROBOTIC ARM")
                    font.bold:      true
                    font.pointSize: ScreenTools.smallFontPointSize
                    color:          _armOrange
                }

                Rectangle {
                    Layout.fillWidth:       true
                    Layout.preferredHeight: armView.height + 2
                    color:                  "#060614"
                    radius:                 ScreenTools.defaultBorderRadius
                    border.color:           _armOrange
                    border.width:           1
                    clip:                   true

                    ROVArmView {
                        id:                 armView
                        anchors.centerIn:   parent
                        width:              parent.width - 2
                        height:             Math.round(width * 450 / 740)
                        rollDeg:            _ctl.rollDegrees
                        clawRatio:          _ctl.clawRatio
                    }
                }

                RowLayout {
                    Layout.fillWidth: true

                    QGCLabel {
                        Layout.fillWidth: true
                        text:             qsTr("Roll: %1°").arg(_ctl.rollDegrees.toFixed(1))
                        color:            _warnColor
                        font.bold:        true
                        font.pointSize:   ScreenTools.smallFontPointSize
                    }
                    QGCLabel {
                        text:             qsTr("Claw: %1 cm").arg(_ctl.clawCm.toFixed(1))
                        color:            _okColor
                        font.bold:        true
                        font.pointSize:   ScreenTools.smallFontPointSize
                    }
                }

                GridLayout {
                    Layout.fillWidth: true
                    columns:          4
                    columnSpacing:    _margins / 2
                    rowSpacing:       _margins / 2

                    QGCButton {
                        Layout.fillWidth: true
                        text:             qsTr("ROLL ◀")
                        backgroundColor:  _warnColor
                        textColor:        "black"
                        onClicked:        _ctl.rollStep(-_ctl.bigStep)
                    }
                    QGCButton {
                        Layout.fillWidth: true
                        text:             qsTr("ROLL ▶")
                        backgroundColor:  _warnColor
                        textColor:        "black"
                        onClicked:        _ctl.rollStep(_ctl.bigStep)
                    }
                    QGCButton {
                        Layout.fillWidth: true
                        text:             qsTr("OPEN")
                        backgroundColor:  _okColor
                        textColor:        "black"
                        onClicked:        _ctl.clawStep(_ctl.bigStep)
                    }
                    QGCButton {
                        Layout.fillWidth: true
                        text:             qsTr("CLOSE")
                        backgroundColor:  _failColor
                        textColor:        "black"
                        onClicked:        _ctl.clawStep(-_ctl.bigStep)
                    }
                    QGCButton {
                        Layout.fillWidth:  true
                        Layout.columnSpan: 2
                        text:              qsTr("CENTER ROLL")
                        onClicked:         _ctl.rollCenter()
                    }
                    QGCButton {
                        Layout.fillWidth:  true
                        Layout.columnSpan: 2
                        text:              qsTr("SYNC SERVOS")
                        onClicked:         _ctl.resendServoPulses()
                    }
                }

                QGCCheckBox {
                    id:               keyboardCheckBox
                    Layout.fillWidth: true
                    text:             qsTr("Keyboard servo control (WASD / arrows)")
                    onClicked: {
                        if (checked) {
                            keyCatcher.forceActiveFocus()
                        }
                    }
                }

                QGCLabel {
                    Layout.fillWidth: true
                    visible:          keyboardCheckBox.checked
                    text:             keyCatcher.activeFocus
                                      ? qsTr("W/S fast claw · A/D fast roll · ↑↓ fine claw · ←→ fine roll")
                                      : qsTr("Click here to give keys back to the arm")
                    font.pointSize:   ScreenTools.smallFontPointSize
                    color:            keyCatcher.activeFocus ? _okColor : _warnColor
                    wrapMode:         Text.WordWrap

                    QGCMouseArea {
                        anchors.fill: parent
                        enabled:      keyboardCheckBox.checked && !keyCatcher.activeFocus
                        onClicked:    keyCatcher.forceActiveFocus()
                    }
                }

                Item {
                    id:      keyCatcher
                    width:   1
                    height:  1
                    enabled: keyboardCheckBox.checked

                    Keys.onPressed: (event) => {
                        switch (event.key) {
                        case Qt.Key_W:      _ctl.clawStep(-_ctl.bigStep);   break   // fast close claw
                        case Qt.Key_S:      _ctl.clawStep(_ctl.bigStep);    break   // fast open claw
                        case Qt.Key_A:      _ctl.rollStep(-_ctl.bigStep);   break   // fast rotate left
                        case Qt.Key_D:      _ctl.rollStep(_ctl.bigStep);    break   // fast rotate right
                        case Qt.Key_Up:     _ctl.clawStep(-_ctl.smallStep); break   // slow close claw
                        case Qt.Key_Down:   _ctl.clawStep(_ctl.smallStep);  break   // slow open claw
                        case Qt.Key_Left:   _ctl.rollStep(-_ctl.smallStep); break   // slow rotate left
                        case Qt.Key_Right:  _ctl.rollStep(_ctl.smallStep);  break   // slow rotate right
                        default:            return
                        }
                        event.accepted = true
                    }
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: qgcPal.groupBorder }

                // Log ----------------------------------------------------
                QGCLabel {
                    text:           qsTr("LINK LOG")
                    font.bold:      true
                    font.pointSize: ScreenTools.smallFontPointSize
                    color:          _accent
                }

                Rectangle {
                    Layout.fillWidth:       true
                    Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 7
                    color:                  Qt.darker(qgcPal.window, 1.35)
                    radius:                 ScreenTools.defaultBorderRadius
                    border.color:           qgcPal.groupBorder
                    border.width:           1
                    clip:                   true

                    ListView {
                        id:                 logView
                        anchors.fill:       parent
                        anchors.margins:    _margins / 2
                        model:              _ctl.logLines
                        clip:               true
                        spacing:            1

                        delegate: QGCLabel {
                            width:          logView.width
                            text:           modelData
                            font.pointSize: ScreenTools.smallFontPointSize
                            font.family:    ScreenTools.fixedFontFamily
                            wrapMode:       Text.NoWrap
                            elide:          Text.ElideRight
                        }

                        onCountChanged: positionViewAtEnd()
                    }
                }
            }
        }
    }
}
