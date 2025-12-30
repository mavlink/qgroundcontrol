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
import QtQuick.Dialogs
import QtQuick.Layouts

import QtLocation
import QtPositioning
import QtQuick.Window
import QtQml.Models

import QGroundControl

import QGroundControl.Controls

import QGroundControl.FlightDisplay
import QGroundControl.FlightMap




// To implement a custom overlay copy this code to your own control in your custom code source. Then override the
// FlyViewCustomLayer.qml resource with your own qml. See the custom example and documentation for details.
Item {
    id: _root

    property var parentToolInsets               // These insets tell you what screen real estate is available for positioning the controls in your overlay
    property var totalToolInsets:   _toolInsets // These are the insets for your custom overlay additions
    property var mapControl

    // since this file is a placeholder for the custom layer in a standard build, we will just pass through the parent insets
    QGCToolInsets {
        id:                     _toolInsets
        leftEdgeTopInset:       parentToolInsets.leftEdgeTopInset
        leftEdgeCenterInset:    parentToolInsets.leftEdgeCenterInset
        leftEdgeBottomInset:    parentToolInsets.leftEdgeBottomInset
        rightEdgeTopInset:      parentToolInsets.rightEdgeTopInset
        rightEdgeCenterInset:   parentToolInsets.rightEdgeCenterInset
        rightEdgeBottomInset:   parentToolInsets.rightEdgeBottomInset
        topEdgeLeftInset:       parentToolInsets.topEdgeLeftInset
        topEdgeCenterInset:     parentToolInsets.topEdgeCenterInset
        topEdgeRightInset:      parentToolInsets.topEdgeRightInset
        bottomEdgeLeftInset:    parentToolInsets.bottomEdgeLeftInset
        bottomEdgeCenterInset:  parentToolInsets.bottomEdgeCenterInset
        bottomEdgeRightInset:   parentToolInsets.bottomEdgeRightInset
    }
    Rectangle{
        property real   _margins:      ScreenTools.defaultFontPixelHeight / 2
        property real   _smallMargins: ScreenTools.defaultFontPixelWidth / 2
        property real   _spacing:      ScreenTools.defaultFontPixelWidth * 5
        id:shortcutIndicator
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        width: parent.width * 0.1
        height: ScreenTools.defaultFontPixelHeight + 4
        color:      Qt.rgba(qgcPal.window.r, qgcPal.window.g, qgcPal.window.b, 0.5)
        radius:     _margins

        LabelledLabel{
            anchors.leftMargin: _margins * 2
            anchors.rightMargin: _margins * 2
            id: infoLabel
            anchors.fill: parent
        }

        Connections{
            target:  mainWindow
            onShortcutPressed: function(description,value){
                infoLabel.label = description
                infoLabel.labelText = value
                console.log(description.length * ScreenTools.defaultFontPixelWidth)
                shortcutIndicator.width = description.length * ScreenTools.defaultFontPixelWidth + value.length * ScreenTools.defaultFontPixelWidth + shortcutIndicator._spacing
            }
        }

    }

}
