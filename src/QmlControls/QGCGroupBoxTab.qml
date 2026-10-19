import QtQuick
import QtQuick.Layouts

/// A tab page within a QGCGroupBox. The group box shows a header button for each visible tab and only shows the
/// content of the current tab.
ColumnLayout {
    id: tab
    Layout.fillWidth: true
    Layout.maximumWidth: Number.POSITIVE_INFINITY
    Layout.minimumWidth: 0
    // Child layouts fill height by default, which would take height from content shared between tabs
    Layout.fillHeight: false
    spacing: tab.parent ? tab.parent.spacing : 0
    visible: tabVisible && isCurrentTab

    property string title
    property string iconSource
    property bool tabVisible: true ///< false hides both the tab's header button and its content
    property Item badge: null ///< Shown in the header button after the title
    property string buttonObjectName ///< objectName for the header button

    readonly property bool isCurrentTab: _groupBox !== null && _groupBox._currentTab === tab
    readonly property bool _isQGCGroupBoxTab: true
    property var _groupBox: null

    onTabVisibleChanged: {
        if (_groupBox) {
            _groupBox._validateCurrentIndex()
        }
    }
}
