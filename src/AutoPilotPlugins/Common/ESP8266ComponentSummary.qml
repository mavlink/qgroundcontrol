import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.FactControls
import QGroundControl.Controls

Item {
    implicitWidth: mainLayout.implicitWidth
    implicitHeight: mainLayout.implicitHeight
    width: parent.width  // grows when Loader is wider than implicitWidth

    FactPanelController { id: controller; }

    ESP8266ComponentController {
        id:         esp8266
    }

    property Fact debugEnabled:     controller.getParameterFact(esp8266.componentID, "DEBUG_ENABLED")
    property Fact wifiChannel:      controller.getParameterFact(esp8266.componentID, "WIFI_CHANNEL")
    property Fact wifiHostPort:     controller.getParameterFact(esp8266.componentID, "WIFI_UDP_HPORT")
    property Fact wifiClientPort:   controller.getParameterFact(esp8266.componentID, "WIFI_UDP_CPORT")
    property Fact uartBaud:         controller.getParameterFact(esp8266.componentID, "UART_BAUDRATE")
    property Fact wifiMode:         controller.getParameterFact(esp8266.componentID, "WIFI_MODE", false) //-- Don't bitch if missing

    ColumnLayout {
        id: mainLayout
        spacing: 0

        VehicleSummaryRow {
            labelText: qsTr("Firmware Version")
            valueText: esp8266.version
        }
        VehicleSummaryRow {
            labelText: qsTr("WiFi Mode")
            valueText: wifiMode ? (wifiMode.value === 0 ? qsTr("AP Mode") : qsTr("Station Mode")) : qsTr("AP Mode")
        }
        VehicleSummaryRow {
            labelText:  qsTr("WiFi Channel")
            valueText:  wifiChannel ? wifiChannel.valueString : ""
            visible:    wifiMode ? wifiMode.value === 0 : true
        }
        VehicleSummaryRow {
            labelText: qsTr("WiFi AP SSID")
            valueText: esp8266.wifiSSID
        }
        VehicleSummaryRow {
            labelText: qsTr("WiFi AP Password")
            valueText: esp8266.wifiPassword
        }
        /* Too much info makes it all crammed
        VehicleSummaryRow {
            labelText: qsTr("WiFi STA SSID")
            valueText: esp8266.wifiSSIDSta
        }
        VehicleSummaryRow {
            labelText: qsTr("WiFi STA Password")
            valueText: esp8266.wifiPasswordSta
        }
        */
        VehicleSummaryRow {
            labelText: qsTr("UART Baud Rate")
            valueText: uartBaud ? uartBaud.valueString : ""
        }
    }
}
