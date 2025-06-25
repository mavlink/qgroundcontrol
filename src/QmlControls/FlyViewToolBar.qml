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
import QGroundControl.Palette
import QGroundControl.MultiVehicleManager
import QGroundControl.ScreenTools
import QGroundControl.Controllers

Rectangle {
    id: _root
    width: parent.width
    height: ScreenTools.toolbarHeight
    anchors.top: parent.top
    color: "transparent"

    property var _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
    property bool _communicationLost: _activeVehicle ? _activeVehicle.vehicleLinkManager.communicationLost : false
    property color _mainStatusBGColor: qgcPal.brandingPurple

    function dropMainStatusIndicatorTool() {
        mainStatusIndicator.dropMainStatusIndicator();
    }

    // hình ở header
    // Đặt hình ảnh nền chiếm toàn bộ diện tích của _root

    QGCPalette {
        id: qgcPal
    }

    /// Bottom single pixel divider
    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 1
        color: "black"
        visible: qgcPal.globalTheme === QGCPalette.Light
    }

    Rectangle {
                width: parent.width
                height: parent.height * 1.2                    // Mỏng lại, ví dụ 10px
                anchors.top: parent.top        // Gắn vào phía trên
                gradient: Gradient {
                    orientation: Gradient.Vertical
                    GradientStop {
                        position: 0.0
                        color: "#b5000000"
                    } // Đen với alpha 50%
                    GradientStop {
                        position: 1.0
                        color: "#00000000"
                    } // Trong suốt
                }
            } 

    Item {
        id: mainStatusIndicatorItem
        anchors.fill: parent
        z:2
        QGCLabel {
            id: mainStatusLabel
            horizontalAlignment: Text.AlignHCenter
            anchors.centerIn: parent 
            verticalAlignment: Text.AlignVCenter
            text: mainStatusIndicator.mainStatusText()
            
            font.pointSize: ScreenTools.largeFontPointSize

            QGCMouseArea {
                id: mainStatusMouseArea
                anchors.fill: parent
                onClicked: mainStatusIndicator.dropMainStatusIndicator()
            }
        }
        // rectangle {
        //     anchors.fill: parent
        //     color: mainStatusIndicator._mainStatusBGColor
        //     opacity: 0.5
        //     visible: mainStatusIndicator.showMainStatusIndicator()
        // }
    }
    //  Cụm nút bấm bên trái và giữa
    RowLayout {
        id: viewButtonRow
        anchors.bottomMargin: 1
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.leftMargin: 10
        spacing:                ScreenTools.defaultFontPixelWidth / 2

        Rectangle {
            id: buttonContainer
            color: mouseArea.pressed ? "#c80066c5" : (qgcPal.globalTheme === QGCPalette.Light ? "#c8ffffff" : "#c8000000")
            radius: 10
            border.color: "#a8616161"
            border.width:  1
            Layout.alignment: Qt.AlignVCenter
            height: viewButtonRow.height * 0.8
            width: currentButton.implicitWidth + 4
            

            QGCToolBarButton {
                id: currentButton
                anchors.centerIn: parent
                icon.source: "/res/QGCLogoFull.svg"
                logo: true
            }
            Behavior on color {
                ColorAnimation { duration: 150 }
            }
            MouseArea {
                id: mouseArea
                anchors.fill: parent
                onClicked: {
                    mainWindow.showToolSelectDialog()
                }
            }
        }



        MainStatusIndicator {
            id: mainStatusIndicator
            Layout.preferredHeight: viewButtonRow.height
        }
        QGCButton {
            id: disconnectButton
            text: qsTr("Disconnect")
            onClicked: _activeVehicle.closeVehicle()
            visible: _activeVehicle && _communicationLost
        }
    }

    QGCFlickable {
        id: toolsFlickable
        anchors.leftMargin: ScreenTools.defaultFontPixelWidth * ScreenTools.largeFontPointRatio * 1.5
        anchors.rightMargin: ScreenTools.defaultFontPixelWidth / 2
        anchors.left: viewButtonRow.right
        anchors.bottomMargin: 1
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        contentWidth: toolIndicators.width
        flickableDirection: Flickable.HorizontalFlick

        FlyViewToolBarIndicators {
            id: toolIndicators
        }
    }

    //-------------------------------------------------------------------------
    //-- Branding Logo
    Image {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.margins: ScreenTools.defaultFontPixelHeight * 0.66
        visible: _activeVehicle && !_communicationLost && x > (toolsFlickable.x + toolsFlickable.contentWidth + ScreenTools.defaultFontPixelWidth)
        fillMode: Image.PreserveAspectFit
        source: _outdoorPalette ? _brandImageOutdoor : _brandImageIndoor
        mipmap: true

        property bool _outdoorPalette: qgcPal.globalTheme === QGCPalette.Light
        property bool _corePluginBranding: QGroundControl.corePlugin.brandImageIndoor.length != 0
        property string _userBrandImageIndoor: QGroundControl.settingsManager.brandImageSettings.userBrandImageIndoor.value
        property string _userBrandImageOutdoor: QGroundControl.settingsManager.brandImageSettings.userBrandImageOutdoor.value
        property bool _userBrandingIndoor: QGroundControl.settingsManager.brandImageSettings.visible && _userBrandImageIndoor.length != 0
        property bool _userBrandingOutdoor: QGroundControl.settingsManager.brandImageSettings.visible && _userBrandImageOutdoor.length != 0
        property string _brandImageIndoor: brandImageIndoor()
        property string _brandImageOutdoor: brandImageOutdoor()

        function brandImageIndoor() {
            if (_userBrandingIndoor) {
                return _userBrandImageIndoor;
            } else {
                if (_userBrandingOutdoor) {
                    return _userBrandImageOutdoor;
                } else {
                    if (_corePluginBranding) {
                        return QGroundControl.corePlugin.brandImageIndoor;
                    } else {
                        return _activeVehicle ? _activeVehicle.brandImageIndoor : "";
                    }
                }
            }
        }

        function brandImageOutdoor() {
            if (_userBrandingOutdoor) {
                return _userBrandImageOutdoor;
            } else {
                if (_userBrandingIndoor) {
                    return _userBrandImageIndoor;
                } else {
                    if (_corePluginBranding) {
                        return QGroundControl.corePlugin.brandImageOutdoor;
                    } else {
                        return _activeVehicle ? _activeVehicle.brandImageOutdoor : "";
                    }
                }
            }
        }
    }

    // Small parameter download progress bar
    Rectangle {
        anchors.top: parent.top
        height: _root.height * 0.05
        width: _activeVehicle ? _activeVehicle.loadProgress * parent.width : 0
        color: qgcPal.colorGreen
        visible: !largeProgressBar.visible
    }

    // Large parameter download progress bar
    Rectangle {
        id: largeProgressBar
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: parent.height
        color: qgcPal.window
        visible: _showLargeProgress

        property bool _initialDownloadComplete: _activeVehicle ? _activeVehicle.initialConnectComplete : true
        property bool _userHide: false
        property bool _showLargeProgress: !_initialDownloadComplete && !_userHide && qgcPal.globalTheme === QGCPalette.Light

        Connections {
            target: QGroundControl.multiVehicleManager
            function onActiveVehicleChanged(activeVehicle) {
                largeProgressBar._userHide = false;
            }
        }

        Rectangle {
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: _activeVehicle ? _activeVehicle.loadProgress * parent.width : 0
            color: qgcPal.colorGreen
        }

        QGCLabel {
            anchors.centerIn: parent
            text: qsTr("Downloading")
            font.pointSize: ScreenTools.largeFontPointSize
        }

        QGCLabel {
            anchors.margins: _margin
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            text: qsTr("Click anywhere to hide")

            property real _margin: ScreenTools.defaultFontPixelWidth / 2
        }

        MouseArea {
            anchors.fill: parent
            onClicked: largeProgressBar._userHide = true
        }
    }
}
