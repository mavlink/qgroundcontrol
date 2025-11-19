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
        color:          qgcPal.toolbarBackground
    }

    Rectangle {
        anchors.top:    parent.top
        anchors.bottom: parent.bottom
        anchors.left:   gradientBackground.right
        anchors.right:  parent.right
        color:          qgcPal.toolbarBackground
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
                    // onClicked handler removed - permanent sidebar is now always visible
                }

                ColumnLayout {
                    id:                 brandTitle
                    Layout.fillHeight:  true
                    Layout.alignment:   Qt.AlignVCenter
                    spacing:            ScreenTools.defaultFontPixelHeight * 0.10

                    QGCLabel {
                        id:                 appTitle
                        Layout.alignment:   Qt.AlignVCenter
                        text:               qsTr("IG GCS FLY")
                        color:              qgcPal.globalTheme === QGCPalette.Light ? qgcPal.text : qgcPal.brandingBlue
                        font.pointSize:     ScreenTools.largeFontPointSize
                        font.bold:          true
                    }

                    QGCLabel {
                        id:                 appTagline
                        Layout.alignment:   Qt.AlignVCenter
                        text:               qsTr("FOR THE NATION WITH PRECISION")
                        color:              qgcPal.globalTheme === QGCPalette.Light ? qgcPal.windowTransparentText : qgcPal.brandingBlue
                        opacity:            qgcPal.globalTheme === QGCPalette.Light ? 0.8 : 0.9
                        font.pointSize:     ScreenTools.smallFontPointSize
                    }
                }
            }

            QGCButton {
                id:         disconnectButton
                text:       qsTr("Disconnect")
                onClicked:  _activeVehicle.closeVehicle()
                visible:    _activeVehicle && _communicationLost
                neon:       true
                pill:       true
                neonColor:  qgcPal.colorRed
            }
        }

        Item {
            id:                     centerStatusHolder
            Layout.fillHeight:      true
            Layout.fillWidth:       true
            Layout.minimumWidth:    topStatusPanel.visible ? topStatusPanel.implicitWidth : 0

            FlyViewTopStatusPanel {
                id:                 topStatusPanel
                anchors.verticalCenter: parent.verticalCenter
                anchors.horizontalCenter: parent.horizontalCenter
                flightModeIndicator: flightModeProxy
                statusIndicator:     mainStatusIndicator
            }
        }

        QGCFlickable {
            id:                     indicatorsFlickable
            Layout.alignment:       Qt.AlignRight
            Layout.fillHeight:      true
            Layout.preferredWidth:  Math.min(contentWidth, availableWidth)
            contentWidth:           rightRow.width
            flickableDirection:     Flickable.HorizontalFlick

            property real availableWidth: Math.max(0, mainLayout.width - leftStatusLayout.width - centerStatusHolder.width)

            Row {
                id:                 rightRow
                height:             indicatorsFlickable.height
                spacing:            ScreenTools.defaultFontPixelWidth

                FlyViewToolBarIndicators { id: toolIndicators }

                MainStatusIndicator {
                    id:                 mainStatusIndicator
                    height:             indicatorsFlickable.height
                    showStatusLabel:    false
                }
            }

    // Hidden flight mode indicator used to provide drawer interactions for the top status panel
    FlightModeIndicator {
        id:         flightModeProxy
        visible:    false
        enabled:    false
    }
        }
    }

    ParameterDownloadProgress {
        anchors.fill: parent
    }
}
