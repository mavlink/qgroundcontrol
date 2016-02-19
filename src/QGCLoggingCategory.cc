/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
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

/// @file
///     @author Don Gagne <don@thegagnes.com>

#include "QGCLoggingCategory.h"

// Add Global logging categories (not class specific) here using QGC_LOGGING_CATEGORY
QGC_LOGGING_CATEGORY(FirmwareUpgradeLog,        "FirmwareUpgradeLog")
QGC_LOGGING_CATEGORY(FirmwareUpgradeVerboseLog, "FirmwareUpgradeVerboseLog")
QGC_LOGGING_CATEGORY(MissionCommandsLog,        "MissionCommandsLog")
QGC_LOGGING_CATEGORY(MissionItemLog,            "MissionItemLog")

QGCLoggingCategoryRegister* _instance = NULL;

QGCLoggingCategoryRegister* QGCLoggingCategoryRegister::instance(void)
{
    if (!_instance) {
        _instance = new QGCLoggingCategoryRegister();
        Q_CHECK_PTR(_instance);
    }
    
    return _instance;
}
