import QtQuick  
import QtQuick.Layouts  
  
import QGroundControl  
import QGroundControl.Controls  
import QGroundControl.FactSystem  
import QGroundControl.FactControls  
import QGroundControl.Palette  
import QGroundControl.ScreenTools  
  
Rectangle {  
    id: root  
  
    color: qgcPal.window
    opacity: 0.75    
    border.color: qgcPal.windowShade  
    border.width: 1  
    radius: 4  
  
    property var activeVehicle: QGroundControl.multiVehicleManager.activeVehicle  
  
    // Гибридный список: Fact если доступен, иначе прямое свойство  
    property var factList: [  
        activeVehicle ? activeVehicle.getFact("groundSpeed") || activeVehicle.groundSpeed : null,  
        activeVehicle ? activeVehicle.getFact("batteryConsumeRate") || activeVehicle.batteryConsumeRate : null,  
        activeVehicle ? activeVehicle.getFact("distanceToHome") || activeVehicle.distanceToHome : null,  
        activeVehicle ? activeVehicle.getFact("altitudeRelative") || activeVehicle.altitudeRelative : null,  
        activeVehicle ? activeVehicle.getFact("batteryRemaining") || activeVehicle.batteryRemaining : null,  
        null // Площадь  
    ]  
  
    // Обновляем список при смене активного дрона  
    onActiveVehicleChanged: {  
        factList = [  
            activeVehicle ? activeVehicle.getFact("groundSpeed") || activeVehicle.groundSpeed : null,  
            activeVehicle ? activeVehicle.getFact("batteryConsumeRate") || activeVehicle.batteryConsumeRate : null,  
            activeVehicle ? activeVehicle.getFact("distanceToHome") || activeVehicle.distanceToHome : null,  
            activeVehicle ? activeVehicle.getFact("altitudeRelative") || activeVehicle.altitudeRelative : null,  
            activeVehicle ? activeVehicle.getFact("batteryRemaining") || activeVehicle.batteryRemaining : null,  
            null  
        ]  
    }  
  
    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }  
  
    GridLayout {  
        anchors.fill: parent  
        anchors.margins: ScreenTools.defaultFontPixelWidth * 1.5  
        columns: 3  
        rows: 2  
        columnSpacing: ScreenTools.defaultFontPixelWidth * -2.5 
        rowSpacing: ScreenTools.defaultFontPixelHeight * 0.3  
  
        Repeater {  
            model: 6  
  
            Column {  
                spacing: ScreenTools.defaultFontPixelHeight * 0.2  
  
                QGCLabel {  
                    text: {  
                        var f = root.factList[index]  
                        if (!f) return defaultLabels[index]  
                        return f.shortDescription || defaultLabels[index]  
                    }  
                    font.bold: true  
                    font.pointSize: ScreenTools.defaultFontPointSize * 0.95  
                    color: qgcPal.text  
                    horizontalAlignment: Text.AlignHCenter  
                    anchors.horizontalCenter: parent.horizontalCenter  
                }  
  
                QGCLabel {  
                    property var fact: root.factList[index]  
  
                    text: {  
                        if (!fact) return "—"  
                        // Если это Fact, используем valueString и units  
                        if (fact.valueString !== undefined) {  
                            return fact.valueString + (fact.units ? " " + fact.units : "")  
                        }  
                        // Иначе прямое свойство (число)  
                        if (typeof fact === "number" && !isNaN(fact)) {  
                            return fact.toFixed(1) + " " + getDefaultUnit(index)  
                        }  
                        return "—"  
                    }  
  
                    font.pointSize: ScreenTools.defaultFontPointSize * 1.42  
                    font.bold: true  
                    color: qgcPal.text  
                    horizontalAlignment: Text.AlignHCenter  
                    anchors.horizontalCenter: parent.horizontalCenter  
                }  
            }  
        }  
    }  
  
    readonly property var defaultLabels: [  
        "Скорость", "Расход", "Расстояние",  
        "Высота",   "Объём",  "Площадь"  
    ]  
  
    function getDefaultUnit(idx) {  
        var units = ["м/с", "л/м", "м", "м", "л", "га"]  
        return units[idx]  
    }  
}