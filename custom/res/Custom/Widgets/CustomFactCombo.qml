/****************************************************************************
 *
 *   (c) 2009-2019 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick                      2.11
import QtQuick.Controls             1.2
import QtQuick.Controls.Styles      1.4

import QGroundControl.FactSystem    1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0

import Custom.Widgets             1.0

Item {
    id:                 _root
    width:              background.width
    height:             background.height

    property Fact       fact:           Fact { }
    property bool       indexModel:     true  ///< true: model must be specifed, selected index is fact value, false: use enum meta data
    property real       pointSize:      ScreenTools.smallFontPointSize
    property string     text:           ""
    property int        currentIndex:   comboBox.currentIndex
    property real       level:          0.5

    CustomTextBackground {
        id:                         background
        contentWidth:               menuRow.width
        contentHeight:              labelText.height * 2
        opacity:                    parent.level
    }
    Image {
        source:                     "/custom/img/menu_dropdown.svg"
        height:                     background.height * 0.25
        width:                      height
        antialiasing:               true
        sourceSize.height:          height
        fillMode:                   Image.PreserveAspectFit
        anchors.right:              background.right
        anchors.rightMargin:        background.height * 0.25
        anchors.verticalCenter:     background.verticalCenter
    }
    Row {
        id:                         menuRow
        spacing:                    _root.text !== "" ? ScreenTools.defaultFontPixelWidth * 0.25 : 0
        anchors.centerIn:           parent
        QGCLabel {
            id:                     labelText
            color:                  "#AAAAAA"
            text:                   _root.text
            visible:                _root.text !== ""
            font.pointSize:         _root.pointSize
            anchors.verticalCenter: parent.verticalCenter
        }
        CustomComboBox {
            id:                     comboBox
            model:                  _root.fact ? _root.fact.enumStrings : []
            centeredLabel:          true
            pointSize:              _root.pointSize
            currentIndex:           _root.fact ? (_root.indexModel ? _root.fact.value : _root.fact.enumIndex) : 0
            onActivated: {
                if (_root.indexModel) {
                    _root.fact.value = index
                } else {
                    _root.fact.value = _root.fact.enumValues[index]
                }
            }
        }
    }
    MouseArea {
        anchors.fill: parent
        onClicked: {
            comboBox.clicked()
        }
    }
}
