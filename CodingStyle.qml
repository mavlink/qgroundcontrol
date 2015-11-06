/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009, 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

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

import QtQuick          2.5
import QtQuick.Controls 1.3

import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0

/// This is an example Qml file which is used to describe the QGroundControl coding style.
/// In general almost everything in here has some coding style meaning including order of
/// code. Not all style choices are explained. If there is any confusison please ask
/// and we'll answer and update style as needed.
Item {
    // Property binding to item properties
    width:  ScreenTools.defaultFontPixelHeight * 10 // No hardcoded sizing. All sizing must be relative to a ScreenTools font size
    height: ScreenTools.defaultFontPixelHeight * 20

    // Property definitions available to consumers of this Qml Item come first
    property int    myIntProperty:  10
    property real   myRealProperty: 20.0

    // Property definitions which are internal to the item are prepending with an underscore
    // to signal private and come second
    readonly property real _rectWidth:  ScreenTools.defaultFontPixelWidth * 10  // Use readonly appropriately to increase binding performance
    readonly property real _rectHeight: ScreenTools.defaultFontPixelWidth * 10

    function myFunction() {
        console.log("myFunction was called")
    }

    QGCPalette {
        id:                 qgcPal  // Not how id does not use an underscore
        colorGroupEnabled:  enabled
    }

    // You should always use the QGC provided variants of base control since they automatically support
    // our theming and font support.
    QGCButton {
        // Also not how there is no id: specified for this control. Only add id: if it is needed.
        text: "Button"

        onClicked: myFunction()
    }

    Rectangle {
        width:  _rectWidth
        height: _rectHeight
        color:  qgcPal.window   // Use QGC palette colors for everything, no hardcoded colors
    }

    // For scoped blocks which are long include a comment so you can tell what the brace is matching.
    // This is very handy when the top level brace scrolls off the screen. The endbrace comment in this
    // specific file is only included as style info. This example code is not long enough to really need it.
} // Item - CodingStyle
