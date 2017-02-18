/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.4
import QtQuick.Controls 1.2

import QGroundControl.ScreenTools   1.0
import QGroundControl.Palette       1.0

Rectangle {
    id:         _root
    color:      qgcPal.window
    width:      ScreenTools.defaultFontPixelWidth * 6
    height:     buttonStripColumn.height + (buttonStripColumn.anchors.margins * 2)
    radius:     _radius

    property string title:              "Title"
    property alias  model:              repeater.model
    property var    showAlternateIcon
    property var    rotateImage
    property var    buttonEnabled
    property var    buttonVisible

    signal clicked(int index, bool checked)

    readonly property real  _radius:                ScreenTools.defaultFontPixelWidth / 2
    readonly property real  _margin:                ScreenTools.defaultFontPixelWidth / 2
    readonly property real  _buttonSpacing:         ScreenTools.defaultFontPixelWidth
    readonly property bool  _showOptionalElements:  !ScreenTools.isShortScreen

    QGCPalette { id: qgcPal }
    ExclusiveGroup { id: dropButtonsExclusiveGroup }

    function uncheckAll() {
        dropButtonsExclusiveGroup.current = null
        // Signal all toggles as off
        for (var i=0; i<model.length; i++) {
            if (model[i].toggleButton === true) {
                clicked(index, false)
            }
        }
    }

    MouseArea {
        x:          -_root.x
        y:          -_root.y
        width:      _root.parent.width
        height:     _root.parent.height
        visible:    dropPanel.visible
        onClicked:  dropPanel.hide()
    }

    Column {
        id:                 buttonStripColumn
        anchors.margins:    ScreenTools.defaultFontPixelWidth  / 2
        anchors.top:        parent.top
        anchors.left:       parent.left
        anchors.right:      parent.right

        QGCLabel {
            anchors.horizontalCenter:   parent.horizontalCenter
            text:                       title
            visible:                    _showOptionalElements
        }

        Item { width: 1; height: _buttonSpacing; visible: _showOptionalElements }

        Rectangle {
            anchors.left:       parent.left
            anchors.right:      parent.right
            height:             1
            color:              qgcPal.text
            visible:            _showOptionalElements
        }

        Repeater {
            id: repeater

            delegate: Column {
                id:         buttonColumn
                width:      buttonStripColumn.width
                visible:    _root.buttonVisible ? _root.buttonVisible[index] : true

                property bool checked: false
                property ExclusiveGroup exclusiveGroup: dropButtonsExclusiveGroup

                property var    _iconSource:        modelData.iconSource
                property var    _alternateIconSource:   modelData.alternateIconSource
                property var    _source:                (_root.showAlternateIcon && _root.showAlternateIcon[index]) ? _alternateIconSource : _iconSource
                property bool   rotateImage:            _root.rotateImage ? _root.rotateImage[index] : false

                onExclusiveGroupChanged: {
                    if (exclusiveGroup) {
                        exclusiveGroup.bindCheckable(buttonColumn)
                    }
                }

                onRotateImageChanged: {
                    if (rotateImage) {
                        imageRotation.running = true
                    } else {
                        imageRotation.running = false
                        button.rotation = 0
                    }
                }

                Item {
                    width:      1
                    height:     _buttonSpacing
                    visible:    index == 0 ? _showOptionalElements : true
                }

                Rectangle {
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    height:         width
                    color:          checked ? qgcPal.buttonHighlight : qgcPal.button

                    QGCColoredImage {
                        id:                 button
                        anchors.fill:       parent
                        source:             _source
                        sourceSize.height:  parent.height
                        fillMode:           Image.PreserveAspectFit
                        mipmap:             true
                        smooth:             true
                        color:              checked ? qgcPal.buttonHighlightText : qgcPal.buttonText

                        RotationAnimation on rotation {
                            id:             imageRotation
                            loops:          Animation.Infinite
                            from:           0
                            to:             360
                            duration:       500
                            running:        false
                        }

                    }

                    MouseArea {
                        anchors.left:   parent.left
                        anchors.right:  parent.right
                        anchors.top:    parent.top
                        height:         parent.height + (_showOptionalElements? buttonLabel.height + buttonColumn.spacing : 0)
                        visible:        _root.buttonEnabled ? _root.buttonEnabled[index] : true

                        onClicked: {
                            if (modelData.dropPanelComponent === undefined) {
                                dropPanel.hide()
                                if (modelData.toggle === true) {
                                    checked = !checked
                                } else {
                                    // dropPanel.hide above will close panel, but we need to do this to clear toggles
                                    uncheckAll()
                                }
                                _root.clicked(index, checked)
                            } else {
                                if (checked) {
                                    dropPanel.hide()    // hide affects checked, so this needs to be duplicated inside not outside if
                                } else {
                                    dropPanel.hide()    // hide affects checked, so this needs to be duplicated inside not outside if
                                    checked = true
                                    var panelEdgeTopPoint = mapToItem(_root, width, 0)
                                    dropPanel.show(panelEdgeTopPoint, height, modelData.dropPanelComponent)
                                }
                            }
                        }
                    }
                }

                QGCLabel {
                    id:                         buttonLabel
                    anchors.horizontalCenter:   parent.horizontalCenter
                    font.pointSize:             ScreenTools.smallFontPointSize
                    text:                       modelData.name
                    visible:                    _showOptionalElements
                }
            }
        }
    }

    DropPanel {
        id:         dropPanel
        toolStrip:  _root
    }
}
