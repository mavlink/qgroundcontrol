import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

/// File or folder browser control for a string-valued Fact.
/// Shows a label, the current path (or a default placeholder), and a
/// "Browse" button that opens a native file dialog.
///
/// Properties:
///   fact         - The string Fact whose rawValue stores the path (required).
///   label        - Display label (defaults to fact.shortDescription).
///   dialogTitle  - Title for the file dialog (defaults to label).
///   selectFolder - true to browse folders, false for files (default true).
///   defaultText  - Placeholder shown when the Fact value is empty.
RowLayout {
    id:      root

    property string label:          fact.shortDescription
    property Fact   fact
    property string dialogTitle:    label
    property bool   selectFolder:   true
    property string defaultText:    qsTr("<not set>")

    spacing: ScreenTools.defaultFontPixelWidth * 2

    ColumnLayout {
        Layout.fillWidth:    true
        Layout.minimumWidth: implicitWidth
        spacing:             0

        QGCLabel {
            Layout.fillWidth:   true
            text:               root.label
        }

        QGCLabel {
            Layout.fillWidth:   true
            font.pointSize:     ScreenTools.smallFontPointSize
            text:               root.fact.rawValue === "" ? root.defaultText : root.fact.value
            elide:              Text.ElideMiddle
        }
    }

    QGCButton {
        text:       qsTr("Browse")
        onClicked:  _browseDialog.openForLoad()

        QGCFileDialog {
            id:                 _browseDialog
            title:              root.dialogTitle
            folder:             root.fact.rawValue
            selectFolder:       root.selectFolder
            onAcceptedForLoad:  (file) => root.fact.rawValue = file
        }
    }
}
