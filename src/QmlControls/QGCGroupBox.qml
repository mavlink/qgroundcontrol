pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

import "QGCGroupBox.js" as GroupBox

/// Bordered content area with rounded header buttons sitting on the top border.
///     - Group box: set heading to show a single non-interactive header button.
///     - Tabs: add QGCGroupBoxTab children. Each visible tab gets a header button and only the current tab's content is shown.
///       Children which are not tabs are always shown. heading is ignored when there are tabs.
Item {
    id: control
    // Matches the previous ColumnLayout based SettingsGroupLayout, which filled the width of a parent layout
    Layout.fillWidth: true
    implicitWidth: Math.max(_contentColumn.implicitWidth + _contentInset * 2, _headerImplicitWidth + _headerInset * 2)
    implicitHeight: _contentColumn.y + _contentColumn.implicitHeight + _contentInset

    default property alias contentData: _contentLayout.data

    property alias contentSpacing: _contentLayout.spacing

    property string heading
    property string headingDescription
    property real headingPointSize: ScreenTools.defaultFontPointSize
    property color outerBorderColor: QGroundControl.globalPalette.groupBorder
    property bool showBorder: true
    property bool showDividers: true
    property bool wrapTabs: false ///< Wrap tab buttons onto multiple rows instead of eliding them to fit on one row
    property int currentIndex: 0 ///< Index of the current QGCGroupBoxTab, -1 if no tab is visible

    /// Distance from the top of the group box to the content, for sizing content to an available height
    readonly property real contentTopInset: _contentColumn.y + _contentLayout.y
    /// Distance from the left, right and bottom edges of the group box to the content
    readonly property real contentSideInset: _contentInset

    property real _margins: ScreenTools.defaultFontPixelHeight / 2
    property real _contentInset: showBorder ? _margins : 0
    property real _headerInset: showBorder ? _margins : 0
    property real _headerButtonSpacing: ScreenTools.defaultFontPixelWidth / 2
    property real _headerButtonHeight: Math.round(_headerButtonFontMetrics.height + ScreenTools.defaultFontPixelHeight * 0.4)
    property var _tabs: []
    property Item _currentTab: currentIndex >= 0 && currentIndex < _tabs.length ? _tabs[currentIndex] : null
    property var _shownHeaderButtonImplicitWidths: {
        let widths = []
        for (const headerButton of _header.children) {
            if (headerButton.shown) {
                widths.push(headerButton.implicitWidth)
            }
        }
        return widths
    }
    property int _shownHeaderButtonCount: {
        let count = 0
        for (const headerButton of _header.children) {
            if (headerButton.shown) {
                count++
            }
        }
        return count
    }
    property real _headerImplicitWidth: _shownHeaderButtonImplicitWidths.reduce((sum, width) => sum + width, 0) +
                                        _headerButtonSpacing * Math.max(_shownHeaderButtonImplicitWidths.length - 1, 0)
    property real _maxHeaderButtonWidth: wrapTabs ? _header.width : GroupBox.maxHeaderButtonWidth(_shownHeaderButtonImplicitWidths, _header.width, _headerButtonSpacing)
    property Item _dividerLayout: GroupBox.dividerLayout(_tabs.length > 0, _currentTab, _contentLayout)

    // Sorted top to bottom for divider visibility checks
    property var _ySortedChildren: {
        if (!showDividers || !_dividerLayout) {
            return []
        }
        let children = []
        for (const child of _dividerLayout.children) {
            children.push(child)
        }
        children.sort((a, b) => a.y - b.y)
        return children
    }

    // Right edge of the first row of header buttons, which is where the top border line resumes
    property real _headerFirstRowRight: {
        let right = 0
        for (const headerButton of _header.children) {
            if (headerButton.shown && headerButton.y === 0) {
                right = Math.max(right, headerButton.x + headerButton.width)
            }
        }
        return right
    }
    property color _dividerColor: QGroundControl.globalPalette.groupBorder
    property var _dividerItemStates: _ySortedChildren.map(function(item) {
        return item ? { visible: item.visible, isRepeater: _isRepeater(item) } : null
    })

    function _updateTabs() {
        let tabs = []
        for (const child of _contentLayout.children) {
            if (child._isQGCGroupBoxTab === true) {
                child._groupBox = control
                tabs.push(child)
            }
        }
        // Reassigning the model rebuilds every header button, so only do it when the tabs actually changed
        if (!GroupBox.sameItems(tabs, _tabs)) {
            _tabs = tabs
        }
        _validateCurrentIndex()
    }

    function _validateCurrentIndex() {
        if (_tabs.length === 0) {
            return
        }
        const index = GroupBox.validTabIndex(currentIndex, _tabs.map(tab => tab.tabVisible))
        if (index !== currentIndex) {
            currentIndex = index
        }
    }

    function _isRepeater(item) {
        return item && item.toString().startsWith("QQuickRepeater")
    }

    onCurrentIndexChanged: _validateCurrentIndex()

    component HeaderButton: AbstractButton {
        id: headerButton
        width: groupBox ? Math.min(implicitWidth, groupBox._maxHeaderButtonWidth) : implicitWidth
        implicitWidth: leftPadding + rightPadding + buttonContent.implicitWidth
        implicitHeight: groupBox ? groupBox._headerButtonHeight : 0
        leftPadding: ScreenTools.defaultFontPixelWidth * 1.5
        rightPadding: leftPadding
        hoverEnabled: _isTabChoice && !ScreenTools.isMobile
        focusPolicy: _isTabChoice ? Qt.ClickFocus : Qt.NoFocus
        visible: shown

        property Item groupBox: null
        property bool shown: true
        property bool interactive: true
        property string iconSource
        property Item badge: null

        // A lone heading or tab is a title rather than a choice, so it is drawn as a quiet outlined label
        property bool _isTabChoice: interactive && groupBox !== null && groupBox._shownHeaderButtonCount > 1
        // Only the selected tab is filled so a group box has at most one strong color block. Disabled buttons are
        // never filled since the disabled highlight colors have too little contrast, and solid fills would not match
        // translucent backgrounds. A disabled selected tab is marked with a heavier border instead.
        property bool _filled: _isTabChoice && checked && enabled
        property bool _disabledSelected: _isTabChoice && checked && !enabled
        property color _textColor: _filled ? buttonPal.buttonHighlightText : buttonPal.text
        property Item _adoptedBadge: null

        // Moves the badge into the header button, releasing a previously adopted badge which has been replaced
        function _adoptBadge() {
            if (_adoptedBadge && _adoptedBadge !== badge) {
                _adoptedBadge.parent = null
            }
            if (badge) {
                badge.parent = badgeHost
            }
            _adoptedBadge = badge
        }

        QGCPalette { id: buttonPal; colorGroupEnabled: headerButton.enabled }

        background: Rectangle {
            radius: ScreenTools.defaultBorderRadius
            color: headerButton._filled ? buttonPal.buttonHighlight : "transparent"
            border.width: headerButton._disabledSelected ? 2 : 1
            border.color: {
                if (headerButton._filled) {
                    return buttonPal.buttonHighlight
                }
                if (headerButton._disabledSelected) {
                    return buttonPal.text
                }
                return headerButton.groupBox ? headerButton.groupBox.outerBorderColor : buttonPal.groupBorder
            }

            Rectangle {
                anchors.fill: parent
                radius: parent.radius
                color: buttonPal.buttonHighlight
                opacity: headerButton.enabled && headerButton._isTabChoice && !headerButton.checked && (headerButton.hovered || headerButton.pressed) ? 0.2 : 0
            }
        }

        contentItem: RowLayout {
            id: buttonContent
            spacing: ScreenTools.defaultFontPixelWidth / 2

            QGCColoredImage {
                Layout.alignment: Qt.AlignHCenter
                height: ScreenTools.defaultFontPixelHeight
                width: height
                sourceSize.height: height
                fillMode: Image.PreserveAspectFit
                source: headerButton.iconSource
                color: headerButton._textColor
                visible: headerButton.iconSource !== ""
            }

            QGCLabel {
                Layout.fillWidth: true
                text: headerButton.text
                color: headerButton._textColor
                font.pointSize: headerButton.groupBox ? headerButton.groupBox.headingPointSize : ScreenTools.defaultFontPointSize
                font.bold: !headerButton._isTabChoice
                elide: Text.ElideRight
                visible: text !== ""
            }

            Item {
                id: badgeHost
                implicitWidth: headerButton.badge && headerButton.badge.visible ? headerButton.badge.width : 0
                implicitHeight: headerButton.badge ? headerButton.badge.height : 0
                visible: headerButton.badge !== null
            }
        }

        onBadgeChanged: _adoptBadge()

        // Change handlers do not run for the initial value, so a badge set at creation is adopted here
        Component.onCompleted: _adoptBadge()
    }

    FontMetrics {
        id: _headerButtonFontMetrics
        font.family: ScreenTools.normalFontFamily
        font.pointSize: control.headingPointSize
    }

    // The border is drawn in pieces so the top edge stops either side of the header buttons, which lets the header
    // buttons stay transparent over whatever background the group box is on.
    Item {
        id: outline
        y: _header.visible ? control._headerButtonHeight / 2 : 0
        width: control.width
        height: control.height - y
        visible: control.showBorder

        property real _radius: control._margins
        property var _topSegments: GroupBox.topBorderSegments(width, _radius,
                                                              _header.visible ? _header.x - control._headerButtonSpacing : _radius,
                                                              _header.visible ? _header.x + control._headerFirstRowRight + control._headerButtonSpacing : _radius)

        // Sides and bottom, then the top left and top right corners
        Repeater {
            model: [
                Qt.rect(0, outline._radius, outline.width, Math.max(outline.height - outline._radius, 0)),
                Qt.rect(0, 0, outline._radius, outline._radius),
                Qt.rect(outline.width - outline._radius, 0, outline._radius, outline._radius)
            ]

            Item {
                id: outlinePiece
                x: modelData.x
                y: modelData.y
                width: modelData.width
                height: modelData.height
                clip: true

                required property rect modelData

                Rectangle {
                    x: -outlinePiece.x
                    y: -outlinePiece.y
                    width: outline.width
                    height: outline.height
                    radius: outline._radius
                    color: "transparent"
                    border.color: control.outerBorderColor
                    border.width: 1
                }
            }
        }

        Repeater {
            model: outline._topSegments

            Rectangle {
                x: modelData.x
                width: modelData.width
                height: 1
                color: control.outerBorderColor

                required property var modelData
            }
        }
    }

    Flow {
        id: _header
        x: control._headerInset
        width: control.width - control._headerInset * 2
        spacing: control._headerButtonSpacing
        visible: control._shownHeaderButtonImplicitWidths.length > 0

        HeaderButton {
            groupBox: control
            text: control.heading
            shown: control._tabs.length === 0 && control.heading !== ""
            checked: true
            interactive: false
        }

        Repeater {
            model: control._tabs

            HeaderButton {
                required property var modelData
                required property int index

                groupBox: control

                objectName: modelData.buttonObjectName
                text: modelData.title
                iconSource: modelData.iconSource
                badge: modelData.badge
                shown: modelData.tabVisible
                checked: index === control.currentIndex
                onClicked: control.currentIndex = index
            }
        }
    }

    ColumnLayout {
        id: _contentColumn
        x: control._contentInset
        y: _header.visible ? _header.height + control._margins / 2 : control._contentInset
        width: control.width - control._contentInset * 2
        height: control.height - y - control._contentInset
        spacing: control._margins / 2

        QGCLabel {
            Layout.fillWidth: true
            text: control.headingDescription
            wrapMode: Text.WordWrap
            font.pointSize: ScreenTools.smallFontPointSize
            visible: text !== ""
        }

        ColumnLayout {
            id: _contentLayout
            Layout.fillWidth: true
            // Layouts cap their maximum width to their implicit width when no child fills the width
            Layout.maximumWidth: Number.POSITIVE_INFINITY
            // Keep content within the border even when a child's minimum width is wider than the group box
            Layout.minimumWidth: 0
            Layout.fillHeight: true
            spacing: control._margins * (control.showDividers ? 2 : 1)

            onChildrenChanged: control._updateTabs()
        }
    }

    Item {
        id: dividerOverlay
        x: _contentColumn.x + (control._dividerLayout && control._dividerLayout !== _contentLayout ? control._dividerLayout.x : 0)
        y: _contentColumn.y + _contentLayout.y + (control._dividerLayout && control._dividerLayout !== _contentLayout ? control._dividerLayout.y : 0)
        width: control._dividerLayout ? control._dividerLayout.width : _contentLayout.width

        Repeater {
            model: control._ySortedChildren.length

            Rectangle {
                id: divider
                y: _contentItem ? _contentItem.y + _contentItem.height + control._dividerLayout.spacing / 2 : 0
                width: dividerOverlay.width
                height: 1
                color: control._dividerColor
                visible: GroupBox.dividerVisibleAfter(divider.index, control._dividerItemStates)

                required property int index
                property var _contentItem: index < control._ySortedChildren.length ? control._ySortedChildren[index] : undefined
            }
        }
    }

    Component.onCompleted: _updateTabs()
}
