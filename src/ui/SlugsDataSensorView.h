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
    void slugLocalPositionChanged(UASInterface* uasTemp,
                                  double x,
                                  double y,
                                  double z,
                                  quint64 time);
    /**
         * @brief Adds the UAS for data display
         *
         * Adds the UAS and makes all the correct connections for data display on the Widgets
    */
    void slugSpeedLocalPositionChanged(UASInterface* uasTemp,
                                       double vx,
                                       double vy,
                                       double vz,
                                       quint64 time);
    /**
         * @brief Adds the UAS for data display
         *
         * Adds the UAS and makes all the correct connections for data display on the Widgets
    */
    void slugAttitudeChanged(UASInterface* uasTemp,
                             double slugroll,
                             double slugpitch,
                             double slugyaw,
                             quint64 time);
    /**
         * @brief Adds the UAS for data display
         *
         * Adds the UAS and makes all the correct connections for data display on the Widgets
    */
    void slugsSensorBiasChanged(UASInterface* uasTemp,
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
    void slugsDiagnosticMessageChanged(UASInterface* uasTemp,
                                       double diagfl1,
                                       double diagfl2,
                                       double diagfl3,
                                       int16_t diagsh1,
                                       int16_t diagsh2,
                                       int16_t diagsh3,
                                       quint64 time);

protected:
    QTimer* updateTimer;
     UASInterface* activeUAS;

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




private:
    Ui::SlugsDataSensorView *ui;
    void loadParameters();


};

#endif // SLUGSDATASENSORVIEW_H
