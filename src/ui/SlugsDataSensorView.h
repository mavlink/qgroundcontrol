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
 *   @brief Grpahical presentation of SLUGS generated data
 *
 *   @author Juan F. Robles  <jfroblesc@gmail.com>
 *
 */

#ifndef SLUGSDATASENSORVIEW_H
#define SLUGSDATASENSORVIEW_H

#include <QWidget>

#include "UASInterface.h"
#include "SlugsMAV.h"

namespace Ui {
    class SlugsDataSensorView;
}

class SlugsDataSensorView : public QWidget
{
    Q_OBJECT

public:
    explicit SlugsDataSensorView(QWidget *parent = 0);
    ~SlugsDataSensorView();

public slots:
    /**
         * @brief Adds the UAS for data display
         *
         * Adds the UAS and makes all the correct connections for data display on the Widgets. If
         * there is no current UAS active, it sets it as active.

         * @param uas The UAS being added
    */
    void addUAS(UASInterface* uas);

    /**
         * @brief Sets the UAS as active
         *
         * @param uas The UAS being set as active
    */
    void setActiveUAS(UASInterface* uas);

    /**
         * @brief Adds the UAS for data display
         *
         * Adds the UAS and makes all the correct connections for data display on the Widgets
    */

    void refresh();




    /**
         * @brief Adds the UAS for data display
         *
         * Adds the UAS and makes all the correct connections for data display on the Widgets
    */
    void slugLocalPositionChanged(UASInterface* uas,
                                  double x,
                                  double y,
                                  double z,
                                  quint64 time);
    /**
         * @brief Adds the UAS for data display
         *
         * Adds the UAS and makes all the correct connections for data display on the Widgets
    */
    void slugSpeedLocalPositionChanged(UASInterface* uas,
                                       double vx,
                                       double vy,
                                       double vz,
                                       quint64 time);
    /**
         * @brief Adds the UAS for data display
         *
         * Adds the UAS and makes all the correct connections for data display on the Widgets
    */
    void slugAttitudeChanged(UASInterface* uas,
                             double slugroll,
                             double slugpitch,
                             double slugyaw,
                             quint64 time);

    /**
         * @brief Adds the UAS for data display
         *
         * Adds the UAS and makes all the correct connections for data display on the Widgets
    */
    void slugsGlobalPositionChanged(UASInterface* uas,
                                    double lat,
                                    double lon,
                                    double alt,
                                    quint64 time);
    /**
         * @brief Adds the UAS for data display
         *
         * Adds the UAS and makes all the correct connections for data display on the Widgets
    */
    void slugsSensorBiasChanged(int systemId,
                                double axb,
                                double ayb,
                                double azb,
                                double gxb,
                                double gyb,
                                double gzb,
                                quint64 time);
    /**
         * @brief Adds the UAS for data display
         *
         * Adds the UAS and makes all the correct connections for data display on the Widgets
    */
    void slugsDiagnosticMessageChanged(int systemId,
                                       double diagfl1,
                                       double diagfl2,
                                       double diagfl3,
                                       int16_t diagsh1,
                                       int16_t diagsh2,
                                       int16_t diagsh3,
                                       quint64 time);

    /**
         * @brief Adds the UAS for data display
         *
         * Adds the UAS and makes all the correct connections for data display on the Widgets
    */
    void slugsCpuLoadChanged(int systemId,
                             uint8_t sensload,
                             uint8_t ctrlload,
                             uint8_t batvolt,
                             quint64 time);


    /**
         * @brief Adds the UAS for data display
         *
         * Adds the UAS and makes all the correct connections for data display on the Widgets
    */
    void slugsNavegationChanged(int systemId,
                                double navu_m,
                                double navphi_c,
                                double navtheta_c,
                                double navpsiDot_c,
                                double navay_body,
                                double navtotalDist,
                                double navdist2Go,
                                uint8_t navfromWP,
                                uint8_t navtoWP,
                                quint64 time);

    /**
         * @brief Adds the UAS for data display
         *
         * Adds the UAS and makes all the correct connections for data display on the Widgets
    */
   void  slugsDataLogChanged(int systemId,
                            double logfl_1,
                            double logfl_2,
                            double logfl_3,
                            double logfl_4,
                            double logfl_5,
                            double logfl_6,
                            quint64 time);

   /**
        * @brief Adds the UAS for data display
        *
        * Adds the UAS and makes all the correct connections for data display on the Widgets
   */

