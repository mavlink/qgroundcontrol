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
    id:     _root
    width:  parent.width
    height: ScreenTools.toolbarHeight
    color:  "transparent"

    property var    _activeVehicle:     QGroundControl.multiVehicleManager.activeVehicle
    property bool   _communicationLost: _activeVehicle ? _activeVehicle.vehicleLinkManager.communicationLost : false
    property color  _mainStatusBGColor: qgcPal.brandingPurple

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
        color:          "black"
        visible:        qgcPal.globalTheme === QGCPalette.Light
    }

    Rectangle {
        anchors.top:    parent.top
        anchors.bottom: parent.bottom
        anchors.left:   parent.left
        width:          leftStatusLayout.width
        
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 0;                                     color: _mainStatusBGColor }
            GradientStop { position: qgcButton.x + qgcButton.width; color: _mainStatusBGColor }
            GradientStop { position: 1;                                     color: _root.color }
        }
    }

    RowLayout {
        id:                     mainLayout
        anchors.bottomMargin:   1
        anchors.top:            parent.top
        anchors.bottom:         parent.bottom
        anchors.left:           parent.left
        anchors.right:          brandingLogo.visible ? brandingLogo.left : parent.right
        anchors.rightMargin:    brandingLogo.anchors.margins
        spacing:                ScreenTools.defaultFontPixelWidth

        RowLayout {
            id:                 leftStatusLayout
            Layout.fillHeight:  true
            Layout.alignment:   Qt.AlignLeft
            spacing:            ScreenTools.defaultFontPixelWidth * 2

            RowLayout {
                id:                 mainStatusLayout
                Layout.fillHeight:  true
                spacing:            ScreenTools.defaultFontPixelWidth / 2

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

                QGCButton {
                    id:                 disconnectButton
                    text:               qsTr("Disconnect")
                    onClicked:          _activeVehicle.closeVehicle()
                    visible:            _activeVehicle && _communicationLost
                }
            }

            FlightModeIndicator {
                Layout.fillHeight:  true
                visible:            _activeVehicle
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

            FlyViewToolBarIndicators { id: toolIndicators }
        }
    }

    Image {
        id:                 brandingLogo
        anchors.margins:    ScreenTools.defaultFontPixelHeight * 0.66
        anchors.top:        parent.top
        anchors.bottom:     parent.bottom
        anchors.right:      parent.right
        fillMode:           Image.PreserveAspectFit
        source:             _outdoorPalette ? _brandImageOutdoor : _brandImageIndoor
        mipmap:             true
        visible:            _showBranding

        property bool _showBranding: leftStatusLayout.width + indicatorsFlickable.contentWidth + width + (anchors.margins * 3) < _root.width

        property bool   _outdoorPalette:        qgcPal.globalTheme === QGCPalette.Light
        property bool   _corePluginBranding:    QGroundControl.corePlugin.brandImageIndoor.length != 0
        property string _userBrandImageIndoor:  QGroundControl.settingsManager.brandImageSettings.userBrandImageIndoor.value
        property string _userBrandImageOutdoor: QGroundControl.settingsManager.brandImageSettings.userBrandImageOutdoor.value
        property bool   _userBrandingIndoor:    QGroundControl.settingsManager.brandImageSettings.visible && _userBrandImageIndoor.length != 0
        property bool   _userBrandingOutdoor:   QGroundControl.settingsManager.brandImageSettings.visible && _userBrandImageOutdoor.length != 0
        property string _brandImageIndoor:      brandImageIndoor()
        property string _brandImageOutdoor:     brandImageOutdoor()

        function brandImageIndoor() {
            if (_userBrandingIndoor) {
                return _userBrandImageIndoor
            } else {
                if (_userBrandingOutdoor) {
                    return _userBrandImageOutdoor
                } else {
                    if (_corePluginBranding) {
                        return QGroundControl.corePlugin.brandImageIndoor
                    } else {
                        return _activeVehicle ? _activeVehicle.brandImageIndoor : ""
                    }
                }
            }
        }

        function brandImageOutdoor() {
            if (_userBrandingOutdoor) {
                return _userBrandImageOutdoor
            } else {
                if (_userBrandingIndoor) {
                    return _userBrandImageIndoor
                } else {
                    if (_corePluginBranding) {
                        return QGroundControl.corePlugin.brandImageOutdoor
                    } else {
                        return _activeVehicle ? _activeVehicle.brandImageOutdoor : ""
                    }
                }
            }
        }
    }

    ParameterDownloadProgress {
        anchors.fill: parent
    }
}
