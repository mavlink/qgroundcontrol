/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlightDisplay
import QGroundControl.Palette
import QGroundControl.ScreenTools

Rectangle {
    id:             bottomStrip
    height:         Math.max(ScreenTools.minTouchPixels * 0.94, ScreenTools.defaultFontPixelHeight * 2.94)
    implicitWidth:  _contentPreferredWidth
    color:          "transparent"
    radius:         Math.round(ScreenTools.defaultFontPixelWidth * 0.78)
    border.color:   "transparent"
    border.width:   0
    clip:           false

    property var  _guidedController:   globals.guidedControllerFlyView
    property var  backdropSourceItem:  null
    property real _stripMargin:        ScreenTools.defaultFontPixelWidth * 0.38
    property real _rowSpacing:         ScreenTools.defaultFontPixelWidth * 0.38
    property real _operationSeparatorExtraGap: ScreenTools.defaultFontPixelWidth * 2
    property real _toolButtonWidth:    Math.max(ScreenTools.defaultFontPixelWidth * 7.35, ScreenTools.minTouchPixels * 1.02)
    property real _toolIconSize:       ScreenTools.defaultFontPixelHeight * 1.26
    property int  _visibleToolActionCount: visibleToolActionCount()
    property int  _operationSeparatorCount: operationSeparatorCount()
    property real _toolActionsWidth:   _visibleToolActionCount > 0 ?
                                            ((_visibleToolActionCount * _toolButtonWidth) +
                                             (Math.max(0, _visibleToolActionCount - 1) * _rowSpacing) +
                                             (_operationSeparatorCount * _operationSeparatorExtraGap)) : 0
    property real _contentPreferredWidth: (_stripMargin * 2) + _toolActionsWidth
    property real spacing:             0

    signal displayPreFlightChecklist

    QGCPalette { id: qgcPal }
    FlightModeDisplay { id: flightModeDisplay }

    GlassBackdrop {
        anchors.fill:       parent
        sourceItem:         bottomStrip.backdropSourceItem
        backdropBlurEnabled:true
        targetItem:         bottomStrip
        cornerRadius:       bottomStrip.radius
        sourceScale:        0.42
        blurAmount:         0.94
        blurMax:            42
        sourceBrightness:   -0.01
        sourceSaturation:   0.62
        tintColor:          Qt.rgba(0.045, 0.048, 0.052, 0.68)
        sheenColor:         "transparent"
    }

    FlyViewToolStripActionList {
        id: flyActionList
        onDisplayPreFlightChecklist: bottomStrip.displayPreFlightChecklist()
    }

    ToolStripDropPanel {
        id:                 dropPanel
        toolStrip:          bottomStrip
        allowOutsideParent: true
        z:                  QGroundControl.zOrderWidgets + 1
    }

    function clearOtherActionChecks(activeIndex) {
        for (var i = 0; i < actionRepeater.count; i++) {
            if (i !== activeIndex) {
                var item = actionRepeater.itemAt(i)
                if (item) {
                    item.checked = false
                }
            }
        }
    }

    function actionVisible(action) {
        return action && action.visible && action.enabled
    }

    function actionIsNonOperation(action) {
        return action && typeof action.nonOperationAction !== "undefined" && action.nonOperationAction
    }

    function showOperationSeparatorBefore(actionIndex, action) {
        if (!actionVisible(action) || actionIsNonOperation(action)) {
            return false
        }

        for (var i = actionIndex - 1; i >= 0; i--) {
            var previousAction = flyActionList.model[i]
            if (actionVisible(previousAction)) {
                return actionIsNonOperation(previousAction)
            }
        }

        return false
    }

    function operationSeparatorCount() {
        var count = 0
        for (var i = 0; i < flyActionList.model.length; i++) {
            if (showOperationSeparatorBefore(i, flyActionList.model[i])) {
                count++
            }
        }
        return count
    }

    function visibleToolActionCount() {
        var count = 0
        for (var i = 0; i < flyActionList.model.length; i++) {
            if (actionVisible(flyActionList.model[i])) {
                count++
            }
        }
        return count
    }

    component ActionButton: Rectangle {
        id: actionButton

        property var  toolStripAction: null
        property bool checked:         toolStripAction ? toolStripAction.checked : false
        property bool checkable:       toolStripAction && (toolStripAction.dropPanelComponent || toolStripAction.checkable)
        property bool available:       toolStripAction && toolStripAction.enabled
        property int  actionIndex:     -1
        property string imageSource:   toolStripAction ? (toolStripAction.showAlternateIcon ? toolStripAction.alternateIconSource : toolStripAction.iconSource) : ""
        property string displayText:   toolStripAction ? String(toolStripAction.text).split("|")[0] : ""
        property string displayLabelText: flightModeDisplay.labelText(displayText)
        property string cornerBadgeText: toolStripAction && typeof toolStripAction.cornerBadgeText !== "undefined" ? toolStripAction.cornerBadgeText : ""
        property string inlineBadgeText:  cornerBadgeText !== "" ? "" : flightModeDisplay.badgeText(displayText)
        property bool showOperationSeparator: bottomStrip.showOperationSeparatorBefore(actionIndex, toolStripAction)
        property bool statusAction: toolStripAction && typeof toolStripAction.statusAction !== "undefined" && toolStripAction.statusAction

        visible:                bottomStrip.actionVisible(toolStripAction)
        Layout.preferredWidth:  visible ? _toolButtonWidth : 0
        Layout.minimumWidth:    visible ? _toolButtonWidth : 0
        Layout.maximumWidth:    visible ? _toolButtonWidth : 0
        Layout.fillHeight:      visible
        Layout.leftMargin:      visible && showOperationSeparator ? bottomStrip._operationSeparatorExtraGap : 0
        radius:                 Math.round(ScreenTools.defaultFontPixelWidth * 0.30)
        color:                  buttonMouse.pressed ? Qt.rgba(0.135, 0.140, 0.150, 0.44) :
                                    (statusAction ? ((available && buttonMouse.containsMouse) ? Qt.rgba(1, 1, 1, 0.040) : "transparent") :
                                     (checked ? Qt.rgba(0.82, 0.90, 0.95, 0.095) :
                                      ((available && buttonMouse.containsMouse) ? Qt.rgba(1, 1, 1, 0.060) : "transparent")))
        border.color:           statusAction ? (checked ? qgcPal.primaryButton : Qt.rgba(0.82, 0.90, 0.95, buttonMouse.containsMouse ? 0.30 : 0.20)) :
                                    (checked ? qgcPal.primaryButton :
                                     ((available && buttonMouse.containsMouse) ? Qt.rgba(0.82, 0.90, 0.95, 0.16) : "transparent"))
        border.width:           checked || statusAction || (available && buttonMouse.containsMouse) ? 1 : 0
        opacity:                available ? 1.0 : 0.48

        property color _contentColor: checked || statusAction ? qgcPal.text : qgcPal.buttonText
        property color _contentColorSecondary: qgcPal.colorGreen

        Rectangle {
            id:                 operationSeparator
            anchors.left:       parent.left
            anchors.leftMargin: -Math.round((bottomStrip._rowSpacing + bottomStrip._operationSeparatorExtraGap + width) / 2)
            anchors.verticalCenter: parent.verticalCenter
            width:              1
            height:             parent.height * 0.58
            radius:             width / 2
            color:              Qt.rgba(0.82, 0.90, 0.95, 0.22)
            visible:            actionButton.showOperationSeparator
        }

        onCheckedChanged: {
            if (toolStripAction && toolStripAction.checked !== checked) {
                toolStripAction.checked = checked
            }
        }

        ColumnLayout {
            anchors.centerIn:   parent
            width:              parent.width - ScreenTools.defaultFontPixelWidth * 0.48
            spacing:            0

            Image {
                Layout.alignment:   Qt.AlignHCenter
                source:             actionButton.imageSource
                visible:            source !== "" && actionButton.toolStripAction && actionButton.toolStripAction.fullColorIcon
                width:              bottomStrip._toolIconSize
                height:             width
                sourceSize.width:   width
                sourceSize.height:  height
                fillMode:           Image.PreserveAspectFit
                smooth:             true
                mipmap:             true
            }

            QGCColoredImage {
                id:                 actionIcon
                Layout.alignment:   Qt.AlignHCenter
                source:             actionButton.imageSource
                visible:            source !== "" && (!actionButton.toolStripAction || !actionButton.toolStripAction.fullColorIcon)
                color:              actionButton._contentColor
                width:              bottomStrip._toolIconSize
                height:             width
                sourceSize.width:   width
                sourceSize.height:  height
                fillMode:           Image.PreserveAspectFit

                QGCColoredImage {
                    anchors.centerIn:   parent
                    source:             actionButton.toolStripAction ? actionButton.toolStripAction.alternateIconSource : ""
                    visible:            source !== "" && actionButton.toolStripAction && actionButton.toolStripAction.biColorIcon
                    color:              actionButton._contentColorSecondary
                    width:              parent.width
                    height:             parent.height
                    sourceSize.width:   width
                    sourceSize.height:  height
                    fillMode:           Image.PreserveAspectFit
                }
            }

            Item {
                id:                         actionTextHost
                Layout.alignment:           Qt.AlignHCenter
                Layout.fillWidth:           true
                Layout.preferredHeight:     Math.max(actionTextLabel.implicitHeight,
                                                     actionInlineBadge.visible ? actionInlineBadge.height : 0)

                RowLayout {
                    id:                     actionTextRow
                    anchors.centerIn:       parent
                    spacing:                actionInlineBadge.visible ? ScreenTools.defaultFontPixelWidth * 0.22 : 0

                    QGCLabel {
                        id:                     actionTextLabel
                        Layout.preferredWidth:  Math.max(0, Math.min(implicitWidth,
                                                                     actionTextHost.width -
                                                                     (actionInlineBadge.visible ? actionInlineBadge.width + actionTextRow.spacing : 0)))
                        Layout.maximumWidth:    Layout.preferredWidth
                        text:                   actionButton.displayLabelText
                        color:                  actionButton._contentColor
                        font.bold:              actionButton.checked || actionButton.imageSource === ""
                        font.pointSize:         ScreenTools.controlFontPointSize
                        horizontalAlignment:    Text.AlignHCenter
                        verticalAlignment:      Text.AlignVCenter
                        fontSizeMode:           Text.HorizontalFit
                        minimumPointSize:       ScreenTools.captionFontPointSize
                        elide:                  Text.ElideRight
                        maximumLineCount:       1
                    }

                    Rectangle {
                        id:                 actionInlineBadge
                        Layout.alignment:   Qt.AlignVCenter
                        Layout.preferredWidth: actionInlineBadgeLabel.implicitWidth + ScreenTools.defaultFontPixelWidth * 0.42
                        Layout.preferredHeight: Math.max(ScreenTools.defaultFontPixelHeight * 0.66,
                                                         actionInlineBadgeLabel.implicitHeight + ScreenTools.defaultFontPixelHeight * 0.04)
                        radius:             Math.round(height * 0.28)
                        color:              Qt.rgba(0.82, 0.88, 0.94, actionButton.checked ? 0.16 : 0.10)
                        border.color:       Qt.rgba(0.82, 0.88, 0.94, actionButton.checked ? 0.28 : 0.18)
                        border.width:       1
                        visible:            actionButton.inlineBadgeText !== ""

                        QGCLabel {
                            id:                     actionInlineBadgeLabel
                            anchors.centerIn:       parent
                            text:                   actionButton.inlineBadgeText
                            color:                  qgcPal.buttonText
                            font.bold:              true
                            font.pointSize:         Math.max(7, ScreenTools.captionFontPointSize - 1)
                            horizontalAlignment:    Text.AlignHCenter
                            verticalAlignment:      Text.AlignVCenter
                            maximumLineCount:       1
                        }
                    }
                }
            }
        }

        Rectangle {
            id:                 actionCornerBadge
            anchors.top:        parent.top
            anchors.right:      parent.right
            anchors.topMargin:  ScreenTools.defaultFontPixelHeight * 0.20
            anchors.rightMargin:ScreenTools.defaultFontPixelWidth * 0.20
            width:              actionCornerBadgeLabel.implicitWidth + ScreenTools.defaultFontPixelWidth * 0.42
            height:             Math.max(ScreenTools.defaultFontPixelHeight * 0.66,
                                         actionCornerBadgeLabel.implicitHeight + ScreenTools.defaultFontPixelHeight * 0.04)
            radius:             Math.round(height * 0.28)
            color:              Qt.rgba(0.82, 0.88, 0.94, actionButton.checked ? 0.16 : 0.10)
            border.color:       Qt.rgba(0.82, 0.88, 0.94, actionButton.checked ? 0.28 : 0.18)
            border.width:       1
            visible:            actionButton.cornerBadgeText !== ""
            z:                  3

            QGCLabel {
                id:                     actionCornerBadgeLabel
                anchors.centerIn:       parent
                text:                   actionButton.cornerBadgeText
                color:                  qgcPal.buttonText
                font.bold:              true
                font.pointSize:         Math.max(7, ScreenTools.captionFontPointSize - 1)
                horizontalAlignment:    Text.AlignHCenter
                verticalAlignment:      Text.AlignVCenter
                maximumLineCount:       1
            }
        }

        QGCMouseArea {
            id:             buttonMouse
            anchors.fill:   parent
            enabled:        actionButton.available
            hoverEnabled:   !ScreenTools.isMobile
            onClicked: {
                if (!actionButton.toolStripAction) {
                    return
                }
                if (mainWindow.allowViewSwitch()) {
                    dropPanel.hide()
                    if (!actionButton.toolStripAction.dropPanelComponent) {
                        actionButton.toolStripAction.triggered(actionButton)
                    } else {
                        actionButton.checked = true
                        bottomStrip.clearOtherActionChecks(actionButton.actionIndex)
                        var panelEdgeTopPoint = actionButton.mapToItem(bottomStrip, actionButton.width / 2, 0)
                        dropPanel.show(panelEdgeTopPoint, actionButton.toolStripAction.dropPanelComponent, actionButton)
                    }
                } else if (actionButton.checkable) {
                    actionButton.checked = !actionButton.checked
                }
            }
        }
    }

    RowLayout {
        anchors.fill:       parent
        anchors.margins:    _stripMargin
        spacing:            _rowSpacing

        QGCFlickable {
            id:                     actionFlick
            property real _overflowEpsilon: ScreenTools.defaultFontPixelWidth * 0.25
            property bool _rawHorizontalOverflow: contentWidth > width + _overflowEpsilon
            property bool _stableHorizontalOverflow: false

            Layout.fillWidth:       true
            Layout.fillHeight:      true
            Layout.preferredWidth:  bottomStrip._toolActionsWidth
            Layout.minimumWidth:    0
            contentWidth:           bottomStrip._toolActionsWidth
            contentHeight:          height
            flickableDirection:     _stableHorizontalOverflow ? Flickable.HorizontalFlick : Flickable.VerticalFlick
            boundsBehavior:         Flickable.StopAtBounds
            interactive:            _stableHorizontalOverflow
            clip:                   true
            indicatorColor:         qgcPal.buttonText

            function _refreshHorizontalOverflow() {
                if (_rawHorizontalOverflow) {
                    overflowSettleTimer.restart()
                } else {
                    overflowSettleTimer.stop()
                    _stableHorizontalOverflow = false
                    contentX = 0
                }
            }

            on_RawHorizontalOverflowChanged: _refreshHorizontalOverflow()
            Component.onCompleted:           _refreshHorizontalOverflow()

            Timer {
                id:         overflowSettleTimer
                interval:   120
                repeat:     false
                onTriggered: actionFlick._stableHorizontalOverflow = actionFlick._rawHorizontalOverflow
            }

            on_StableHorizontalOverflowChanged: {
                if (!_stableHorizontalOverflow) {
                    contentX = 0
                }
            }

            RowLayout {
                id:                 actionRow
                anchors.top:        parent.top
                anchors.bottom:     parent.bottom
                spacing:            _rowSpacing

                Repeater {
                    id:     actionRepeater
                    model:  flyActionList.model

                    ActionButton {
                        toolStripAction: modelData
                        actionIndex:     index
                    }
                }
            }
        }

    }
}
