/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009, 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
 This file is part of the QGROUNDCONTROL project
 
 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.
 
 ======================================================================*/

#include "ValuesWidgetController.h"

#include <QSettings>

const char* ValuesWidgetController::_groupKey =         "ValuesWidget";
const char* ValuesWidgetController::_largeValuesKey =   "large";
const char* ValuesWidgetController::_smallValuesKey =   "small";

ValuesWidgetController::ValuesWidgetController(void)
{
    QSettings settings;
    QStringList largeDefaults;

    settings.beginGroup(_groupKey);

    largeDefaults << "Vehicle.altitudeRelative" << "Vehicle.groundSpeed";
    _largeValues = settings.value(_largeValuesKey, largeDefaults).toStringList();
    _smallValues = settings.value(_smallValuesKey, QStringList()).toStringList();

    _altitudeProperties << "altitudeRelative" << "altitudeAMSL";

    // Keep back compat for removed WGS84 value
    if (_largeValues.contains ("Vehicle.altitudeWGS84")) {
        setLargeValues(_largeValues.replaceInStrings("Vehicle.altitudeWGS84", "Vehicle.altitudeRelative"));
    }
    if (_smallValues.contains ("Vehicle.altitudeWGS84")) {
        setSmallValues(_largeValues.replaceInStrings("Vehicle.altitudeWGS84", "Vehicle.altitudeRelative"));
    }
}

void ValuesWidgetController::setLargeValues(const QStringList& values)
{
    QSettings settings;

    settings.beginGroup(_groupKey);
    settings.setValue(_largeValuesKey, values);

    _largeValues = values;
    emit largeValuesChanged(values);
}

void ValuesWidgetController::setSmallValues(const QStringList& values)
{
    QSettings settings;

    settings.beginGroup(_groupKey);
    settings.setValue(_smallValuesKey, values);

    _smallValues = values;
    emit smallValuesChanged(values);
}
