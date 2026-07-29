import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtQuick.Effects

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlyView
import QGroundControl.SynclairVisionUI


Item {
    required property var guidedValueSlider
    property var flyView   // Reference to the parent FlyView, set by whoever instantiates this toolbar

    id:     control
    width:  parent.width
    height: ScreenTools.toolbarHeight

    property var    _activeVehicle:     QGroundControl.multiVehicleManager.activeVehicle
    property bool   _communicationLost: _activeVehicle ? _activeVehicle.vehicleLinkManager.communicationLost : false
    property color  _mainStatusBGColor: qgcPal.brandingPurple
    property real   _leftRightMargin:   ScreenTools.defaultFontPixelWidth * 0.75
    property var    _guidedController:  globals.guidedControllerFlyView
    readonly property string _toolbarLogoSource: SVState.synclairOverlay ? "/res/resources/svlogo.png" : "/res/QGCLogoFull.svg"


    function dropMainStatusIndicatorTool() {
        mainStatusIndicator.dropMainStatusIndicator();
    }

    QGCPalette { id: qgcPal }

    QGCFlickable {
        anchors.fill:       parent
        contentWidth:       toolBarLayout.width
        flickableDirection: Flickable.HorizontalFlick

        Row {
            id:         toolBarLayout
            height:     parent.height
            spacing:    0

            Item {
                id:     leftPanel
                width:  leftPanelLayout.implicitWidth
                height: parent.height

                // Gradient background behind Q button and main status indicator
                Rectangle {
                    id:         gradientBackground
                    height:     parent.height
                    width:      mainStatusLayout.width
                    opacity:    qgcPal.windowTransparent.a

                    gradient: Gradient {
                        orientation: Gradient.Horizontal
                        GradientStop { position: 0; color: _mainStatusBGColor }
                        //GradientStop { position: qgcButton.x + qgcButton.width; color: _mainStatusBGColor }
                        GradientStop { position: 1; color: qgcPal.window }
                    }
                }

                // Standard toolbar background to the right of the gradient
                Rectangle {
                    anchors.left:   gradientBackground.right
                    anchors.right:  parent.right
                    height:         parent.height
                    color:          qgcPal.windowTransparent
                }

                RowLayout {
                    id:         leftPanelLayout
                    height:     parent.height
                    spacing:    ScreenTools.defaultFontPixelWidth * 2

                    RowLayout {
                        id:         mainStatusLayout
                        height:     parent.height
                        spacing:    0

                        QGCToolBarButton {
                            id:                 qgcButton
                            objectName:         "toolbar_qgcLogo"
                            Layout.fillHeight:  true
                            icon.source: control._toolbarLogoSource

                            logo:               true
                            onClicked:          mainWindow.showToolSelectDialog()
                        }



                        MainStatusIndicator {
                            id:                 mainStatusIndicator
                            objectName:         "toolbar_mainStatusIndicator"
                            Layout.fillHeight:  true
                        }
                    }

                    QGCButton {
                        id:         disconnectButton
                        text:       qsTr("Disconnect")
                        onClicked:  _activeVehicle.closeVehicle()
                        visible:    _activeVehicle && _communicationLost
                    }

                    FlightModeIndicator {
                        objectName:         "toolbar_flightModeIndicator"
                        Layout.fillHeight:  true
                        visible:            _activeVehicle
                    }


                }
            }
            Item {
                id:     centerPanel
                // center panel takes up all remaining space in toolbar between left and right panels
                width:  Math.max(guidedActionConfirm.visible ? guidedActionConfirm.width : 0, control.width - (leftPanel.width + rightPanel.width))
                height: parent.height

                Rectangle {
                    anchors.fill:   parent
                    color:          qgcPal.windowTransparent
                }

                GuidedActionConfirm {
                    id:                         guidedActionConfirm
                    height:                     parent.height
                    anchors.horizontalCenter:   parent.horizontalCenter
                    guidedController:           control._guidedController
                    guidedValueSlider:          control.guidedValueSlider
                    messageDisplay:             guidedActionMessageDisplay
                }
            }

            Item {
                id:     rightPanel
                width:  flyViewIndicators.width
                height: parent.height

                Rectangle {
                    anchors.fill:   parent
                    color:          qgcPal.windowTransparent
                }

                FlyViewToolBarIndicators {
                    id:     flyViewIndicators
                    height: parent.height
                }

                Item {
                    id:     overlayButtonWrapper
                    anchors.right: flyViewIndicators.right
                    anchors.rightMargin: _margins * 2
                    anchors.verticalCenter: parent.verticalCenter
                    width:  overlayButton.width
                    height: overlayButton.height

                    // True as long as the SV welcome prompt has NOT been closed/acked yet
                    readonly property bool _svWelcomeNotYetShown: {
                        var shownIds = QGroundControl.settingsManager.appSettings.firstRunPromptIdsShown.rawValue
                        return !shownIds.includes(QGroundControl.corePlugin.svInitialWelcomePromptId)
                    }

                    // Glow sits BEHIND and OUTSIDE overlayButton — sibling, not child
                    MultiEffect {
                        id:             overlayGlow
                        anchors.fill:   overlayButton
                        source:         overlayButton
                        z:              -1
                        visible:        overlayButtonWrapper._svWelcomeNotYetShown && !SVState.synclairOverlay

                        shadowEnabled:  true
                        shadowColor:    "yellow"
                        shadowScale:    1.0
                        shadowBlur:     1.0
                        shadowHorizontalOffset: 0
                        shadowVerticalOffset:   0

                        SequentialAnimation {
                            running: overlayGlow.visible
                            loops:   Animation.Infinite

                            NumberAnimation {
                                target:     overlayGlow
                                property:   "shadowOpacity"
                                from:       0
                                to:         1
                                duration:   1000
                                easing.type: Easing.InOutSine
                            }
                            NumberAnimation {
                                target:     overlayGlow
                                property:   "shadowOpacity"
                                from:       1
                                to:         0
                                duration:   1000
                                easing.type: Easing.InOutSine
                            }
                        }
                    }

                    QGCCheckBoxSlider {
                        id: overlayButton
                        text: "Synclair Vision: QGroundControl"
                        checked: SVState.synclairOverlay

                        onCheckedChanged: {
                            SVState.synclairOverlay = checked

                            if (checked) {
                                var appSettings = QGroundControl.settingsManager.appSettings
                                var shownIds = appSettings.firstRunPromptIdsShown.rawValue
                                var promptId = QGroundControl.corePlugin.svInitialWelcomePromptId

                                // ONLY RUNS THE FIRST TIME EVER
                                if (!shownIds.includes(promptId)) {

                                    // 1. Swap main screen to Video/Overlay BEFORE prompting
                                    if (control.flyView && control.flyView.showVideoFullScreen) {
                                        control.flyView.showVideoFullScreen()
                                    }

                                    // 2. Open the welcome prompt
                                    welcomePromptLoader.active = true
                                }
                            }
                        }

                        Loader {
                            id: welcomePromptLoader
                            active: false
                            source: "qrc:/qml/QGroundControl/SynclairVisionUI/Flyview/SVWelcomePrompt.qml"

                            onLoaded: {
                                item.open()
                            }
                        }
                    }
                }
                
            }
        }
    }

    // The guided action message display is outside of the GuidedActionConfirm control so that it doesn't end up as
    // part of the Flickable
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
            id:         messageLabel
            x:          _margins
            y:          _margins
            width:      ScreenTools.defaultFontPixelWidth * 30
            wrapMode:   Text.WordWrap
            text:       guidedActionConfirm.message
        }

        

        

        PropertyAnimation {
            id:         messageOpacityAnimation
            target:     guidedActionMessageDisplay
            property:   "opacity"
            from:       1
            to:         0
            duration:   500
        }

        Timer {
            id:             messageFadeTimer
            interval:       4000
            onTriggered:    messageOpacityAnimation.start()
        }
    }

    ParameterDownloadProgress {
        anchors.fill: parent
    }



    


}