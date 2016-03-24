/*=====================================================================

 QGroundControl Open Source Ground Control Station

 (c) 2009 - 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

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

/// @file
///     @author Don Gagne <don@thegagnes.com>

import QtQuick 2.3
import QtQuick.Controls 1.3

import QGroundControl.FactSystem 1.0
import QGroundControl.Controls 1.0
import QGroundControl.Palette 1.0

FocusScope {
    property alias color: rectangle.color

    property string __missingParams:    ""
    property string __errorMsg:         ""

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    function showMissingParameterOverlay(missingParamName) {
        if (__missingParams.length != 0) {
            __missingParams = __missingParams.concat(", ")
        }
        __missingParams = __missingParams.concat(missingParamName)
        __missingParamsOverlay.visible = true
    }

    function showError(errorMsg) {
        __errorMsg = errorMsg
        __missingParamsOverlay.visible = true
    }

    Rectangle {
        id:     rectangle
        color: qgcPal.window

        Rectangle {
            id:             __missingParamsOverlay
            anchors.fill:   parent
            z:              9999
            visible:        false
            color:          qgcPal.window
            opacity:        0.85

            QGCLabel {
                anchors.fill:   parent
                wrapMode:       Text.WordWrap
                text:           __errorMsg.length ? __errorMsg : qsTr("Parameters(s) missing: %1").arg(__missingParams)
            }
        }
    }
}
