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
///     @brief Battery, propeller and magnetometer summary
///     @author Gus Grubba <mavlink@grubba.com>

import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2

import QGroundControl.FactSystem 1.0
import QGroundControl.FactControls 1.0
import QGroundControl.Controls 1.0

Column {
    anchors.fill: parent
    anchors.margins: 8

    Row {
        width: parent.width
        QGCLabel { id: battFull; text: "Battery Full:" }
        FactLabel {
            fact: Fact { name: "BAT_V_CHARGED" }
            horizontalAlignment: Text.AlignRight;
            width: parent.width - battFull.contentWidth;
        }
    }

    Row {
        width: parent.width
        QGCLabel { id: battEmpty; text: "Battery Empty:" }
        FactLabel {
            fact: Fact { name: "BAT_V_EMPTY" }
            horizontalAlignment: Text.AlignRight;
            width: parent.width - battEmpty.contentWidth;
        }
    }

    Row {
        width: parent.width
        QGCLabel { id: battCells; text: "Number of Cells:" }
        FactLabel {
            fact: Fact { name: "BAT_N_CELLS" }
            horizontalAlignment: Text.AlignRight;
            width: parent.width - battCells.contentWidth;
        }
    }

    Row {
        width: parent.width
        QGCLabel { id: battDrop; text: "Voltage Drop:" }
        FactLabel {
            fact: Fact { name: "BAT_V_LOAD_DROP" }
            horizontalAlignment: Text.AlignRight;
            width: parent.width - battDrop.contentWidth;
        }
    }

}
