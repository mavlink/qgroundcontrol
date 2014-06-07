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

#ifndef ARDUPILOTMEGAMAV_H
#define ARDUPILOTMEGAMAV_H

#include "UAS.h"
class ArduPilotMegaMAV : public UAS
{
    Q_OBJECT
public:
    ArduPilotMegaMAV(MAVLinkProtocol* mavlink, QThread* thread, int id = 0);
    /** @brief Set camera mount stabilization modes */
    void setMountConfigure(unsigned char mode, bool stabilize_roll,bool stabilize_pitch,bool stabilize_yaw);
    /** @brief Set camera mount control */
    void setMountControl(double pa,double pb,double pc,bool islatlong);
public slots:
    /** @brief Receive a MAVLink message from this MAV */
    void receiveMessage(LinkInterface* link, mavlink_message_t message);
    void sendTxRequests();
private:
    QTimer *txReqTimer;
};

#endif // ARDUPILOTMAV_H
