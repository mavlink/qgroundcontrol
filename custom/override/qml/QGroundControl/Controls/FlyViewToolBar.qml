/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import QGroundControl
import QGroundControl.Controls

Rectangle {
    id:     control
    width:  parent.width
    height: ScreenTools.toolbarHeight
    color:  "transparent"

    property var    _activeVehicle:     QGroundControl.multiVehicleManager.activeVehicle
    property bool   _communicationLost: _activeVehicle ? _activeVehicle.vehicleLinkManager.communicationLost : false
    property color  _mainStatusBGColor: qgcPal.brandingPurple
    property real   _leftRightMargin:   ScreenTools.defaultFontPixelWidth * 0.75
    property var    _computersManager:  _activeVehicle ? _activeVehicle.autopilotPlugin.onboardComputersManager : false

    function dropMainStatusIndicatorTool() {
        mainStatusIndicator.dropMainStatusIndicator();
    }

    QGCPalette { id: qgcPal }

    /// Bottom single pixel divider
    Rectangle {
        anchors.left:   parent.left
        anchors.right:  parent.right
        anchors.bottom: parent.bottom
        height:         1
        color:          qgcPal.toolbarDivider
    }

    Rectangle {
        id:             gradientBackground
        anchors.top:    parent.top
        anchors.bottom: parent.bottom
        anchors.left:   parent.left
        width:          mainStatusLayout.width
        opacity:        qgcPal.windowTransparent.a
        
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 0; color: _mainStatusBGColor }
            //GradientStop { position: qgcButton.x + qgcButton.width; color: _mainStatusBGColor }
            GradientStop { position: 1; color: qgcPal.window }
        }
    }

    Rectangle {
        anchors.top:    parent.top
        anchors.bottom: parent.bottom
        anchors.left:   gradientBackground.right
        anchors.right:  parent.right
        color:          qgcPal.windowTransparent
    }

    RowLayout {
        id:                     mainLayout
        anchors.bottomMargin:   1
        anchors.rightMargin:    control._leftRightMargin
        anchors.top:            parent.top
        anchors.bottom:         parent.bottom
        anchors.left:           parent.left
        anchors.right:          parent.right
        spacing:                ScreenTools.defaultFontPixelWidth

        RowLayout {
            id:                 leftStatusLayout
            Layout.fillHeight:  true
            Layout.alignment:   Qt.AlignLeft
            spacing:            ScreenTools.defaultFontPixelWidth * 2

            RowLayout {
                id:                 mainStatusLayout
                Layout.fillHeight:  true
                spacing:            0

                QGCToolBarButton {
                    id:                 qgcButton
                    Layout.fillHeight:  true
                    icon.source:        "/res/QGCLogoFull.svg"
                    logo:               true
                    onClicked:          mainWindow.showToolSelectDialog()
                }

                MainStatusIndicator {
                    id:                 mainStatusIndicator
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
                Layout.fillHeight:  true
                visible:            _activeVehicle
            }

            QGCLabel{
                // Layout.fillHeight:  true
                Layout.alignment:   Qt.AlignVCenter
                font.pointSize:     ScreenTools.defaultFontPointSize
                font.family:        ScreenTools.normalFontFamily
                font.weight:        Font.Normal
                text: qsTr("v"+QGroundControl.corePlugin.version)
            }
        }

        QGCFlickable {
            id:                     indicatorsFlickable
            Layout.alignment:       Qt.AlignRight
            Layout.fillHeight:      true
            Layout.preferredWidth:  Math.min(contentWidth, availableWidth)
            contentWidth:           toolIndicators.width
            flickableDirection:     Flickable.HorizontalFlick

            property real availableWidth: mainLayout.width - leftStatusLayout.width

            FlyViewToolBarIndicators { 
                id: toolIndicators 
                anchors.right: foxFourLogo.visible? foxFourLogo.left: parent.right
            }
        }
        ParameterDownloadProgress {
            anchors.fill: parent
        }
    }



    Image {
        id:foxFourLogo
        anchors.right:          parent.right
        anchors.top:            parent.top
        anchors.bottom:         parent.bottom
        anchors.margins:        ScreenTools.defaultFontPixelHeight * 0.66
        source:                 _outdoorPalette ? "/custom/img/FoxFourTextLogo_dark.svg" : "/custom/img/FoxFourTextLogo_light.svg"
        visible:                _computersManager &&
                                // checkForVGM(_computersManager.computersInfo) &&
                                x > (indicatorsFlickable.x + indicatorsFlickable.contentWidth + ScreenTools.defaultFontPixelWidth)
        mipmap:                 true
        fillMode:               Image.PreserveAspectFit

        property bool   _outdoorPalette:        qgcPal.globalTheme === QGCPalette.Light

        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                console.log(qsTr("Component ") + _computersManager.currentComputerComponent)
                showVehicleConfigParametersPageComponent(qsTr("Component ") + _computersManager.currentComputerComponent)
            }
        }
    }
}
