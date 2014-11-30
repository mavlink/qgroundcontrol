/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
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

#include "MockUASManager.h"

MockUASManager::MockUASManager(void) :
    UASManagerInterface(NULL, false /* do not register singleton with QGCApplication */),
    _mockUAS(NULL)
{
    
}

UASInterface* MockUASManager::getActiveUAS(void)
{
    return _mockUAS;
}

void MockUASManager::setMockActiveUAS(MockUAS* mockUAS)
{
    // Signal components that the last UAS is no longer active.
    if (_mockUAS != NULL) {
        emit activeUASStatusChanged(_mockUAS, false);
        emit activeUASStatusChanged(_mockUAS->getUASID(), false);
    }
    _mockUAS = mockUAS;
    
    // And signal that a new UAS is.
    emit activeUASSet(_mockUAS);
    if (_mockUAS)
    {
        // We don't support swiching between different UAS
        //_mockUAS->setSelected();
        emit activeUASSet(_mockUAS->getUASID());
        emit activeUASStatusChanged(_mockUAS, true);
        emit activeUASStatusChanged(_mockUAS->getUASID(), true);
    }
}

