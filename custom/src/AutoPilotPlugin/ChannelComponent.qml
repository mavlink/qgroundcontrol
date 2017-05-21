/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.3
import QtQuick.Controls 1.2
import QtQuick.Dialogs  1.2
import QtQuick.Layouts  1.2

import QGroundControl               1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0

import TyphoonHQuickInterface           1.0
import TyphoonHQuickInterface.Widgets   1.0

SetupPage {
    id:             channelPage
    pageComponent:  pageComponent

    Component {
        id: pageComponent

        Item {
            width:  Math.max(availableWidth, innerColumn.width)
            height: innerColumn.height
            anchors.verticalCenter: parent.verticalCenter

            FactPanelController { id: controller; factPanel: channelPage.viewPanel }

            ColumnLayout {
                id:                         innerColumn
                spacing:                    ScreenTools.defaultFontPixelHeight * 0.5
                anchors.horizontalCenter:   parent.horizontalCenter
                QGCGroupBox {
                    title:  qsTr("Sticks")
                    Column {
                        spacing:        ScreenTools.defaultFontPixelHeight
                        width:          parent.width
                        ST16Analog {
                            text:   "J1"
                            value:  TyphoonHQuickInterface.J1
                        }
                        ST16Analog {
                            text:   "J2"
                            value:  TyphoonHQuickInterface.J2
                        }
                        ST16Analog {
                            text:   "J3"
                            value:  TyphoonHQuickInterface.J3
                        }
                        ST16Analog {
                            text:   "J4"
                            value:  TyphoonHQuickInterface.J4
                        }
                    }
                }
                QGCGroupBox {
                    title:  qsTr("Back Knobs")
                    Column {
                        spacing:        ScreenTools.defaultFontPixelHeight
                        width:          parent.width
                        ST16Analog {
                            text:   "K2"
                            value:  TyphoonHQuickInterface.K2
                        }
                        ST16Analog {
                            text:   "K3"
                            value:  TyphoonHQuickInterface.K3
                        }
                    }
                }
                QGCGroupBox {
                    title:  qsTr("Others")
                    Column {
                        spacing:        ScreenTools.defaultFontPixelHeight
                        width:          parent.width
                        ST16Analog {
                            text:   "K1"
                            value:  TyphoonHQuickInterface.K1
                        }
                        ST16Analog {
                            text:   "L Trim"
                            value:  TyphoonHQuickInterface.T12
                        }
                        ST16Analog {
                            text:   "R Trim"
                            value:  TyphoonHQuickInterface.T34
                        }
                        ST16Analog {
                            text:   "Switches"
                            value:  TyphoonHQuickInterface.ASwitch
                        }
                    }
                }
                Item {
                    width:  1
                    height: ScreenTools.defaultFontPixelHeight
                }
                Row {
                    spacing: ScreenTools.defaultFontPixelWidth * 2
                    anchors.left: parent.left
                    QGCButton {
                        text:       "Start Calibration"
                      //enabled:    TyphoonHQuickInterface.calibrationComplete
                        width:      ScreenTools.defaultFontPixelWidth * 18
                        onClicked: {
                            TyphoonHQuickInterface.startCalibration()
                        }
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    QGCLabel {
                        text:   "Not yet functional"
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
            }
        }
    }
}
