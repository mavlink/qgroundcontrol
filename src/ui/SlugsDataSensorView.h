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
#include "mavlink.h"


namespace Ui
{
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
         * @brief Updates the Raw Data widget
    */
    void slugRawDataChanged (int uasId, const mavlink_raw_imu_t& rawData);

#ifdef MAVLINK_ENABLED_SLUGS
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
         * @brief set COG and SOG values
         *
         * COG and SOG GPS display on the Widgets
    */
    void slugsGPSCogSog(int systemId,
                        double cog,
                        double sog);



    /**
         * @brief Updates the CPU load widget - 170
    */
    void slugsCpuLoadChanged(int systemId,
                             const mavlink_cpu_load_t& cpuLoad);

    /**
         * @brief Updates the air data widget - 171
    */
    void slugsAirDataChanged(int systemId,
                             const mavlink_air_data_t& airData);

    /**
         * @brief Updates the sensor bias widget - 172
    */
    void slugsSensorBiasChanged(int systemId,
                                const mavlink_sensor_bias_t& sensorBias);

    /**
         * @brief Updates the diagnostic widget - 173
    */
    void slugsDiagnosticMessageChanged(int systemId,
                                       const mavlink_diagnostic_t& diagnostic);


    /**
         * @brief Updates the Navigation widget - 176
    */
    void slugsNavegationChanged(int systemId,
                                const mavlink_slugs_navigation_t& slugsNavigation);

    /**
         * @brief Updates the Data Log widget - 177
    */
    void  slugsDataLogChanged(int systemId,
                              const mavlink_data_log_t& dataLog);

//   /**
//        * @brief Updates the PWM Commands widget - 175
//   */
//   void slugsPWMChanged(int systemId,
//                        const mavlink_servo_output_raw_t& pwmCommands);

    /**
         * @brief Updates the filtered sensor measurements widget - 178
    */
    void slugsFilteredDataChanged(int systemId,
                                  const mavlink_scaled_imu_t& filteredData);


    /**
         * @brief Updates the gps Date Time widget - 179
    */
    void slugsGPSDateTimeChanged(int systemId,
                                 const mavlink_gps_date_time_t& gpsDateTime);


    void slugsRCRawChannels(int systemId,
                            const mavlink_rc_channels_raw_t& gpsDateTime);

    void slugsRCServo(int systemId,
                      const mavlink_servo_output_raw_t& gpsDateTime);


#endif // MAVLINK_ENABLED_SLUGS

protected:
    UASInterface* activeUAS;

private:
    Ui::SlugsDataSensorView *ui;







};

#endif // SLUGSDATASENSORVIEW_H
