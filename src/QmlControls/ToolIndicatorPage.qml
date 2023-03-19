/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.11
import QtQuick.Layouts  1.11

import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0

// ToolIndicatorPage 
//      The base control for all Toolbar Indicator drop down pages. It supports a normal and expanded view.

Row {
    property bool showExpand:       false   // Controls whether the expand widget is shown or not
    property Item contentItem               // Item for the normal view portion of the page
    property Item expandedItem              // Item for the expanded portion of the page
    property var  editFieldWidth:   ScreenTools.defaultFontPixelWidth * 13
 
    // These properties are bound by the MainRoowWindow loader
    property bool expanded: false            
    property var  drawer

    id:         _root
    spacing:    _margins

    property real _margins: ScreenTools.defaultFontPixelHeight

    onContentItemChanged: {
        if (_root.contentItem) {
            _root.contentItem.parent = _contentItem
        }
    }

    onExpandedItemChanged: {
        if (_root.expandedItem) {
            _root.expandedItem.parent = _expandedItem
        }
    }

    Item {
        id:                 _contentItem
        width:              childrenRect.width
        height:             childrenRect.height
    }

    Rectangle {
        width:              1
        height:             Math.max(_contentItem.height, _expandedItem.height)
        color:              QGroundControl.globalPalette.text
        visible:            expanded
    }

    Item {
        id:                 _expandedItem
        width:              childrenRect.width
        height:             childrenRect.height
        visible:            expanded
    }
}
