/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.4

import QGroundControl.Palette 1.0
import QGroundControl.ScreenTools 1.0

Rectangle {
    property string flightModeName          ///< User visible name for this  flight mode
    property string flightModeDescription
    property real   rcValue                 ///< Current rcValue to show in Monitor display, range: 0.0 - 1.0
    property int    modeChannelIndex        ///< Index into channel list for this mode
    property bool   modeChannelEnabled      ///< true: Channel combo box is enabled
    property bool   modeSelected            ///< true: This mode is currently selected
    property real   thresholdValue          ///< Treshold setting for this mode, show in Threshold display, range 0.0 - 1.0
    property bool   thresholdDragEnabled    ///< true: Threshold value indicator can be dragged to modify value

    anchors.leftMargin:     ScreenTools.defaultFontPixelWidth
    anchors.rightMargin:    ScreenTools.defaultFontPixelWidth
    anchors.left:           parent.left
    anchors.right:          parent.right
    height:                 column.height + (ScreenTools.defaultFontPixelWidth * 2)
    color:                  _qgcPal.window

    signal modeChannelIndexSelected(int index)

    QGCPalette { id: _qgcPal; colorGroupEnabled: enabled }

    Item {
        anchors.margins:    ScreenTools.defaultFontPixelWidth
        anchors.fill:       parent

        Column {
            id:         column
            width:      parent.width
            spacing:    ScreenTools.defaultFontPixelHeight / 4

            Row {
                width:      parent.width
                spacing:    ScreenTools.defaultFontPixelWidth

                Rectangle {
                    width:  modeLabel.width
                    height: channelCombo.height
                    color:  modeSelected ? _qgcPal.buttonHighlight : _qgcPal.button

                    QGCLabel {
                        id:                     modeLabel
                        width:                  ScreenTools.defaultFontPixelWidth * 18
                        anchors.top:            parent.top
                        anchors.bottom:         parent.bottom
                        color:                  modeSelected ? _qgcPal.buttonHighlightText : _qgcPal.text
                        horizontalAlignment:    Text.AlignHCenter
                        verticalAlignment:      Text.AlignVCenter
                        text:                   flightModeName
                    }
                }

                QGCComboBox {
                    id:             channelCombo
                    width:          ScreenTools.defaultFontPixelWidth * 15
                    model:          controller.channelListModel
                    currentIndex:   modeChannelIndex
                    enabled:        modeChannelEnabled

                    onActivated: modeChannelIndexSelected(index)
                }

                QGCLabel {
                    width:      parent.width - x
                    wrapMode:   Text.WordWrap
                    text:       flightModeDescription
                    visible:    !ScreenTools.isTinyScreen
                }
            }

            Row {
                width:      parent.width
                spacing:    ScreenTools.defaultFontPixelWidth * 2

                QGCLabel {
                    id:                 monitorLabel
                    height:             ScreenTools.defaultFontPixelHeight
                    verticalAlignment:  Text.AlignVCenter
                    text:               qsTr("Monitor:")
                }

                Item {
                    height: ScreenTools.defaultFontPixelHeight
                    width:  parent.width - x

                    // Won't be able to pull these properties, need to reference parent.
                    property int            __lastRcValue:      1500
                    readonly property int   __rcValueMaxJitter: 2

                    // Bar
                    Rectangle {
                        id:                     bar
                        anchors.verticalCenter: parent.verticalCenter
                        width:                  parent.width
                        height:                 parent.height / 2
                        color:                  _qgcPal.windowShadeDark
                    }

                    // RC Indicator
                    Rectangle {
                        anchors.verticalCenter: parent.verticalCenter
                        width:                  parent.height * 0.75
                        height:                 width
                        x:                      (parent.width * rcValue) - (width / 2)
                        radius:                 width / 2
                        color:                  _qgcPal.text
                    }
                } // Item
            }

            Row {
                width:      parent.width
                spacing:    ScreenTools.defaultFontPixelWidth * 2

                QGCLabel {
                    id:                 thresholdLabel
                    height:             ScreenTools.defaultFontPixelHeight
                    verticalAlignment:  Text.AlignVCenter
                    text:               qsTr("Threshold:")
                }

                Item {
                    id:     thresholdContainer
                    height: ScreenTools.defaultFontPixelHeight
                    width:  parent.width - x

                    // Bar
                    Rectangle {
                        anchors.verticalCenter: parent.verticalCenter
                        width:                  parent.width
                        height:                 parent.height / 2
                        color:                  _qgcPal.windowShadeDark
                    }

                    // Threshold Indicator
                    Rectangle {
                        id:                     thresholdIndicator
                        anchors.verticalCenter: parent.verticalCenter
                        width:                  parent.height * 0.75
                        height:                 width
                        x:                      (parent.width * thresholdValue) - (width / 2)
                        radius:                 width / 2
                        color:                  thresholdDragEnabled ? _qgcPal.buttonHighlight : _qgcPal.text

                        Drag.active:    thresholdDrag.drag.active
                        Drag.hotSpot.x: width / 2
                        Drag.hotSpot.y: height / 2

                        MouseArea {
                            id:             thresholdDrag
                            anchors.fill:   parent
                            cursorShape:    Qt.SizeHorCursor
                            drag.target:    thresholdDragEnabled ? parent : null
                            drag.minimumX:  - (width / 2)
                            drag.maximumX:  thresholdContainer.width - (width / 2)
                            drag.axis:      Drag.XAxis

                            property bool dragActive: drag.active

                            onDragActiveChanged: {
                                if (!drag.active) {
                                    thresholdValue = (thresholdIndicator.x + (thresholdIndicator.width / 2)) / thresholdContainer.width
                                }
                            }
                        }
                    }
                } // Item
            } // Row
        } // Column
    } // Item
} // Rectangle
