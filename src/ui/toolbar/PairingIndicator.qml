/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick                  2.11
import QtQuick.Controls         2.4
import QtQuick.Layouts          1.11

import QGroundControl                       1.0
import QGroundControl.Controls              1.0
import QGroundControl.MultiVehicleManager   1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.Palette               1.0

//-------------------------------------------------------------------------
//-- GPS Indicator
Item {
    id:             _root
    width:          pairingRow.width * 1.1
    anchors.top:    parent.top
    anchors.bottom: parent.bottom
    property bool _light:           qgcPal.globalTheme === QGCPalette.Light && !activeVehicle
    property real _contentWidth:    ScreenTools.defaultFontPixelWidth  * 30
    property real _contentSpacing:  ScreenTools.defaultFontPixelHeight * 0.5
    Row {
        id:                     pairingRow
        spacing:                ScreenTools.defaultFontPixelWidth
        anchors.top:            parent.top
        anchors.bottom:         parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        Image {
            id:                 pairingIcon
            height:             parent.height
            width:              height
            source:             _light ? "/qmlimages/PairingIconLight.svg" : "/qmlimages/PairingIcon.svg"
            sourceSize.width:   width
            fillMode:           Image.PreserveAspectFit
            smooth:             true
            mipmap:             true
            antialiasing:       true
            anchors.verticalCenter: parent.verticalCenter
        }
        QGCLabel {
            text:               qsTr("Pair Vehicle")
            width:              !activeVehicle ? (ScreenTools.defaultFontPixelWidth * 12) : 0
            visible:            !activeVehicle
            font.pointSize:     ScreenTools.mediumFontPointSize
            font.family:        ScreenTools.demiboldFontFamily
            anchors.verticalCenter: parent.verticalCenter
        }
    }
    MouseArea {
        anchors.fill:           parent
        onClicked: {
            if(QGroundControl.pairingManager.pairedDeviceNameList.length > 1) {
                connectionPopup.open()
            } else {
                if(QGroundControl.pairingManager.pairingLinkTypeStrings.length > 1)
                    pairingPopup.open()
                else {
                    mhPopup.open()
                }
            }
        }
    }
    //-------------------------------------------------------------------------
    //-- Pairing
    Popup {
        id:                     pairingPopup
        width:                  pairingBody.width
        height:                 pairingBody.height
        modal:                  true
        focus:                  true
        parent:                 Overlay.overlay
        x:                      Math.round((mainWindow.width  - width)  * 0.5)
        y:                      Math.round((mainWindow.height - height) * 0.5)
        closePolicy:            Popup.CloseOnEscape | Popup.CloseOnPressOutside
        background: Rectangle {
            anchors.fill:       parent
            color:              qgcPal.globalTheme === QGCPalette.Light ? Qt.rgba(1,1,1,0.95) : Qt.rgba(0,0,0,0.75)
            radius:             ScreenTools.defaultFontPixelWidth * 0.25
        }
        Item {
            id:                 pairingBody
            width:              comboListCol.width  + (ScreenTools.defaultFontPixelWidth   * 8)
            height:             comboListCol.height + (ScreenTools.defaultFontPixelHeight  * 2)
            anchors.centerIn:   parent
            Column {
                id:                 comboListCol
                spacing:            _contentSpacing
                anchors.centerIn:   parent
                Item { width: 1; height: 1; }
                QGCLabel {
                    text:           qsTr("Pair New Vehicle")
                    font.pointSize: ScreenTools.mediumFontPointSize
                    font.family:    ScreenTools.demiboldFontFamily
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                Item { width: 1; height: 1; }
                Rectangle {
                    width:          ScreenTools.defaultFontPixelWidth  * 40
                    height:         ScreenTools.defaultFontPixelHeight * 8
                    color:          Qt.rgba(0,0,0,0)
                    border.color:   qgcPal.text
                    border.width:   1
                    anchors.horizontalCenter: parent.horizontalCenter
                    QGCLabel {
                        text:       "Graphic"
                        anchors.centerIn: parent
                    }
                }
                Item { width: 1; height: 1; }
                QGCLabel {
                    text:           qsTr("Please choose a pairing method in order to connect to a nearby device")
                    width:          _contentWidth
                    wrapMode:       Text.WordWrap
                    horizontalAlignment: Text.AlignHCenter
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                Item { width: 1; height: 1; }
                Repeater {
                    model:          QGroundControl.pairingManager.pairingLinkTypeStrings
                    delegate:       QGCButton {
                        text:       modelData
                        width:      _contentWidth
                        anchors.horizontalCenter: parent.horizontalCenter
                        onClicked: {
                            pairingPopup.close()
                            if (index === QGroundControl.pairingManager.nfcIndex) {
                                nfcPopup.open()
                            } else if (index === QGroundControl.pairingManager.microhardIndex) {
                                mhPopup.open()
                            }
                        }
                    }
                }
                Item { width: 1; height: 1; }
            }
        }
    }
    //-------------------------------------------------------------------------
    //-- Microhard
    Popup {
        id:                     mhPopup
        width:                  mhBody.width
        height:                 mhBody.height
        modal:                  true
        focus:                  true
        parent:                 Overlay.overlay
        x:                      Math.round((mainWindow.width  - width)  * 0.5)
        y:                      Math.round((mainWindow.height - height) * 0.5)
        closePolicy:            Popup.CloseOnEscape | Popup.CloseOnPressOutside
        background: Rectangle {
            anchors.fill:       parent
            color:              qgcPal.globalTheme === QGCPalette.Light ? Qt.rgba(1,1,1,0.95) : Qt.rgba(0,0,0,0.75)
            radius:             ScreenTools.defaultFontPixelWidth * 0.25
        }
        Item {
            id:                 mhBody
            width:              mhCol.width  + (ScreenTools.defaultFontPixelWidth   * 8)
            height:             mhCol.height + (ScreenTools.defaultFontPixelHeight  * 2)
            anchors.centerIn:   parent
            Column {
                id:                 mhCol
                spacing:            _contentSpacing
                anchors.centerIn:   parent
                Item { width: 1; height: 1; }
                QGCLabel {
                    text:           qsTr("Pair New Vehicle")
                    font.pointSize: ScreenTools.mediumFontPointSize
                    font.family:    ScreenTools.demiboldFontFamily
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                Item { width: 1; height: 1; }
                QGCLabel {
                    text:           qsTr("To connect to your vehicle, please click 3 times on the button in order to put the vehicle in a discovery mode")
                    width:          _contentWidth
                    wrapMode:       Text.WordWrap
                    horizontalAlignment: Text.AlignHCenter
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                Item { width: 1; height: 1; }
                Rectangle {
                    width:          ScreenTools.defaultFontPixelWidth  * 40
                    height:         ScreenTools.defaultFontPixelHeight * 8
                    color:          Qt.rgba(0,0,0,0)
                    border.color:   qgcPal.text
                    border.width:   1
                    anchors.horizontalCenter: parent.horizontalCenter
                    QGCLabel {
                        text:       "Graphic"
                        anchors.centerIn: parent
                    }
                }
                Item { width: 1; height: 1; }
                QGCButton {
                    text:           qsTr("Pair Via Microhard")
                    width:          _contentWidth
                    anchors.horizontalCenter: parent.horizontalCenter
                    onClicked: {
                        mhPopup.close()
                        connectionPopup.open()
                        QGroundControl.pairingManager.startMicrohardPairing();
                    }
                }
                Item { width: 1; height: 1; }
            }
        }
    }
    //-------------------------------------------------------------------------
    //-- NFC
    Popup {
        id:                     nfcPopup
        width:                  nfcBody.width
        height:                 nfcBody.height
        modal:                  true
        focus:                  true
        parent:                 Overlay.overlay
        x:                      Math.round((mainWindow.width  - width)  * 0.5)
        y:                      Math.round((mainWindow.height - height) * 0.5)
        closePolicy:            Popup.CloseOnEscape | Popup.CloseOnPressOutside
        background: Rectangle {
            anchors.fill:       parent
            color:              qgcPal.globalTheme === QGCPalette.Light ? Qt.rgba(1,1,1,0.95) : Qt.rgba(0,0,0,0.75)
            radius:             ScreenTools.defaultFontPixelWidth * 0.25
        }
        Item {
            id:                 nfcBody
            width:              nfcCol.width  + (ScreenTools.defaultFontPixelWidth   * 8)
            height:             nfcCol.height + (ScreenTools.defaultFontPixelHeight  * 2)
            anchors.centerIn:   parent
            Column {
                id:                 nfcCol
                spacing:            _contentSpacing
                anchors.centerIn:   parent
                Item { width: 1; height: 1; }
                QGCLabel {
                    text:           qsTr("Pair New Vehicle")
                    font.pointSize: ScreenTools.mediumFontPointSize
                    font.family:    ScreenTools.demiboldFontFamily
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                Item { width: 1; height: 1; }
                Rectangle {
                    width:          ScreenTools.defaultFontPixelWidth  * 40
                    height:         ScreenTools.defaultFontPixelHeight * 12
                    color:          Qt.rgba(0,0,0,0)
                    border.color:   qgcPal.text
                    border.width:   1
                    anchors.horizontalCenter: parent.horizontalCenter
                    QGCLabel {
                        text:       "Graphic"
                        anchors.centerIn: parent
                    }
                }
                Item { width: 1; height: 1; }
                QGCLabel {
                    text:           qsTr("Please make sure your vehicle is close to your Ground Station device")
                    width:          _contentWidth
                    wrapMode:       Text.WordWrap
                    horizontalAlignment: Text.AlignHCenter
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                Item { width: 1; height: 1; }
                QGCButton {
                    text:           qsTr("Pair Via NFC")
                    width:          _contentWidth
                    anchors.horizontalCenter: parent.horizontalCenter
                    onClicked: {
                        nfcPopup.close()
                        connectionPopup.open()
                        QGroundControl.pairingManager.startNFCScan();
                    }
                }
                Item { width: 1; height: 1; }
            }
        }
    }
    //-------------------------------------------------------------------------
    //-- Connection Manager
    Popup {
        id:                     connectionPopup
        width:                  connectionBody.width
        height:                 connectionBody.height
        modal:                  true
        focus:                  true
        parent:                 Overlay.overlay
        x:                      Math.round((mainWindow.width  - width)  * 0.5)
        y:                      Math.round((mainWindow.height - height) * 0.5)
        closePolicy:            cancelButton.visible ? Popup.NoAutoClose : (Popup.CloseOnEscape | Popup.CloseOnPressOutside)
        background: Rectangle {
            anchors.fill:       parent
            color:              qgcPal.globalTheme === QGCPalette.Light ? Qt.rgba(1,1,1,0.95) : Qt.rgba(0,0,0,0.75)
            radius:             ScreenTools.defaultFontPixelWidth * 0.25
        }
        Item {
            id:                 connectionBody
            width:              connectionCol.width  + (ScreenTools.defaultFontPixelWidth   * 8)
            height:             connectionCol.height + (ScreenTools.defaultFontPixelHeight  * 2)
            anchors.centerIn:   parent
            Column {
                id:                 connectionCol
                spacing:            _contentSpacing * 2
                anchors.centerIn:   parent
                Item { width: 1; height: 1; }
                QGCLabel {
                    text:           QGroundControl.pairingManager.pairingStatusStr
                    font.pointSize: ScreenTools.mediumFontPointSize
                    font.family:    ScreenTools.demiboldFontFamily
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                Item { width: 1; height: 1; }
                QGCColoredImage {
                    id:                 busyIndicator
                    height:             ScreenTools.defaultFontPixelHeight * 2
                    width:              height
                    source:             "/qmlimages/MapSync.svg"
                    sourceSize.height:  height
                    fillMode:           Image.PreserveAspectFit
                    mipmap:             true
                    smooth:             true
                    color:              qgcPal.text
                    visible:            cancelButton.visible
                    anchors.horizontalCenter: parent.horizontalCenter
                    RotationAnimation on rotation {
                        loops:          Animation.Infinite
                        from:           360
                        to:             0
                        duration:       720
                        running:        busyIndicator.visible
                    }
                }
                QGCLabel {
                    text:           qsTr("List Of Available Devices")
                    visible:        QGroundControl.pairingManager.pairedDeviceNameList.length > 0 && !cancelButton.visible
                    font.pointSize: ScreenTools.mediumFontPointSize
                    font.family:    ScreenTools.demiboldFontFamily
                }
                Item { width: 1; height: 1; }
                GridLayout {
                    columns:        3
                    visible:        QGroundControl.pairingManager.pairedDeviceNameList.length > 0 && !cancelButton.visible
                    columnSpacing:  ScreenTools.defaultFontPixelWidth
                    rowSpacing:     ScreenTools.defaultFontPixelHeight * 0.25
                    anchors.horizontalCenter: parent.horizontalCenter
                    Repeater {
                        model:      QGroundControl.pairingManager.pairedDeviceNameList
                        QGCLabel {
                            text:   modelData
                            Layout.row:         index
                            Layout.column:      0
                            Layout.minimumWidth:ScreenTools.defaultFontPixelWidth * 14
                            Layout.fillWidth:   true
                        }
                    }
                    Repeater {
                        model:      QGroundControl.pairingManager.pairedDeviceNameList
                        QGCButton {
                            text:               qsTr("Connect")
                            Layout.row:         index
                            Layout.column:      1
                            onClicked: {
                                QGroundControl.pairingManager.connectToPairedDevice(modelData)
                            }
                        }
                    }
                    Repeater {
                        model:      QGroundControl.pairingManager.pairedDeviceNameList
                        QGCColoredImage {
                            height:             ScreenTools.defaultFontPixelHeight * 1.5
                            width:              height
                            sourceSize.height:   height
                            source:             "/res/TrashDelete.svg"
                            color:              qgcPal.colorRed
                            Layout.row:         index
                            Layout.column:      2
                            MouseArea {
                                anchors.fill:   parent
                                onClicked: {
                                    //-- TODO:
                                }
                            }
                        }
                    }
                }
                Item { width: 1; height: 1; }
                RowLayout {
                    id:                         connectedButtons
                    visible:                    QGroundControl.pairingManager.pairingStatus === PairingManager.PairingConnected || QGroundControl.pairingManager.pairingStatus === PairingManager.PairingIdle
                    spacing:                    ScreenTools.defaultFontPixelWidth * 4
                    anchors.horizontalCenter:   parent.horizontalCenter
                    QGCButton {
                        text:                   qsTr("Pair Another")
                        Layout.minimumWidth:    _contentWidth * 0.333
                        Layout.fillWidth:       true
                        onClicked: {
                            connectionPopup.close()
                            if(QGroundControl.pairingManager.pairingLinkTypeStrings.length > 1)
                                pairingPopup.open()
                            else {
                                mhPopup.open()
                            }
                        }
                    }
                    QGCButton {
                        text:                   QGroundControl.pairingManager.pairingStatus === PairingManager.PairingConnected ? qsTr("Go And Fly") : qsTr("Close")
                        Layout.minimumWidth:    _contentWidth * 0.333
                        Layout.fillWidth:       true
                        onClicked: {
                            connectionPopup.close()
                        }
                    }
                }
                QGCButton {
                    id:                         cancelButton
                    visible:                    QGroundControl.pairingManager.pairingStatus === PairingManager.PairingActive || QGroundControl.pairingManager.pairingStatus === PairingManager.PairingConnecting
                    text:                       qsTr("Cancel")
                    width:                      _contentWidth
                    anchors.horizontalCenter:   parent.horizontalCenter
                    onClicked: {
                        if(QGroundControl.pairingManager.pairingStatus === PairingManager.PairingActive)
                            QGroundControl.pairingManager.stopPairing()
                        else {
                            //-- TODO: Cancel connection to paired device
                        }
                        connectionPopup.close()
                    }
                }
                QGCButton {
                    visible:                    !cancelButton.visible && !connectedButtons.visible
                    text:                       qsTr("Close")
                    width:                      _contentWidth
                    anchors.horizontalCenter:   parent.horizontalCenter
                    onClicked: {
                        connectionPopup.close()
                    }
                }
                Item { width: 1; height: 1; }
            }
        }
    }
}
