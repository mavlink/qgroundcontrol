/****************************************************************************
 *
 * Copyright (c) 2016, Intel Corporation
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/**
 * @file
 *   @brief QGC Video Streaming Widget
 *   @author Ricardo de Almeida Gonzaga <ricardo.gonzaga@intel.com>
 */

import QtQuick                    2.5
import QtQuick.Controls           1.2
import QtQuick.Controls.Styles    1.2
import QtQuick.Dialogs            1.2

import QGroundControl.Palette     1.0
import QGroundControl.Controls    1.0
import QGroundControl.Controllers 1.0
import QGroundControl.ScreenTools 1.0

QGCView {
    viewPanel:    panel

    property real _margins:             ScreenTools.defaultFontPixelWidth
    property real _elementsHeight:      ScreenTools.defaultFontPixelWidth * 3
    property real _elementsWidth:       ScreenTools.defaultFontPixelWidth * 28
    property real _buttonWidth:         ScreenTools.defaultFontPixelWidth * 15
    property real _frameSizeFieldWidth: ScreenTools.defaultFontPixelWidth * 12

    QGCPalette {
        id:                qgcPal
        colorGroupEnabled: enabled
    }

    VideoStreamingWidgetController {
        id:                   controller
        factPanel:            panel
        serverLabel:          serverLabel
        ipField:              ipField
        portField:            portField
        streamsComboBox:      streamsComboBox
        formatComboBox:       formatComboBox
        frameSizeWidthField:  frameSizeWidthField
        frameSizeHeightField: frameSizeHeightField
        nameField:            nameField
    }

    QGCViewPanel {
        id:                panel
        anchors.fill:      parent
        enabled:           controller.rtspEnabled

        Rectangle {
            anchors.fill:                   parent
            color:                          qgcPal.window

            QGCLabel {
                id:                         title
                anchors.margins:            _margins
                anchors.left:               parent.left
                anchors.right:              parent.right
                anchors.top:                parent.top
                horizontalAlignment:        Text.AlignHCenter
                wrapMode:                   Text.WordWrap
                textFormat:                 Text.RichText
                text:                       "RTSP Stream Configuration"
            }

            Row {
                id:                         serverRow
                spacing:                    ScreenTools.defaultFontPixelWidth
                anchors.margins:            _margins
                anchors.left:               parent.left
                anchors.right:              parent.right
                anchors.top:                title.bottom

                QGCLabel {
                    id:                     serverLabel
                    width:                  _elementsWidth
                    height:                 _elementsHeight
                    wrapMode:               Text.WordWrap
                    textFormat:             Text.RichText
                    text:                   "Server:"
                }


                QGCButton {
                    id:                     refreshButton
                    text:                   "Refresh"
                    width:                  _buttonWidth
                    height:                 _elementsHeight
                    onClicked:              controller.refresh()
                }
            }

            Row {
                id:                         ipRow
                spacing:                    ScreenTools.defaultFontPixelWidth
                anchors.margins:            _margins
                anchors.left:               parent.left
                anchors.right:              parent.right
                anchors.top:                serverRow.bottom

                QGCTextField {
                    id:                     ipField
                    width:                  _elementsWidth
                    height:                 _elementsHeight
                    anchors.verticalCenter: parent.verticalCenter
                }

                QGCButton {
                    id:                     ipButton
                    text:                   "Set IP"
                    width:                  _buttonWidth
                    height:                 _elementsHeight
                    onClicked:              controller.setIp()
                }
            }

            Row {
                id:                         portRow
                spacing:                    ScreenTools.defaultFontPixelWidth
                anchors.margins:            _margins
                anchors.left:               parent.left
                anchors.right:              parent.right
                anchors.top:                ipRow.bottom

                QGCTextField {
                    id:                     portField
                    width:                  _elementsWidth
                    height:                 _elementsHeight
                    anchors.verticalCenter: parent.verticalCenter
                }

                QGCButton {
                    id:                     portButton
                    text:                   "Set Port"
                    width:                  _buttonWidth
                    height:                 _elementsHeight
                    onClicked:              controller.setPort()
                }
            }

            Row {
                id:                         streamsRow
                spacing:                    ScreenTools.defaultFontPixelWidth
                anchors.margins:            _margins
                anchors.left:               parent.left
                anchors.right:              parent.right
                anchors.top:                portRow.bottom

                QGCLabel {
                    id:                     streamsLabel
                    width:                  ScreenTools.defaultFontPixelWidth * 7
                    height:                 _elementsHeight
                    wrapMode:               Text.WordWrap
                    textFormat:             Text.RichText
                    text:                   "Stream: "
                }

                QGCComboBox {
                    id:                     streamsComboBox
                    width:                  ScreenTools.defaultFontPixelWidth * 36
                    height:                 _elementsHeight
                    onActivated:            controller.setActiveStream()
                }
            }

            Row {
                id:                         formatRow
                spacing:                    ScreenTools.defaultFontPixelWidth
                anchors.margins:            _margins
                anchors.left:               parent.left
                anchors.right:              parent.right
                anchors.top:                streamsRow.bottom

                QGCComboBox {
                    id:                     formatComboBox
                    width:                  _elementsWidth
                    height:                 _elementsHeight
                }

                QGCButton {
                    id:                     formatButton
                    text:                   "Set Format"
                    width:                  _buttonWidth
                    height:                 _elementsHeight
                    onClicked:              controller.setFormat()
                }
            }

            Row {
                id:                         frameSizeRow
                spacing:                    ScreenTools.defaultFontPixelWidth
                anchors.margins:            _margins
                anchors.left:               parent.left
                anchors.right:              parent.right
                anchors.top:                formatRow.bottom

                QGCTextField {
                    id:                     frameSizeWidthField
                    width:                  _frameSizeFieldWidth
                    height:                 _elementsHeight
                    anchors.verticalCenter: parent.verticalCenter
                }

                QGCLabel {
                    id:                     frameSizeLabel
                    width:                  ScreenTools.defaultFontPixelWidth * 2
                    height:                 _elementsHeight
                    horizontalAlignment:    Text.AlignHCenter
                    wrapMode:               Text.WordWrap
                    textFormat:             Text.RichText
                    text:                   "x"
                }

                QGCTextField {
                    id:                     frameSizeHeightField
                    width:                  _frameSizeFieldWidth
                    height:                 _elementsHeight
                    anchors.verticalCenter: parent.verticalCenter
                }

                QGCButton {
                    id:                     frameSizeButton
                    text:                   "Set Frame Size"
                    width:                  _buttonWidth
                    height:                 _elementsHeight
                    onClicked:              controller.setFrameSize()
                }
            }

            Row {
                id:                         nameRow
                spacing:                    ScreenTools.defaultFontPixelWidth
                anchors.margins:            _margins
                anchors.left:               parent.left
                anchors.right:              parent.right
                anchors.top:                frameSizeRow.bottom

                QGCTextField {
                    id:                     nameField
                    width:                  _elementsWidth
                    height:                 _elementsHeight
                    anchors.verticalCenter: parent.verticalCenter
                }

                QGCButton {
                    id:                     nameButton
                    text:                   "Set Name"
                    width:                  _buttonWidth
                    height:                 _elementsHeight
                    onClicked:              controller.setName()
                }
            }
        }
    }
}
