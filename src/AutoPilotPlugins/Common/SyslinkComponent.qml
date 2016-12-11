/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick          2.5
import QtQuick.Controls 1.2
import QtQuick.Dialogs  1.2
import QtQuick.Layouts  1.2

import QGroundControl               1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controllers   1.0

QGCView {
    id:         qgcView
    viewPanel:  panel

    QGCPalette { id: palette; colorGroupEnabled: panel.enabled }

    property real _margins:         ScreenTools.defaultFontPixelHeight
    property real _editFieldWidth:  ScreenTools.defaultFontPixelWidth * 16
    property real _labelWidth:      ScreenTools.defaultFontPixelWidth * 18

    SyslinkComponentController {
        id:             controller
        factPanel:      panel
    }

    QGCViewPanel {
        id:             panel
        anchors.fill:   parent

        Flickable {
            clip:                                       true
            anchors.fill:                               parent
            contentHeight:                              mainCol.height
            flickableDirection:                         Flickable.VerticalFlick
            Column {
                id:                                     mainCol
                spacing:                                _margins
                anchors.horizontalCenter:               parent.horizontalCenter
                Item { width: 1; height: _margins * 0.5; }

                Rectangle {
                    color:                              palette.windowShade
                    height:                             settingsRow.height  + _margins * 2
                    Row {
                        id:                             settingsRow
                        spacing:                        _margins * 4
                        anchors.centerIn:               parent
                        Column {
                            spacing:                        _margins * 0.5
                            anchors.verticalCenter:         parent.verticalCenter
                            Row {
                                QGCLabel {
                                    text:                               qsTr("NRF Radio Settings")
                                    font.family:                        ScreenTools.demiboldFontFamily
                                }
                            }
                            Row {
                                QGCLabel {
                                    text:                   qsTr("Channel")
                                    width:                  _labelWidth
                                    anchors.baseline:       channelField.baseline
                                }
                                QGCTextField {
                                    id:                     channelField
                                    width:                  _editFieldWidth
                                    text:                   controller.radioChannel
                                    validator:              IntValidator {bottom: 0; top: 125;}
                                    inputMethodHints:       Qt.ImhDigitsOnly
                                    onEditingFinished: {
                                        controller.radioChannel = text
                                    }
                                }
                            }
                            Row {
                                anchors.right:   parent.right
                                QGCLabel {
                                    wrapMode:       Text.WordWrap
                                    text:           qsTr("Channel can be between 0 and 125")
                                }

                            }
                            Row {
                                QGCLabel {
                                    text:                   qsTr("Address")
                                    width:                  _labelWidth
                                    anchors.baseline:       addressField.baseline
                                }
                                QGCTextField {
                                    id:                     addressField
                                    width:                  _editFieldWidth
                                    text:                   controller.radioAddress
                                    maximumLength:          10
                                    validator:              RegExpValidator { regExp: /^[0-9A-Fa-f]*$/ }
                                    onEditingFinished: {
                                        controller.radioAddress = text
                                    }
                                }
                            }
                            Row {
                                anchors.right:  parent.right
                                QGCLabel {
                                    wrapMode:       Text.WordWrap
                                    text:           qsTr("Address in hex. Default E7E7E7E7E7")
                                }

                            }
                            Row {
                                QGCLabel {
                                    text:                   qsTr("Data Rate")
                                    width:                  _labelWidth
                                    anchors.baseline:       rateField.baseline
                                }
                                QGCComboBox {
                                    id:                     rateField
                                    width:                  _editFieldWidth
                                    model:                  controller.radioRates
                                    currentIndex:           controller.radioRate
                                    onActivated: {
                                        controller.radioRate = index
                                    }
                                }
                            }
                            Row {
                                spacing:                            _margins
                                anchors.horizontalCenter:           parent.horizontalCenter
                                QGCButton {
                                    text:                           qsTr("Restore Defaults")
                                    width:                          _editFieldWidth
                                    onClicked: {
                                        controller.resetDefaults()
                                    }
                                }
                            }
                        }
                    }
                }

            }
        }
    }
}
