import QtQuick
import QtQuick.Controls

MenuItem {
    // MenuItem doesn't support !visible so we have to hack it in
    height: visible ? implicitHeight : 0
}
