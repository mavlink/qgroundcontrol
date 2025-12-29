import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

Item {
    id: root
    width: parent ? parent.width : ScreenTools.defaultFontPixelWidth * 60
    implicitHeight: formColumn.implicitHeight

    ColumnLayout {
        id: formColumn
        anchors.horizontalCenter: parent.horizontalCenter
        width: Math.min(parent.width, ScreenTools.defaultFontPixelWidth * 60)
        anchors.margins: ScreenTools.defaultFontPixelHeight * 0.4
        spacing: ScreenTools.defaultFontPixelHeight * (parent && parent.width < ScreenTools.defaultFontPixelWidth * 50 ? 0.6 : 0.8)
        property real fieldHeight: ScreenTools.defaultFontPixelHeight * (parent && parent.width < ScreenTools.defaultFontPixelWidth * 50 ? 1.8 : 2.0)

        QGCLabel { Layout.fillWidth: true; wrapMode: Text.WrapAnywhere; text: qsTr("Email") }
        QGCTextField {
            Layout.fillWidth: true
            Layout.preferredHeight: formColumn.fieldHeight
            text: mainWindow.currentUserEmail
            enabled: false
        }

        QGCLabel { Layout.fillWidth: true; wrapMode: Text.WrapAnywhere; text: qsTr("Current Password") }
        QGCTextField {
            id: curPass
            Layout.fillWidth: true
            Layout.preferredHeight: formColumn.fieldHeight
            echoMode: TextInput.Password
            placeholderText: qsTr("Enter current password")
        }

        QGCLabel { Layout.fillWidth: true; wrapMode: Text.WrapAnywhere; text: qsTr("New Password") }
        QGCTextField {
            id: newPass
            Layout.fillWidth: true
            Layout.preferredHeight: formColumn.fieldHeight
            echoMode: TextInput.Password
            placeholderText: qsTr("Enter new password")
        }

        QGCLabel { Layout.fillWidth: true; wrapMode: Text.WrapAnywhere; text: qsTr("Confirm Password") }
        QGCTextField {
            id: confPass
            Layout.fillWidth: true
            Layout.preferredHeight: formColumn.fieldHeight
            echoMode: TextInput.Password
            placeholderText: qsTr("Re-enter new password")
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: ScreenTools.defaultFontPixelWidth

            QGCButton {
                text: qsTr("Apply")
                onClicked: {
                    var email = mainWindow.currentUserEmail
                    var cur = curPass.text
                    var np = newPass.text
                    var cp = confPass.text
                    if (!mainWindow.loggedIn || email.length === 0) { changePassError.text = qsTr("Not logged in"); changePassError.visible = true; return }
                    var d = mainWindow.db()
                    var ok = false
                    var salt = ""
                    d.transaction(function(tx) {
                        var rs = tx.executeSql('SELECT password_hash, salt FROM users WHERE email = ?', [email])
                        if (rs.rows.length === 1) {
                            var row = rs.rows.item(0)
                            var h = mainWindow.hashPassword(email, cur, row.salt)
                            ok = (h === row.password_hash)
                            salt = row.salt
                        }
                    })
                    if (!ok) { changePassError.text = qsTr("Incorrect current password"); changePassError.visible = true; return }
                    if (np.length < 8) { changePassError.text = qsTr("Password must be at least 8 characters"); changePassError.visible = true; return }
                    if (np !== cp) { changePassError.text = qsTr("Passwords do not match"); changePassError.visible = true; return }
                    var newSalt = String(new Date().getTime())
                    var newHash = mainWindow.hashPassword(email, np, newSalt)
                    var updated = false
                    d.transaction(function(tx) {
                        tx.executeSql('UPDATE users SET password_hash = ?, salt = ? WHERE email = ?', [newHash, newSalt, email])
                        updated = true
                    })
                    if (!updated) { changePassError.text = qsTr("Update failed"); changePassError.visible = true; return }
                    changePassError.visible = false
                }
            }
            QGCButton {
                text: qsTr("Cancel")
                onClicked: {
                    curPass.text = ""
                    newPass.text = ""
                    confPass.text = ""
                    changePassError.visible = false
                    if (root.parent && root.parent.active !== undefined) {
                        root.parent.active = false
                    }
                }
            }
        }

        QGCLabel {
            id: changePassError
            Layout.fillWidth: true
            color: QGroundControl.globalPalette.alertText
            visible: false
        }
    }
}
