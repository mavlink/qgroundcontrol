import QtQuick
import QtQuick.Dialogs

// On Qt 5.9 android versions there is the following bug: https://bugreports.qt.io/browse/QTBUG-61424
// This prevents FileDialog from being used. So we have a temp hack workaround for it which just no-ops
// the FileDialog fallback mechanism on android 5.9 builds.

Item {
    property var folder
    property var nameFilters
    property var title
    property var selectMultiple
    property var selectFolder

    signal accepted
    signal rejected
}
