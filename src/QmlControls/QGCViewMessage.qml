/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @author Don Gagne <don@thegagnes.com>

import QtQuick 2.3
import QtQuick.Controls 1.2

import QGroundControl.Controls 1.0
import QGroundControl.Palette 1.0

QGCViewDialog {
    property string message

    QGCFlickable {
        anchors.fill:   parent
        contentHeight:  label.contentHeight

        QGCLabel {
            id:             label
            anchors.left:   parent.left
            anchors.right:  parent.right
            wrapMode:       Text.WordWrap
            text:           message
        }
    }
}