   void slugsPWMChanged(int systemId,
                        uint16_t vdt_c,
                        uint16_t vdla_c,
                        uint16_t vdra_c,
                        uint16_t vdr_c,
                        uint16_t vdle_c,
                        uint16_t vdre_c,
                        uint16_t vdlf_c,
                        uint16_t vdrf_c,
                        uint16_t vda1_c,
                        uint16_t vda2_c,
                        quint64 time);

   /**
        * @brief Adds the UAS for data display
        *
        * Adds the UAS and makes all the correct connections for data display on the Widgets
   */
   void slugsFilteredDataChanged(int systemId,
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





    void slugsGPSDateTimeChanged(int systemId,
                                    uint8_t gpsyear,
                                     uint8_t gpsmonth,
                                     uint8_t gpsday,
                                     uint8_t gpshour,
                                     uint8_t gpsmin,
                                     uint8_t gpssec,
                                    uint8_t gpsvisSat,
                                    quint64 time);


protected:
    QTimer* updateTimer;
     UASInterface* activeUAS;

     //Global Position
     double Latitude;
     double Longitude;
     double Height;
     quint64 timeGlobalPosition;

     // Position and Attitude
     //Position
     double Xpos;
     double Ypos;
     double Zpos;
     quint64 TimeActualPosition;
     //Speed
     double VXpos;
     double VYpos;
     double VZpos;
     quint64 TimeActualSpeed;
     //Attitude
     double roll;
     double pitch;
     double yaw;
     quint64 TimeActualAttitude;

     //Sensor Biases
     //Acelerometer
     double Axb;
     double Ayb;
     double Azb;

     //Gyro
     double Gxb;
     double Gyb;
     double Gzb;
     quint64 TimeActualBias;

     //Diagnostic
     double diagFl1;
     double diagFl2;
     double diagFl3;
     int16_t diagSh1;
     int16_t diagSh2;
     int16_t diagSh3;
     quint64 timeDiagnostic;

     //CPU Load
     uint8_t sensLoad;
     uint8_t ctrlLoad;
     uint8_t batVolt;
     quint64 timeCpuLoad;

     //navigation data
     double u_m;
     double phi_c;
     double theta_c;
     double psiDot_c;
     double ay_body;
     double totalDist;
     double dist2Go;
     uint8_t fromWP;
     uint8_t toWP;
     quint64 timeNavigation;

     // Data Log
     double Logfl_1;
     double Logfl_2;
     double Logfl_3;
     double Logfl_4;
     double Logfl_5;
     double Logfl_6;
     quint64 timeDataLog;

     //pwm commands
     uint16_t dt_c; ///< AutoPilot's throttle command
     uint16_t dla_c; ///< AutoPilot's left aileron command
     uint16_t dra_c; ///< AutoPilot's right aileron command
     uint16_t dr_c; ///< AutoPilot's rudder command
     uint16_t dle_c; ///< AutoPilot's left elevator command
     uint16_t dre_c; ///< AutoPilot's right elevator command
     uint16_t dlf_c; ///< AutoPilot's left  flap command
     uint16_t drf_c; ///< AutoPilot's right flap command
     uint16_t aux1; ///< AutoPilot's aux1 command
     uint16_t aux2; ///< AutoPilot's aux2 command
     quint64 timePWMCommand;

     //filtered data
     double aX; ///< Accelerometer X value (m/s^2)
     double aY; ///< Accelerometer Y value (m/s^2)
     double aZ; ///< Accelerometer Z value (m/s^2)
     double gX; ///< Gyro X value (rad/s)
     double gY; ///< Gyro Y value (rad/s)
     double gZ; ///< Gyro Z value (rad/s)
     double mX; ///< Magnetometer X (normalized to 1)
     double mY; ///< Magnetometer Y (normalized to 1)
     double mZ; ///< Magnetometer Z (normalized to 1)
     quint64 timeFiltered;

     //gps date and time
     uint8_t year; ///< Year reported by Gps
     uint8_t month; ///< Month reported by Gps
     uint8_t day; ///< Day reported by Gps
     uint8_t hour; ///< Hour reported by Gps
     uint8_t min; ///< Min reported by Gps
     uint8_t sec; ///< Sec reported by Gps
     uint8_t visSat; ///< Visible sattelites reported by Gps
     quint64 timeGPSDateTime;


private:
    Ui::SlugsDataSensorView *ui;
    void loadParameters();


};

#endif // SLUGSDATASENSORVIEW_H
