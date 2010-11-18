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
    void slugsCPULoad(int systemId,
                      uint8_t sensLoad,
                      uint8_t ctrlLoad,
                      uint8_t batVolt,
                      quint64 time);

    void slugsAirData(int systemId,
                      float dinamicPressure,
                      float staticPresure,
                      uint16_t temperature,
                      quint64 time);

    void slugsSensorBias(int systemId,
                         double axBias,
                         double ayBias,
                         double azBias,
                         double gxBias,
                         double gyBias,
                         double gzBias,
                         quint64 time);

    void slugsDiagnostic(int systemId,
                         double diagFl1,
                         double diagFl2,
                         double diagFl3,
                         int16_t diagSh1,
                         int16_t diagSh2,
                         int16_t diagSh3,
                         quint64 time);

    void slugsPilotConsolePWM(int systemId,
                              uint16_t dt,
                              uint16_t dla,
                              uint16_t dra,
                              uint16_t dr,
                              uint16_t de,
                              quint64 time);

    void slugsPWM(int systemId,
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

    void slugsNavegation(int systemId,
                          double u_m,
                          double phi_c,
                          double theta_c,
                          double psiDot_c,
                          double ay_body,
                          double totalDist,
                          double dist2Go,
                          uint8_t fromWP,
                          uint8_t toWP,
                          quint64 time);

   void slugsDataLog(int systemId,
                 double logfl_1,
                 double logfl_2,
                 double logfl_3,
                 double logfl_4,
                 double logfl_5,
                 double logfl_6,
                 quint64 time);


   void slugsFilteredData(int systemId,
                          double filaX,
                          double filaY,
                          double filaZ,
                          double filgX,
                          double filgY,
                          double filgZ,
                          double filmX,
                          double filmY,
                          double filmZ,
                          quint64 time);

   void slugsGPSDateTime(int systemId,
                         uint8_t gpsyear,
                         uint8_t gpsmonth,
                         uint8_t gpsday,
                         uint8_t gpshour,
                         uint8_t gpsmin,
                         uint8_t gpssec,
                         uint8_t gpsvisSat,
                         quint64 time);


   // Standart messages MAVLINK used by SLUGS
   void slugsActionAck(int systemId,
                       uint8_t action,
                       uint8_t result);




};

#endif // SLUGSMAV_H
