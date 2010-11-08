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

#ifndef SLUGSMAV_H
#define SLUGSMAV_H

#include "UAS.h"
#include "mavlink.h"

class SlugsMAV : public UAS
{
    Q_OBJECT
    Q_INTERFACES(UASInterface)
public:
    SlugsMAV(MAVLinkProtocol* mavlink, int id = 0);

public slots:
    /** @brief Receive a MAVLink message from this MAV */
    void receiveMessage(LinkInterface* link, mavlink_message_t message);

signals:
    // ESPECIAL SLUGS MESSAGE
    void slugsCPULoad(UASInterface* uas,
                      uint8_t sensLoad,
                      uint8_t ctrlLoad,
                      uint8_t batVolt,
                      quint64 time);

    void slugsAirData(UASInterface* uas,
                      float dinamicPressure,
                      float staticPresure,
                      uint16_t temperature,
                      quint64 time);

    void slugsSensorBias(UASInterface* uas,
                         double axBias,
                         double ayBias,
                         double azBias,
                         double gxBias,
                         double gyBias,
                         double gzBias,
                         quint64 time);

    void slugsDiagnostic(UASInterface* uas,
                         double diagFl1,
                         double diagFl2,
                         double diagFl3,
                         int16_t diagSh1,
                         int16_t diagSh2,
                         int16_t diagSh3,
                         quint64 time);

    void slugsPilotConsolePWM(UASInterface* uas,
                              uint16_t dt,
                              uint16_t dla,
                              uint16_t dra,
                              uint16_t dr,
                              uint16_t de,
                              quint64 time);

    void slugsPWM(UASInterface* uas,
                  uint16_t dt_c,
                  uint16_t dla_c,
                  uint16_t dra_c,
                  uint16_t dr_c,
                  uint16_t dle_c,
                  uint16_t dre_c,
                  uint16_t dlf_c,
                  uint16_t drf_c,
                  uint16_t da1_c,
                  uint16_t da2_c,
                  quint64 time);

};

#endif // SLUGSMAV_H
