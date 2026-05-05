pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

ListView {
    id: root

    readonly property real checkboxColumnWidth: ScreenTools.defaultFontPixelHeight * 2
    readonly property real columnSpacing: ScreenTools.defaultFontPixelWidth
    required property OnboardLogController controller
    readonly property real dateColumnWidth: ScreenTools.defaultFontPixelWidth * 26
    readonly property real idColumnWidth: ScreenTools.defaultFontPixelWidth * 8
    readonly property real minimumContentWidth: checkboxColumnWidth + idColumnWidth + dateColumnWidth + sizeColumnWidth + statusColumnWidth + columnSpacing * 4
    readonly property real sizeColumnWidth: ScreenTools.defaultFontPixelWidth * 12
    readonly property real statusColumnWidth: ScreenTools.defaultFontPixelWidth * 18
    property Item statusToolTipOwner: null

    function hideStatusToolTip(owner) {
        if (statusToolTipOwner === owner) {
            statusToolTip.close();
            statusToolTipOwner = null;
        }
    }

    function showStatusToolTip(owner, immediate) {
        if (!owner || !owner.truncated) {
            return;
        }

        statusToolTipOwner = owner;
        statusToolTip.delay = immediate ? 0 : 500;
        statusToolTip.timeout = immediate ? 3000 : -1;
        const position = owner.mapToItem(root, 0, owner.height);
        statusToolTip.x = Math.max(0, Math.min(position.x, root.width - statusToolTip.width));
        statusToolTip.y = Math.max(0, Math.min(position.y, root.height - statusToolTip.height));
        statusToolTip.open();
    }

    boundsBehavior: Flickable.StopAtBounds
    cacheBuffer: height
    clip: true
    contentWidth: Math.max(width, minimumContentWidth)
    headerPositioning: ListView.OverlayHeader
    model: controller.model
    reuseItems: true
    spacing: 0

    onStatusToolTipOwnerChanged: {
        if (!statusToolTipOwner) {
            statusToolTip.close();
        }
    }

    onMovementStarted: {
        statusToolTip.close();
        statusToolTipOwner = null;
    }

    ScrollBar.horizontal: ScrollBar {
    }
    ScrollBar.vertical: ScrollBar {
    }
    delegate: Rectangle {
        id: logRow

        required property int index
        required property OnboardLogEntry object

        color: index % 2 === 0 ? qgcPal.window : qgcPal.windowShade
        height: Math.max(rowLayout.implicitHeight, ScreenTools.defaultFontPixelHeight * 1.5)
        width: root.contentWidth

        ListView.onPooled: {
            root.hideStatusToolTip(statusLabel);
            visible = false;
        }
        ListView.onReused: visible = true

        RowLayout {
            id: rowLayout

            anchors.fill: parent
            spacing: root.columnSpacing

            QGCCheckBox {
                checked: logRow.object.selected
                enabled: logRow.object.received && !root.controller.busy
                focusPolicy: Qt.StrongFocus

                Layout.alignment: Qt.AlignVCenter
                Layout.preferredWidth: root.checkboxColumnWidth

                Accessible.name: qsTr("Select onboard log %1").arg(logRow.object.id)
                Accessible.description: checked ? qsTr("Selected for download") : qsTr("Not selected for download")

                onClicked: logRow.object.selected = checked
            }

            QGCLabel {
                elide: Text.ElideRight
                text: logRow.object.id
                textFormat: Text.PlainText

                Layout.preferredWidth: root.idColumnWidth
            }

            QGCLabel {
                elide: Text.ElideRight
                text: {
                    if (!logRow.object.received) {
                        return "";
                    }
                    if (Number.isNaN(logRow.object.time.getTime()) || logRow.object.time.getUTCFullYear() < 2010) {
                        return qsTr("Date Unknown");
                    }
                    return logRow.object.time.toLocaleString(undefined);
                }
                textFormat: Text.PlainText

                Layout.preferredWidth: root.dateColumnWidth
            }

            QGCLabel {
                elide: Text.ElideRight
                text: logRow.object.sizeStr
                textFormat: Text.PlainText

                Layout.preferredWidth: root.sizeColumnWidth
            }

            QGCLabel {
                id: statusLabel

                activeFocusOnTab: truncated
                elide: Text.ElideRight
                text: logRow.object.errorMessage.length > 0
                      ? qsTr("%1: %2").arg(logRow.object.status).arg(logRow.object.errorMessage)
                      : logRow.object.status
                textFormat: Text.PlainText

                Layout.fillWidth: true
                Layout.minimumWidth: root.statusColumnWidth

                Accessible.name: statusLabel.text

                Component.onDestruction: root.hideStatusToolTip(statusLabel)

                Keys.onPressed: event => {
                    if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter || event.key === Qt.Key_Space) {
                        root.showStatusToolTip(statusLabel, true);
                        event.accepted = true;
                    }
                }

                onActiveFocusChanged: {
                    if (activeFocus) {
                        root.showStatusToolTip(statusLabel, true);
                    } else {
                        root.hideStatusToolTip(statusLabel);
                    }
                }
                onTruncatedChanged: {
                    if (!truncated) {
                        root.hideStatusToolTip(statusLabel);
                    }
                }

                HoverHandler {
                    onHoveredChanged: {
                        if (hovered) {
                            root.showStatusToolTip(statusLabel, false);
                        } else {
                            root.hideStatusToolTip(statusLabel);
                        }
                    }
                }

                TapHandler {
                    onTapped: root.showStatusToolTip(statusLabel, true)
                }
            }
        }
    }
    header: Rectangle {
        color: qgcPal.window
        height: headerRow.implicitHeight
        width: root.contentWidth
        z: 1

        RowLayout {
            id: headerRow

            anchors.fill: parent
            spacing: root.columnSpacing

            Item {
                Layout.preferredWidth: root.checkboxColumnWidth
            }

            QGCLabel {
                text: qsTr("Id")
                textFormat: Text.PlainText

                Layout.preferredWidth: root.idColumnWidth
            }

            QGCLabel {
                text: qsTr("Date")
                textFormat: Text.PlainText

                Layout.preferredWidth: root.dateColumnWidth
            }

            QGCLabel {
                text: qsTr("Size")
                textFormat: Text.PlainText

                Layout.preferredWidth: root.sizeColumnWidth
            }

            QGCLabel {
                text: qsTr("Status")
                textFormat: Text.PlainText

                Layout.fillWidth: true
                Layout.minimumWidth: root.statusColumnWidth
            }
        }
    }

    QGCPalette {
        id: qgcPal

        colorGroupEnabled: root.enabled
    }

    ToolTip {
        id: statusToolTip

        text: root.statusToolTipOwner ? root.statusToolTipOwner.text : ""
        width: Math.min(implicitWidth, root.width)
    }
}
