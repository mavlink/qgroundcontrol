/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick                  2.12
import QtQuick.Controls         2.4
import QtQuick.Dialogs          1.3
import QtQuick.Layouts          1.12

import QtLocation               5.3
import QtPositioning            5.3
import QtQuick.Window           2.2
import QtQml.Models             2.1

import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.Controllers   1.0
import QGroundControl.FlightDisplay 1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.FlightMap     1.0


// This is the ui overlay layer for the widgets/tools for Fly View
Item {
    id: _root
    property var insetsToView: QGCToolInsets {}
    property int _linethickness: 3

    Rectangle {
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        anchors.leftMargin: parent.width/8
        height: checkboxcolumn.height
        width: checkboxcolumn.width
        color: "dimgray"

        ColumnLayout {
            id: checkboxcolumn
            QGCCheckBox{
                checked: true
                text: "leftEdgeTopInset"
                onClicked: leftEdgeTopInset.visible = checked
                textColor: leftEdgeTopInset.color
                textBold: true
            }
            QGCCheckBox{
                checked: true
                text: "leftEdgeBottomInset"
                onClicked: leftEdgeBottomInset.visible = checked
                textColor: leftEdgeBottomInset.color
                textBold: true
            }
            QGCCheckBox{
                checked: true
                text: "rightEdgeTopInset"
                onClicked: rightEdgeTopInset.visible = checked
                textColor: rightEdgeTopInset.color
                textBold: true
            }
            QGCCheckBox{
                checked: true
                text: "rightEdgeBottomInset"
                onClicked: rightEdgeBottomInset.visible = checked
                textColor: rightEdgeBottomInset.color
                textBold: true
            }
            QGCCheckBox{
                checked: true
                text: "topEdgeLeftInset"
                onClicked: topEdgeLeftInset.visible = checked
                textColor: topEdgeLeftInset.color
                textBold: true
            }
            QGCCheckBox{
                checked: true
                text: "topEdgeRightInset"
                onClicked: topEdgeRightInset.visible = checked
                textColor: topEdgeRightInset.color
                textBold: true
            }
            QGCCheckBox{
                checked: true
                text: "bottomEdgeLeftInset"
                onClicked: bottomEdgeLeftInset.visible = checked
                textColor: bottomEdgeLeftInset.color
                textBold: true
            }
            QGCCheckBox{
                checked: true
                text: "bottomEdgeRightInset"
                onClicked: bottomEdgeRightInset.visible = checked
                textColor: bottomEdgeRightInset.color
                textBold: true
            }
            QGCCheckBox{
                checked: true
                text: "centerInsetRect"
                onClicked: centerInsetRect.visible = checked
                textColor: centerInsetRect.border.color
                textBold: true
            }
        }

    }

    Rectangle { // leftEdgeTopInset
        id: leftEdgeTopInset
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.leftMargin: insetsToView.leftEdgeTopInset
        height: parent.height
        width: _linethickness
        color: "coral"
    }
    Rectangle { // leftEdgeBottomInset
        id: leftEdgeBottomInset
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.leftMargin: insetsToView.leftEdgeBottomInset
        height: parent.height
        width: _linethickness
        color: "cornflowerblue"
    }
    Rectangle { // rightEdgeTopInset
        id: rightEdgeTopInset
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.rightMargin: insetsToView.rightEdgeTopInset
        height: parent.height
        width: _linethickness
        color: "darkslateblue"
    }
    Rectangle { // rightEdgeBottomInset
        id: rightEdgeBottomInset
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.rightMargin: insetsToView.rightEdgeBottomInset
        height: parent.height
        width: _linethickness
        color: "firebrick"
    }
    Rectangle { // topEdgeLeftInset
        id: topEdgeLeftInset
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.topMargin: insetsToView.topEdgeLeftInset
        height: _linethickness
        width: parent.width
        color: "mediumseagreen"
    }
    Rectangle { // topEdgeRightInset
        id: topEdgeRightInset
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.topMargin: insetsToView.topEdgeRightInset
        height: _linethickness
        width: parent.width
        color: "moccasin"
    }
    Rectangle { // bottomEdgeLeftInset
        id: bottomEdgeLeftInset
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.bottomMargin: insetsToView.bottomEdgeLeftInset
        height: _linethickness
        width: parent.width
        color: "salmon"
    }
    Rectangle { // bottomEdgeRightInset
        id: bottomEdgeRightInset
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.bottomMargin: insetsToView.bottomEdgeRightInset
        height: _linethickness
        width: parent.width
        color: "slategrey"
    }
    Rectangle {
        id: centerInsetRect
        x: insetsToView.leftEdgeCenterInset
        y: insetsToView.topEdgeCenterInset
        width: _root.width - insetsToView.leftEdgeCenterInset - insetsToView.rightEdgeCenterInset
        height: _root.height - insetsToView.topEdgeCenterInset - insetsToView.bottomEdgeCenterInset
        color: "transparent"
        border.color: "yellow"
        border.width: 1
        radius: 0
    }
}
