/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009, 2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

This file is part of the QGROUNDCONTROL project

    QGROUNDCONTROL is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    QGROUNDCONTROL is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

import QtQuick          2.4
import QtQuick.Controls 1.2

import QGroundControl.Controls  1.0
import QGroundControl.Palette   1.0

Item {
    id: _root

    property alias          source:  icon.source
    property bool           checked: false
    property ExclusiveGroup exclusiveGroup:  null

    signal   clicked()

    QGCPalette { id: qgcPal }

    onExclusiveGroupChanged: {
        if (exclusiveGroup) {
            exclusiveGroup.bindCheckable(_root)
        }
    }

    Rectangle {
        anchors.fill:   parent
        color:          qgcPal.buttonHighlight
        visible:        checked
    }

    QGCColoredImage {
        id:         icon
        width:      parent.height * 0.9
        height:     parent.height * 0.9
        fillMode:   Image.PreserveAspectFit
        color:      checked ? qgcPal.buttonHighlightText : qgcPal.buttonText
        anchors.verticalCenter:     parent.verticalCenter
        anchors.horizontalCenter:   parent.horizontalCenter
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {
            checked = true
            _root.clicked()
        }
    }
}
