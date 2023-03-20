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

RowLayout {
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
            _root.contentItem.parent = _contentItemHolder
        }
    }

    onExpandedItemChanged: {
        if (_root.expandedItem) {
            _root.expandedItem.parent = _expandedItemHolder
        }
    }

    ColumnLayout {
        id:                 _contentItemHolder
        Layout.alignment:   Qt.AlignTop
    }

    Rectangle {
        id:                     divider
        Layout.preferredWidth:  visible ? 1 : -1
        Layout.fillHeight:      true
        color:                  QGroundControl.globalPalette.text
        visible:                expanded
    }

    ColumnLayout {
        id:                     _expandedItemHolder
        Layout.alignment:       Qt.AlignTop
        Layout.preferredWidth:  visible ? -1 : 0
        visible:                expanded
    }
}
