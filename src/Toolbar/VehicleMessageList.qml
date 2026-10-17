import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

TextArea {
    id:                     messageText
    Layout.preferredWidth:  ScreenTools.defaultFontPixelWidth * 50
    height:                 contentHeight
    readOnly:               true
    textFormat:             TextEdit.RichText
    color:                  qgcPal.text
    placeholderText:        qsTr("No Messages")
    placeholderTextColor:   qgcPal.text
    padding:                0
    wrapMode:               TextEdit.Wrap

    property bool noMessages: messageText.length === 0

    property var _fact: null

    function formatMessage(message) {
        message = message.replace(new RegExp("<#E>", "g"), "color: " + qgcPal.errorText + "; font: " + (ScreenTools.defaultFontPointSize.toFixed(0) - 1) + "pt monospace;");
        message = message.replace(new RegExp("<#I>", "g"), "color: " + qgcPal.warningText + "; font: " + (ScreenTools.defaultFontPointSize.toFixed(0) - 1) + "pt monospace;");
        message = message.replace(new RegExp("<#N>", "g"), "color: " + qgcPal.text + "; font: " + (ScreenTools.defaultFontPointSize.toFixed(0) - 1) + "pt monospace;");
        return message;
    }

    Component.onCompleted: {
        messageText.text = formatMessage(_activeVehicle.formattedMessages)
    }

    Connections {
        target: _activeVehicle
        function onNewFormattedMessage(formattedMessage) { messageText.insert(0, formatMessage(formattedMessage)) }
        function onFormattedMessagesChanged() { messageText.text = formatMessage(_activeVehicle.formattedMessages) }
    }

    FactPanelController {
        id: controller
    }

    onLinkActivated: (link) => {
        if (link.startsWith('param://')) {
            var paramName = link.substr(8);
            _fact = controller.getParameterFact(-1, paramName, true)
            if (_fact != null) {
                paramEditorDialogFactory.open()
            }
        } else {
            Qt.openUrlExternally(link);
        }
    }

    QGCPopupDialogFactory {
        id: paramEditorDialogFactory

        dialogComponent: paramEditorDialogComponent
    }

    Component {
        id: paramEditorDialogComponent

        ParameterEditorDialog {
            title:          qsTr("Edit Parameter")
            fact:           messageText._fact
            destroyOnClose: true
        }
    }
}
