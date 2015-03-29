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

import QtQuick 2.2
import QtQuick.Controls 1.2
import QGroundControl.FactSystem 1.0

Item {
    // Use default component id
    TextInput {
        objectName: "testControl"
        Fact { id: fact1; name: "RC_MAP_THROTTLE" }
        text: fact1.value
        onAccepted: { fact1.value = text; }
    }

    // Use specific component id
    TextInput {
        objectName: "testControl"
        Fact { id: fact2; name: "COMPONENT_51"; componentId: 51 }
        text: fact2.value
        onAccepted: { fact2.value = text; }
    }
}
