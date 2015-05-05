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

Item {
    property string __missingFacts: ""

    function showMissingFactOverlay(missingFactName) {
            if (__missingFacts.length != 0) {
                __missingFacts = __missingFacts.concat(", ")
            }
        __missingFacts = __missingFacts.concat(missingFactName)
        __missingFactOverlay.visible = true
    }

    Rectangle {
        QGCPalette { id: __qgcPal; colorGroupEnabled: true }

        id:             __missingFactOverlay
        anchors.fill:   parent
        z:              9999
        visible:        false
        color:          __qgcPal.window
        opacity:        0.85

        QGCLabel {
            anchors.fill:   parent
            wrapMode:       Text.WordWrap
            text:           "Fact(s) missing: " + __missingFacts
        }
    }
}
