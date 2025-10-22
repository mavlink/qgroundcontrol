/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls

import QGroundControl
import QGroundControl.Controls

import "."

Item {
    id: root

    readonly property real _horizontalMargin: ScreenTools.defaultFontPixelWidth * 1.5
    readonly property real _verticalMargin: ScreenTools.defaultFontPixelHeight * 1.5
    readonly property real _spacing: ScreenTools.defaultFontPixelHeight * 0.5

    anchors.fill: parent

    Row {
        id: backButtonRow
        anchors.left: parent.left
        anchors.leftMargin: _horizontalMargin
        anchors.top: parent.top
        anchors.topMargin: _verticalMargin
        spacing: _spacing
        visible: typeof mainWindow !== "undefined"

        QGCLabel {
            text: "<"
            font.pixelSize: ScreenTools.defaultFontPixelHeight * 1.2
        }

        QGCLabel {
            text: qsTr("Back to Fly View")
            font.pixelSize: ScreenTools.defaultFontPixelHeight * 1.2
        }
    }

    QGCMouseArea {
        anchors.fill: backButtonRow
        enabled: backButtonRow.visible
        onClicked: {
            if (typeof mainWindow !== "undefined" && mainWindow.allowViewSwitch()) {
                mainWindow.showFlyView()
            }
        }
    }

    QGCLabel {
        id: titleLabel
        text: qsTr("Vehicle Profiles")
        font.pixelSize: ScreenTools.defaultFontPixelHeight * 1.4
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: backButtonRow.bottom
        anchors.leftMargin: _horizontalMargin
        anchors.rightMargin: _horizontalMargin
        anchors.topMargin: _spacing
        wrapMode: Text.WordWrap
    }

    QGCLabel {
        id: emptyStateLabel
        text: qsTr("No vehicle profiles defined.")
        color: QGroundControl.globalPalette.text
        visible: profilesList.count === 0
        height: visible ? implicitHeight : 0
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: titleLabel.bottom
        anchors.leftMargin: _horizontalMargin
        anchors.rightMargin: _horizontalMargin
        anchors.topMargin: _spacing
        wrapMode: Text.WordWrap
    }

    VehicleProfilesList {
        id: profilesList
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: emptyStateLabel.bottom
        anchors.bottom: parent.bottom
        anchors.leftMargin: _horizontalMargin
        anchors.rightMargin: _horizontalMargin
        anchors.topMargin: _spacing
        anchors.bottomMargin: _verticalMargin
        visible: count > 0
    }
}
