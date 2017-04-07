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

SetupPage {
    id:             switchPage
    pageComponent:  pageComponent
    Component {
        id: pageComponent
        Item {
            width:  Math.max(availableWidth, innerColumn.width)
            height: innerColumn.height

            FactPanelController { id: controller; factPanel: switchPage.viewPanel }

            property Fact _customMode:  controller.getParameterFact(-1, "COM_FLTMODE6")

            ExclusiveGroup { id: homeLoiterGroup }

            Component.onCompleted: {
                console.log(_customMode.value + ' ' + _customMode.valueString)
            }

            function getExplanationText() {
                if(_customMode.value === 1) {
                    return qsTr("Lorem ipsum dolor sit amet, consectetur adipiscing elit. Morbi id gravida urna. Praesent interdum lacinia eros, ut consequat augue varius eget. Curabitur semper nulla vehicula felis ultrices porta. Nunc felis nulla, ullamcorper non lacinia semper, blandit sed lacus. Suspendisse mattis auctor risus a tincidunt. Nam tincidunt pretium lorem eget efficitur. Suspendisse vel scelerisque nunc, in faucibus tellus. Phasellus vel dolor orci. Donec et ante a odio porta finibus. Sed augue leo, elementum ut porttitor sit amet, faucibus in ex. Ut sodales condimentum dui, eu volutpat libero aliquet cursus.")
                } else if(_customMode.value === 13) {
                    return qsTr("Vivamus nec magna pretium, dictum enim nec, convallis purus. Maecenas ornare felis erat, eu dapibus velit lobortis id. Sed vel est odio. Mauris nec dui tristique, mattis justo sed, convallis erat. Morbi ut urna a tortor faucibus sodales. Quisque lectus nisl, maximus in eros nec, pretium ultricies arcu. Proin tellus nunc, semper non vestibulum non, euismod at magna. Vestibulum massa tortor, dapibus malesuada elit sed, molestie bibendum magna. Nam tellus dui, iaculis eget bibendum nec, tincidunt in purus. Proin ac tincidunt purus. Nam eu lectus vel ipsum cursus bibendum.")
                }
                return qsTr("Invalid Mode")
            }

            ColumnLayout {
                id:                         innerColumn
                anchors.horizontalCenter:   parent.horizontalCenter
                spacing:                    ScreenTools.defaultFontPixelHeight
                QGCGroupBox {
                    id:     batteryGroup
                    title:  qsTr("Flight Mode Options")
                    Row {
                        id:             modeGrid
                        spacing:        ScreenTools.defaultFontPixelWidth * 4
                        Column {
                            spacing:        ScreenTools.defaultFontPixelHeight
                            QGCRadioButton {
                                id:                 smartModeButton
                                checked:            _customMode.value === 13
                                exclusiveGroup:     homeLoiterGroup
                                text:               qsTr("Smart Mode")
                                onClicked:          _customMode.value = 13
                            }
                            QGCRadioButton {
                                id:                 manualModeButton
                                checked:            _customMode.value === 1
                                exclusiveGroup:     homeLoiterGroup
                                text:               qsTr("Manual Mode")
                                onClicked:          _customMode.value = 1
                            }
                            Item {
                                width:  1
                                height: ScreenTools.defaultFontPixelHeight
                            }
                            QGCLabel {
                                text:                   getExplanationText()
                                width:                  ScreenTools.defaultFontPixelWidth * 50
                                wrapMode:               Text.WordWrap
                            }
                        }
                        QGCColoredImage {
                            color:                  qgcPal.text
                            height:                 ScreenTools.defaultFontPixelWidth * 20
                            width:                  ScreenTools.defaultFontPixelWidth * 20
                            sourceSize.height:      height
                            mipmap:                 true
                            fillMode:               Image.PreserveAspectFit
                            source:                 "/typhoonh/img/smartMode.svg"
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                }
            }
        }
    }
}
