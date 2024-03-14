import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl.Palette
import QGroundControl.ScreenTools

QGCButton {
    id:             control     //This is a button rework from DonLakeFlyer's QGCButton that allows to contain a text and an Icon with a column look and the same capabilites

    background: Rectangle {
        id:             backRect
        width:          control.width
        height:         control.height
        radius:         backRadius
        border.width:   showBorder ? 1 : 0
        border.color:   qgcPal.buttonText
        color:          _showHighlight ?
                            qgcPal.buttonHighlight :
                            (primary ? qgcPal.primaryButton : qgcPal.button)
    }

    contentItem: Item {
        id: contItem
        implicitWidth:  Math.max(textLabel.implicitWidth , icon.implicitWidth ) * 1.1
        implicitHeight: iconSource === "" ? textLabel.implicitHeight : textLabel.implicitHeight * 2.5 
        ColumnLayout {
            anchors.fill: parent
            QGCColoredImage {
                id: icon
                Layout.fillWidth:       true
                Layout.fillHeight:      true
                Layout.maximumHeight:   parent.height - textLabelContainer.height
                Layout.alignment:       control.text !== "" ? Qt.AlignTop : Qt.AlignHCenter | Qt.AlignVCenter
                source:                 control.iconSource
                color:                  qgcPal.text
                fillMode:               Image.PreserveAspectFit
                sourceSize.height:      height
                sourceSize.width:       width
                visible:                control.iconSource !== "" ? true : false
            }   

            Item {
                id:                     textLabelContainer
                Layout.alignment:       icon.visible ? Qt.AlignBottom | Qt.AlignHCenter : Qt.AlignCenter
                visible:                control.text !== "" ? true : false
                Layout.preferredWidth:  parent.width
                Layout.preferredHeight: textLabel.baselineOffset
                QGCLabel {
                    id: textLabel
                    text:                       control.text
                    font.family:                control.font.family
                    font.pointSize:             control.font.pointSize
                    color:                      _showHighlight ? qgcPal.buttonHighlightText : (primary ? qgcPal.primaryButtonText : qgcPal.buttonText)
                    anchors.baseline:           iconSource !== "" ? parent.bottom : undefined
                    anchors.centerIn:           iconSource === "" ? parent : undefined   
                    anchors.horizontalCenter:   parent.horizontalCenter
                }
            }
        }
    }
}
