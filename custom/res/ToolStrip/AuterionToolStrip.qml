/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.11
import QtQuick.Controls 1.4

import QGroundControl.ScreenTools   1.0
import QGroundControl.Palette       1.0

Rectangle {
    id:         _root
    color:      qgcPal.window
    width:      ScreenTools.isMobile ? ScreenTools.minTouchPixels : ScreenTools.defaultFontPixelWidth * 7
    height:     toolStripColumn.height + (toolStripColumn.anchors.margins * 2)
    radius:     _radius
    border.width:   1
    border.color:   qgcPal.globalTheme === QGCPalette.Light ? Qt.rgba(0,0,0,0.35) : Qt.rgba(1,1,1,0.35)

    property string title:              "Title"
    property alias  model:              repeater.model
    property var    showAlternateIcon                   ///< List of bool values, one for each button in strip - true: show alternate icon, false: show normal icon
    property var    rotateImage                         ///< List of bool values, one for each button in strip - true: animation rotation, false: static image
    property var    animateImage                        ///< List of bool values, one for each button in strip - true: animate image, false: static image
    property var    buttonEnabled                       ///< List of bool values, one for each button in strip - true: button enabled, false: button disabled
    property var    buttonVisible                       ///< List of bool values, one for each button in strip - true: button visible, false: button invisible
    property real   maxHeight                           ///< Maximum height for control, determines whether text is hidden to make control shorter

    signal clicked(int index, bool checked)

    readonly property real  _radius:                ScreenTools.defaultFontPixelWidth / 2
    readonly property real  _margin:                ScreenTools.defaultFontPixelWidth / 2
    readonly property real  _buttonSpacing:         ScreenTools.defaultFontPixelHeight / 4

    QGCPalette { id: qgcPal }
    ExclusiveGroup { id: dropButtonsExclusiveGroup }

    function uncheckAll() {
        dropButtonsExclusiveGroup.current = null
        // Signal all toggles as off
        for (var i=0; i<model.length; i++) {
            if (model[i].toggle === true) {
                _root.clicked(i, false)
            }
        }
    }

    DeadMouseArea {
        anchors.fill: parent
    }

    Column {
        id:                 toolStripColumn
        anchors.margins:    ScreenTools.defaultFontPixelWidth  / 2
        anchors.top:        parent.top
        anchors.left:       parent.left
        anchors.right:      parent.right
        spacing:            _buttonSpacing

        QGCLabel {
            anchors.horizontalCenter:   parent.horizontalCenter
            text:                       title
            font.pointSize:             ScreenTools.mobile ? ScreenTools.smallFontPointSize : ScreenTools.defaultFontPointSize
        }

        Rectangle {
            anchors.left:       parent.left
            anchors.right:      parent.right
            height:             1
            color:              qgcPal.text
        }

        Column {
            anchors.left:   parent.left
            anchors.right:  parent.right
            spacing:        _buttonSpacing

            Repeater {
                id: repeater

                delegate: FocusScope {
                    id:         scope
                    width:      toolStripColumn.width
                    height:     buttonRect.height
                    visible:    _root.buttonVisible ? _root.buttonVisible[index] : true

                    property bool checked: false
                    property ExclusiveGroup exclusiveGroup: dropButtonsExclusiveGroup

                    property bool   _buttonEnabled:         _root.buttonEnabled ? _root.buttonEnabled[index] : true
                    property var    _iconSource:            modelData.iconSource
                    property var    _alternateIconSource:   modelData.alternateIconSource
                    property var    _source:                (_root.showAlternateIcon && _root.showAlternateIcon[index]) ? _alternateIconSource : _iconSource
                    property bool   rotateImage:            _root.rotateImage ? _root.rotateImage[index] : false
                    property bool   animateImage:           _root.animateImage ? _root.animateImage[index] : false
                    property bool   _hovered:               false
                    property bool   _showHighlight:         checked || (_buttonEnabled && _hovered)

                    QGCPalette { id: _repeaterPal; colorGroupEnabled: _buttonEnabled }

                    onExclusiveGroupChanged: {
                        if (exclusiveGroup) {
                            exclusiveGroup.bindCheckable(scope)
                        }
                    }

                    onRotateImageChanged: {
                        if (rotateImage) {
                            imageRotation.running = true
                        } else {
                            imageRotation.running = false
                            buttonImage.rotation = 0
                        }
                    }

                    onAnimateImageChanged: {
                        if (animateImage) {
                            opacityAnimation.running = true
                        } else {
                            opacityAnimation.running = false
                            buttonImage.opacity = 1
                        }
                    }

                    Rectangle {
                        id:             buttonRect
                        anchors.left:   parent.left
                        anchors.right:  parent.right
                        height:         buttonColumn.height
                        color:          _showHighlight ? _repeaterPal.buttonHighlight : _repeaterPal.window

                        Column {
                            id:             buttonColumn
                            anchors.left:   parent.left
                            anchors.right:  parent.right
                            spacing:        -buttonImage.height / 8

                            QGCColoredImage {
                                id:             buttonImage
                                anchors.left:   parent.left
                                anchors.right:  parent.right
                                height:         width * 0.8
                                //anchors.centerIn:   parent
                                source:             _source
                                sourceSize.height:  height
                                fillMode:           Image.PreserveAspectFit
                                mipmap:             true
                                smooth:             true
                                color:              _showHighlight ? _repeaterPal.buttonHighlightText : _repeaterPal.text

                                RotationAnimation on rotation {
                                    id:             imageRotation
                                    loops:          Animation.Infinite
                                    from:           0
                                    to:             360
                                    duration:       500
                                    running:        false
                                }

                                NumberAnimation on opacity {
                                    id:         opacityAnimation
                                    running:    false
                                    from:       0
                                    to:         1.0
                                    loops:      Animation.Infinite
                                    duration:   2000
                                }
                            }

                            QGCLabel {
                                id:                         buttonLabel
                                anchors.horizontalCenter:   parent.horizontalCenter
                                font.pointSize:             ScreenTools.smallFontPointSize
                                text:                       modelData.name
                                color:                      _showHighlight ? _repeaterPal.buttonHighlightText : _repeaterPal.text
                                enabled:                    _buttonEnabled
                            }
                        }  // Column

                        QGCMouseArea {
                            anchors.fill:       parent
                            visible:            _buttonEnabled
                            hoverEnabled:       true
                            preventStealing:    true

                            onContainsMouseChanged: _hovered = containsMouse
                            onContainsPressChanged: _hovered = containsPress

                            onClicked: {
                                scope.focus = true
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
                                        uncheckAll()
                                        checked = true
                                        var panelEdgeTopPoint = mapToItem(_root, width, 0)
                                        dropPanel.show(panelEdgeTopPoint, height, modelData.dropPanelComponent)
                                    }
                                }
                            }
                        }
                    } // Rectangle
                } // FocusScope
            }
        }
    }

    DropPanel {
        id:         dropPanel
        toolStrip:  _root
    }
}
