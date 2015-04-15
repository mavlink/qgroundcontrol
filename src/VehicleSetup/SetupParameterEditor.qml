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

import QtQuick 2.3
import QtQuick.Controls 1.2

import QGroundControl.Controls 1.0
import QGroundControl.ScreenTools 1.0
import QGroundControl.Palette 1.0

Rectangle {
	QGCPalette { id: qgcPal; colorGroupEnabled: true }
	ScreenTools { id: screenTools }

    color: qgcPal.window

    // We use an ExclusiveGroup to maintain the visibility of a single editing control at a time
    ExclusiveGroup {
        id: exclusiveEditorGroup
    }

    Column {
        anchors.fill:parent

        QGCLabel {
            text: "PARAMETER EDITOR"
            font.pointSize: screenTools.dpiAdjustedPointSize(20)
        }

        Item {
            height: 20
            width:	5
        }

		QGCLabel {
			id: infoLabel
			width:      parent.width
			wrapMode:   Text.WordWrap
			text:       "Click a parameter value to modify. Right-click to set an RC to Param mapping. Use caution when modifying parameters here since the values are not checked for validity."
		}

		ParameterEditor {
			width:	parent.width
			height: parent.height - (infoLabel.y + infoLabel.height)
		}
    }
}
