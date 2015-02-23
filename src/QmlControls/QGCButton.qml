import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import QtQuick.Controls.Private 1.0

import QGroundControl.Palette 1.0

Button {
    // primary: true - this is the primary button for this group of buttons
    property bool primary: false

    property var __qgcPal: QGCPalette { colorGroupEnabled: enabled }

    property bool __showHighlight: pressed | hovered | checked

    style: ButtonStyle {
/*
            background: Rectangle {
                implicitWidth: 100
                implicitHeight: 25
                color: __showHighlight ?
                    control.__qgcPal.buttonHighlight :
                    (primary ? control.__qgcPal.primaryButton : control.__qgcPal.button)
            }

            label: Text {
                width: parent.width
                height: parent.height

                verticalAlignment: TextEdit.AlignVCenter
                horizontalAlignment: TextEdit.AlignHCenter

                text: control.text
                color: __showHighlight ?
                    control.__qgcPal.buttonHighlightText :
                    (primary ? control.__qgcPal.primaryButtonText : control.__qgcPal.buttonText)
            }
*/

            /*! The padding between the background and the label components. */
            padding {
                top: 4
                left: 4
                right:  control.menu !== null ? Math.round(TextSingleton.implicitHeight * 0.5) : 4
                bottom: 4
            }

            /*! This defines the background of the button. */
            background: Item {
                property bool down: control.pressed || (control.checkable && control.checked)
                implicitWidth: Math.round(TextSingleton.implicitHeight * 4.5)
                implicitHeight: Math.max(25, Math.round(TextSingleton.implicitHeight * 1.2))

                Rectangle {
                    anchors.fill: parent
                    color: __showHighlight ?
                        control.__qgcPal.buttonHighlight :
                        (primary ? control.__qgcPal.primaryButton : control.__qgcPal.button)
                }

                Image {
                    id: imageItem
                    visible: control.menu !== null
                    source: "arrow-down.png"
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right
                    anchors.rightMargin: padding.right
                    opacity: control.enabled ? 0.6 : 0.5
                }
            }

            /*! This defines the label of the button.  */
            label: Item {
                implicitWidth: row.implicitWidth
                implicitHeight: row.implicitHeight
                baselineOffset: row.y + text.y + text.baselineOffset

                Row {
                    id: row
                    anchors.centerIn: parent
                    spacing: 2

                    Image {
                        source: control.iconSource
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Text {
                        id: text
                        renderType: Text.NativeRendering
                        anchors.verticalCenter: parent.verticalCenter
                        text: control.text
                        color: __showHighlight ?
                            control.__qgcPal.buttonHighlightText :
                            (primary ? control.__qgcPal.primaryButtonText : control.__qgcPal.buttonText)
                    }
                }
            }
        }
}
