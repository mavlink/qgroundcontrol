import QtQuick 2.2
import QtQuick.Controls 1.2

import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controllers   1.0

FactPanel {
    id:             panel
    anchors.fill:   parent
    color:          qgcPal.windowShadeDark

    QGCPalette          { id: qgcPal; colorGroupEnabled: enabled }
    FactPanelController { id: controller; factPanel: panel }

    ESP8266ComponentController {
        id: esp8266
    }

    property Fact debugEnabled:     controller.getParameterFact(esp8266.componentID, "DEBUG_ENABLED")
    property Fact wifiChannel:      controller.getParameterFact(esp8266.componentID, "WIFI_CHANNEL")
    property Fact wifiHostPort:     controller.getParameterFact(esp8266.componentID, "WIFI_UDP_HPORT")
    property Fact wifiClientPort:   controller.getParameterFact(esp8266.componentID, "WIFI_UDP_CPORT")
    property Fact uartBaud:         controller.getParameterFact(esp8266.componentID, "UART_BAUDRATE")

    Column {
        anchors.fill:       parent
        anchors.margins:    8
        VehicleSummaryRow {
            labelText: "Firmware Version:"
            valueText: esp8266.version
        }
        VehicleSummaryRow {
            labelText: "WiFi Channel:"
            valueText: wifiChannel.valueString
        }
        VehicleSummaryRow {
            labelText: "WiFi SSID:"
            valueText: esp8266.wifiSSID
        }
        VehicleSummaryRow {
            labelText: "WiFi Password:"
            valueText: esp8266.wifiPassword
        }
        VehicleSummaryRow {
            labelText: "UART Baud Rate:"
            valueText: uartBaud.valueString
        }
    }
}
