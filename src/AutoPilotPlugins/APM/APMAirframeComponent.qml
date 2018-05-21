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

import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.Controllers   1.0
import QGroundControl.ScreenTools   1.0

SetupPage {
    id:             airframePage
    pageComponent:  _useOldFrameParam ?  oldFramePageComponent: newFramePageComponent

    property real _margins:             ScreenTools.defaultFontPixelWidth
    property bool _useOldFrameParam:    controller.parameterExists(-1, "FRAME")
    property Fact _oldFrameParam:       controller.getParameterFact(-1, "FRAME", false)
    property Fact _newFrameParam:       controller.getParameterFact(-1, "FRAME_CLASS", false)
    property Fact _frameTypeParam:      controller.getParameterFact(-1, "FRAME_TYPE", false)


    APMAirframeComponentController {
        id:         controller
        factPanel:  airframePage.viewPanel
    }

    ExclusiveGroup {
        id: airframeTypeExclusive
    }

    Component {
        id: oldFramePageComponent

        Column {
            width:      availableWidth
            height:     1000
            spacing:    _margins

            RowLayout {
                anchors.left:   parent.left
                anchors.right:  parent.right
                spacing:        _margins

                QGCLabel {
                    font.pointSize:     ScreenTools.mediumFontPointSize
                    wrapMode:           Text.WordWrap
                    text:               qsTr("Please select your airframe type")
                    Layout.fillWidth:   true
                }
            }

            Repeater {
                model: controller.airframeTypesModel

                QGCRadioButton {
                    text: object.name
                    checked: controller.currentAirframeType == object
                    exclusiveGroup: airframeTypeExclusive

                    onCheckedChanged: {
                        if (checked) {
                            controller.currentAirframeType = object
                        }
                    }
                }
            }
        } // Column
    } // Component - oldFramePageComponent

    Component {
        id: newFramePageComponent

        Grid {
            width:      availableWidth
            spacing:    _margins
            columns:    2

            QGCLabel {
                text: qsTr("Frame Class:")
            }

            FactComboBox {
                fact:       _newFrameParam
                indexModel: false
                width:      ScreenTools.defaultFontPixelWidth * 15
            }

            QGCLabel {
                text: qsTr("Frame Type:")
            }

            FactComboBox {
                fact:       _frameTypeParam
                indexModel: false
                width:      ScreenTools.defaultFontPixelWidth * 15
            }
        }
    }
} // SetupPage
