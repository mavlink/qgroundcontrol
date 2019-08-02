/****************************************************************************
 *
 * (c) 2009-2019 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 * @file
 *   @author Gus Grubba <gus@auterion.com>
 */

import QtQuick                      2.11
import QtQuick.Controls             2.4
import QtQuick.Layouts              1.11

import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0

ComboBox {
    id:         control
    padding:    ScreenTools.comboBoxPadding

    property bool centeredLabel:  false

    background: Rectangle {
        implicitWidth:                  ScreenTools.implicitComboBoxWidth
        implicitHeight:                 ScreenTools.implicitComboBoxHeight
        color:                          qgcPal.window
        border.width:                   enabled ? 1 : 0
        border.color:                   "#999"
    }

    /*! Adding the Combobox list item to the theme.  */

    delegate: ItemDelegate {
            width:                      control.width

            contentItem: Text {
                text:                   textRole ? modelData[textRole] : modelData
                color:                  control.currentIndex === index ? qgcPal.buttonHighlightText : qgcPal.buttonText
                verticalAlignment:      Text.AlignVCenter
            }

            background: Rectangle {
                color:                  control.currentIndex === index ? qgcPal.buttonHighlight : qgcPal.button
            }

            highlighted:                control.highlightedIndex === index
        }

    /*! This defines the label of the button.  */
    contentItem: Item {
        implicitWidth:                  text.implicitWidth
        implicitHeight:                 text.implicitHeight

        QGCLabel {
            id:                         text
            anchors.verticalCenter:     parent.verticalCenter
            anchors.horizontalCenter:   centeredLabel ? parent.horizontalCenter : undefined
            text:                       control.currentText
            color:                      qgcPal.text
        }
    }
}
