import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlyView

Item {
    required property var guidedValueSlider

    id:     control
    width:  parent.width
    height: ScreenTools.toolbarHeight

    property var    _activeVehicle:     QGroundControl.multiVehicleManager.activeVehicle
    property bool   _communicationLost: _activeVehicle ? _activeVehicle.vehicleLinkManager.communicationLost : false
    property real   _leftRightMargin:   ScreenTools.defaultFontPixelWidth * 0.75
    property var    _guidedController:  globals.guidedControllerFlyView

    function dropMainStatusIndicatorTool() {
        mainStatusIndicator.dropMainStatusIndicator();
    }

    QGCPalette { id: qgcPal }

    // ---------------------------------------------------------------------
    // ATLAS toolbar background
    // Blue -> dark -> orange
    // ---------------------------------------------------------------------
    Rectangle {
        anchors.fill: parent
        z: -1

        gradient: Gradient {
            orientation: Gradient.Horizontal

            GradientStop {
                position: 0.00
                color: "#5793BC"
            }

            GradientStop {
                position: 0.32
                color: "#263640"
            }

            GradientStop {
                position: 0.50
                color: "#101112"
            }

            GradientStop {
                position: 0.58
                color: "#0C0C0C"
            }

            GradientStop {
                position: 0.72
                color: "#54340E"
            }

            GradientStop {
                position: 1.00
                color: "#F28A08"
            }
        }
    }

    QGCFlickable {
        anchors.fill:       parent
        contentWidth:       toolBarLayout.width
        flickableDirection: Flickable.HorizontalFlick

        Row {
            id:      toolBarLayout
            height:  parent.height
            spacing: 0

            // -----------------------------------------------------------------
            // LEFT
            // -----------------------------------------------------------------
            Item {
                id:     leftPanel
                width:  leftPanelLayout.implicitWidth
                height: parent.height

                Rectangle {
                    anchors.fill: parent
                    color: "transparent"
                }

                RowLayout {
                    id:      leftPanelLayout
                    height:  parent.height
                    spacing: ScreenTools.defaultFontPixelWidth * 2

                    RowLayout {
                        id:      mainStatusLayout
                        height:  parent.height
                        spacing: 0

                        Item {
                            id:                 atlasLogoButton
                            objectName:         "toolbar_atlasLogo"
                            Layout.fillHeight:  true
                            Layout.preferredWidth: 150

                            Image {
                                anchors.fill: parent
                                anchors.leftMargin:   10
                                anchors.rightMargin:  10
                                anchors.topMargin:    5
                                anchors.bottomMargin: 5

                                source: "qrc:/atlas/atlas_logo_white.svg"

                                fillMode: Image.PreserveAspectFit
                                smooth: true
                                asynchronous: true
                            }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: mainWindow.showToolSelectDialog()
                            }
                        }

                        // Mantemos o objeto porque outras partes do QGC
                        // ainda o referenciam, mas não o mostramos.
                        MainStatusIndicator {
                            id:                 mainStatusIndicator
                            objectName:         "toolbar_mainStatusIndicator"
                            Layout.fillHeight:  true
                            visible:            false
                        }
                    }

                    QGCButton {
                        id:        disconnectButton
                        text:      qsTr("Disconnect")
                        onClicked: _activeVehicle.closeVehicle()
                        visible:   _activeVehicle && _communicationLost
                    }

                    FlightModeIndicator {
                        objectName:        "toolbar_flightModeIndicator"
                        Layout.fillHeight: true
                        visible:           _activeVehicle
                    }
                }
            }

            // -----------------------------------------------------------------
            // CENTER
            // -----------------------------------------------------------------
            Item {
                id: centerPanel

                width: Math.max(
                    guidedActionConfirm.visible ? guidedActionConfirm.width : 0,
                    control.width - (leftPanel.width + rightPanel.width)
                )

                height: parent.height

                Rectangle {
                    anchors.fill: parent
                    color: "transparent"
                }

                GuidedActionConfirm {
                    id:                       guidedActionConfirm
                    height:                   parent.height
                    anchors.horizontalCenter: parent.horizontalCenter
                    guidedController:         control._guidedController
                    guidedValueSlider:        control.guidedValueSlider
                    messageDisplay:           guidedActionMessageDisplay
                }
            }

            // -----------------------------------------------------------------
            // RIGHT
            // -----------------------------------------------------------------
            Item {
                id:     rightPanel
                width:  flyViewIndicators.width
                height: parent.height

                Rectangle {
                    anchors.fill: parent
                    color: "transparent"
                }

                FlyViewToolBarIndicators {
                    id:     flyViewIndicators
                    height: parent.height
                }
            }
        }
    }

    // ---------------------------------------------------------------------
    // Guided action message
    // ---------------------------------------------------------------------
    Rectangle {
        id:                         guidedActionMessageDisplay
        anchors.top:                control.bottom
        anchors.topMargin:          _margins
        x:                          control.mapFromItem(guidedActionConfirm.parent, guidedActionConfirm.x, 0).x + (guidedActionConfirm.width - guidedActionMessageDisplay.width) / 2
        width:                      messageLabel.contentWidth + (_margins * 2)
        height:                     messageLabel.contentHeight + (_margins * 2)
        color:                      qgcPal.windowTransparent
        radius:                     ScreenTools.defaultBorderRadius
        visible:                    guidedActionConfirm.visible

        QGCLabel {
            id:       messageLabel
            x:        _margins
            y:        _margins
            width:    ScreenTools.defaultFontPixelWidth * 30
            wrapMode: Text.WordWrap
            text:     guidedActionConfirm.message
        }

        PropertyAnimation {
            id:       messageOpacityAnimation
            target:   guidedActionMessageDisplay
            property: "opacity"
            from:     1
            to:       0
            duration: 500
        }

        Timer {
            id:          messageFadeTimer
            interval:    4000
            onTriggered: messageOpacityAnimation.start()
        }
    }

    ParameterDownloadProgress {
        anchors.fill: parent
    }
}
