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
    id: control
    padding: ScreenTools.comboBoxPadding

    property bool centeredLabel: false
    property var _qgcPal: QGCPalette { colorGroupEnabled: enabled }

    Component.onCompleted: indicator.color = Qt.binding(function() { return _qgcPal.text })

    function hideIndicator() { control.indicator.visible = false }
    function hideBorder() { backgroundRect.width = 0 }
    function showSeparator() { separator.visible = true }

    function showTitle(text)
    {
        title.visible = true
        title.text = text
    }

    function setItemFont(color, size)
    {
        text.color = color
        text.font.pixelSize = 14
    }

    Text {
        id: title
        y: -(control.height / 2)

        color: _qgcPal.text
        font.pixelSize: 15
        visible: false
        leftPadding: control.padding
        horizontalAlignment: Text.AlignLeft
    }

    background: Rectangle {
        id: backgroundRect
        implicitWidth: ScreenTools.implicitComboBoxWidth
        implicitHeight:ScreenTools.implicitComboBoxHeight
        color: _qgcPal.window
        border.width: enabled ? 1 : 0
        border.color: "#999"
    }

    Rectangle
    {
        id: separator
        y: control.height + ScreenTools.defaultFontPixelWidth

        anchors.left: parent.left
        anchors.right: parent.right
        height: 1
        color: qgcPal.windowShade
        visible: false
    }

    /*! Adding the Combobox list item to the theme.  */

    delegate: ItemDelegate {
        width: control.width

        contentItem: Text {
            id: listViewForeground
            text: control.textRole ? (Array.isArray(control.model) ? modelData[control.textRole] : model[control.textRole]) : modelData
            color: control.currentIndex === index ? _qgcPal.buttonText : _qgcPal.button
            verticalAlignment: Text.AlignVCenter
        }

        background: Rectangle {
            id: listViewBackground
            color: control.currentIndex === index ? _qgcPal.brandingDarkBlue : _qgcPal.window
        }

        highlighted: control.highlightedIndex === index
    }

    /*! This defines the label of the button.  */
    contentItem: Item {
        implicitWidth: text.implicitWidth
        implicitHeight: text.implicitHeight

        QGCLabel {
            id: text
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: centeredLabel ? parent.horizontalCenter : undefined
            text: control.currentText
            color: _qgcPal.text
        }
    }
}
