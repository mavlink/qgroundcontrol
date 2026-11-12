pragma Singleton

import QtQuick

import QGroundControl

/// Texts in the application's units that the GPS panels share.
QtObject {
    /// @a meters in the application's horizontal distance unit: whole meters or feet, and kilometers or miles from
    /// one of those on, where the map scale changes its unit too, with one decimal below 100. The longest distance
    /// on earth then takes five digits, which the GPS indicator's drop-down has room for. The binding that calls
    /// this follows the unit setting.
    function distanceText(meters: real): string {
        const conversion = QGroundControl.unitsConversion
        const feet = QGroundControl.settingsManager.unitsSettings.horizontalDistanceUnits.rawValue
                     === UnitsSettings.HorizontalDistanceUnitsFeet
        const unitsPerLargeUnit = feet ? 5280 : 1000
        // Rounded as it is shown, so that 999.6 m reads "1.0 km" and not "1000 m".
        const units = Math.round(conversion.metersToAppSettingsHorizontalDistanceUnits(meters))
        if (units < unitsPerLargeUnit) {
            return conversion.metersToAppSettingsHorizontalDistanceUnitsString(meters, 0)
        }
        const largeUnits = units / unitsPerLargeUnit
        const number = largeUnits.toFixed(largeUnits < 99.95 ? 1 : 0)
        if (feet) {
            //: %1 is a distance in miles, such as 12.3 or 1250
            return qsTr("%1 mi").arg(number)
        }
        //: %1 is a distance in kilometers, such as 12.3 or 1250
        return qsTr("%1 km").arg(number)
    }
}
