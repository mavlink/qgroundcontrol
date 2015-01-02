/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009, 2010 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

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

/**
 * @file
 *   @brief Link Interface common functionality
 *
 *   @author Matthew Coleman <uavflightdirector@gmail.com>
 *
 */

#include "LinkInterface.h"

LinkInterface::LinkInterface(QGCSettingsGroup *pparentGroup, QString groupName) :
    QThread(0),
    QGCSettingsGroup(pparentGroup, groupName),
    _ownedByLinkManager(false),
    _deletedByLinkManager(false),
    _connectAtStartup(false)
{
    // Initialize everything for the data rate calculation buffers.
    inDataIndex = 0;
    outDataIndex = 0;
    link_id = LINK_INVALID_ID;    // link identifier set at invalid

    // Initialize our data rate buffers manually, cause C++<03 is dumb.
    for (int i = 0; i < dataRateBufferSize; ++i)
    {
        inDataWriteAmounts[i] = 0;
        inDataWriteTimes[i] = 0;
        outDataWriteAmounts[i] = 0;
        outDataWriteTimes[i] = 0;
    }

    qRegisterMetaType<LinkInterface*>("LinkInterface*");
}


int LinkInterface::getId() const{
    return link_id;
}

QString LinkInterface::getGroupName(void){
    return tr("LINK_%1").arg(getId());
}

void LinkInterface::setAutoConnect(bool enable){
    _connectAtStartup = enable;
}


void LinkInterface::serialize(QSettings* psettings)
{
    psettings->setValue("AUTO_CONNECT", _connectAtStartup);
}

void LinkInterface::deserialize(QSettings* psettings)
{
    _connectAtStartup = psettings->value("AUTO_CONNECT").toBool();
}
