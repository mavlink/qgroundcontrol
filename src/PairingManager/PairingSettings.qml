/****************************************************************************
 *
 *   (c) 2019 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtGraphicalEffects       1.0
import QtMultimedia             5.5
import QtQuick                  2.3
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.4
import QtQuick.Dialogs          1.2
import QtQuick.Layouts          1.2
import QtLocation               5.3
import QtPositioning            5.3

import QGroundControl                       1.0
import QGroundControl.Controllers           1.0
import QGroundControl.Controls              1.0
import QGroundControl.FactControls          1.0
import QGroundControl.FactSystem            1.0
import QGroundControl.Palette               1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.SettingsManager       1.0

Rectangle {
    id:                 _root
    color:              qgcPal.window
    anchors.fill:       parent
    anchors.margins:    ScreenTools.defaultFontPixelWidth

    property real _labelWidth:                  ScreenTools.defaultFontPixelWidth * 26
    property real _valueWidth:                  ScreenTools.defaultFontPixelWidth * 20
    property real _panelWidth:                  _root.width * _internalWidthRatio

    readonly property real _internalWidthRatio:          0.8

    ExclusiveGroup { id: pairingLinkGroup }

    QGCFlickable {
        clip:               true
        anchors.fill:       parent
        contentHeight:      settingsColumn.height
        contentWidth:       settingsColumn.width
        Column {
            id:                 settingsColumn
            width:              _root.width
            spacing:            ScreenTools.defaultFontPixelHeight * 0.5
            anchors.margins:    ScreenTools.defaultFontPixelWidth
            Item {
                width:                      _panelWidth
                height:                     generalLabel.height
                anchors.margins:            ScreenTools.defaultFontPixelWidth
                anchors.horizontalCenter:   parent.horizontalCenter
                QGCLabel {
                    id:             pairingMethodsLabel
                    text:           qsTr("Pairing methods:")
                    font.family:    ScreenTools.demiboldFontFamily
                }
            }
            Repeater {
                id: repeater
                model: QGroundControl.pairingManager.pairingLinkTypeStrings
                anchors.horizontalCenter:   parent.horizontalCenter
                delegate: QGCButton {
                    width:                      _root.width * 0.2
                    text:                       modelData
                    exclusiveGroup:             pairingLinkGroup
                    anchors.horizontalCenter:   settingsColumn.horizontalCenter
                    onClicked: {
                        checked = true
                        if (index === QGroundControl.pairingManager.nfcIndex) {
                            QGroundControl.pairingManager.startNFCScan();
                        } else if (index === QGroundControl.pairingManager.microhardIndex) {
                            QGroundControl.pairingManager.startMicrohardPairing();
                        }
                    }
                }
            }
            Item {
                width:                      _panelWidth
                height:                     generalLabel.height
                anchors.margins:            ScreenTools.defaultFontPixelWidth
                anchors.horizontalCenter:   settingsColumn.horizontalCenter
                QGCLabel {
                    id:             generalLabel
                    text:           QGroundControl.pairingManager.pairingStatusStr
                    font.family:    ScreenTools.demiboldFontFamily
                }
            }
            Item {
                width:                      _panelWidth
                height:                     generalLabel.height
                anchors.margins:            ScreenTools.defaultFontPixelWidth
                anchors.horizontalCenter:   settingsColumn.horizontalCenter
                QGCLabel {
                    id:             pairingsLabel
                    text:           qsTr("Paired UAVs:")
                    font.family:    ScreenTools.demiboldFontFamily
                }
            }
            Repeater {
                model: QGroundControl.pairingManager.pairedDeviceNameList
                delegate: QGCButton {
                    text:   modelData
                    width:  _root.width * 0.3
                    anchors.horizontalCenter:   parent.horizontalCenter
                    exclusiveGroup: pairingLinkGroup
                    onClicked: {
                        checked = true
                        QGroundControl.pairingManager.connectToPairedDevice(text)
                    }
                }
            }
        }
    }
}
