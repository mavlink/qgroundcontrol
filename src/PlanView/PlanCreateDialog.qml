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
import QGroundControl.Controllers
import QGroundControl.FactSystem
import QGroundControl.FactControls
import QGroundControl.ScreenTools

QGCPopupDialog {
    id:         root
    title:      qsTr("Select Plan")
    buttons:    Dialog.Cancel

    property var planMasterController

    property real _margin:              ScreenTools.defaultFontPixelWidth
    property real _sectionLeftMargin:   ScreenTools.defaultFontPixelWidth * 2
    property var  _appSettings:         QGroundControl.settingsManager.appSettings

    ColumnLayout {
        spacing: ScreenTools.defaultFontPixelHeight / 2

        GridLayout {
            columns:            2
            columnSpacing:      _margin
            rowSpacing:         _margin
            Layout.fillWidth:   true

            Repeater {
                model: planMasterController.planCreators

                Rectangle {
                    id:     button
                    width:  ScreenTools.defaultFontPixelHeight * 7
                    height: planCreatorNameLabel.y + planCreatorNameLabel.height
                    color:  button.pressed || button.highlighted ? qgcPal.buttonHighlight : qgcPal.button

                    property bool highlighted: mouseArea.containsMouse
                    property bool pressed:     mouseArea.pressed

                    Image {
                        id:                 planCreatorImage
                        anchors.left:       parent.left
                        anchors.right:      parent.right
                        source:             object.imageResource
                        sourceSize.width:   width
                        fillMode:           Image.PreserveAspectFit
                        mipmap:             true
                    }

                    QGCLabel {
                        id:                     planCreatorNameLabel
                        anchors.top:            planCreatorImage.bottom
                        anchors.left:           parent.left
                        anchors.right:          parent.right
                        horizontalAlignment:    Text.AlignHCenter
                        text:                   object.name
                        color:                  button.pressed || button.highlighted ? qgcPal.buttonHighlightText : qgcPal.buttonText
                    }

                    QGCMouseArea {
                        id:                 mouseArea
                        anchors.fill:       parent
                        hoverEnabled:       true
                        preventStealing:    true

                        onClicked:          {
                            object.createPlan(coordList())
                            root.close()
                        }

                        function coordList() {
                            var topLeftPoint        = Qt.point(editorMap.centerViewport.x, editorMap.centerViewport.y)
                            var bottomRightPoint    = Qt.point(editorMap.centerViewport.x + editorMap.centerViewport.width, editorMap.centerViewport.y + editorMap.centerViewport.height)
                            var topLeftCoord        = editorMap.toCoordinate(topLeftPoint, false /* clipToViewPort */)
                            var bottomRightCoord    = editorMap.toCoordinate(bottomRightPoint, false /* clipToViewPort */)
                            return [ topLeftCoord, bottomRightCoord ]
                        }
                    }
                }
            }
        }
    }
}
