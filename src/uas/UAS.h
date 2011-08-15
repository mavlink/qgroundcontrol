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
 *   @brief Definition of Unmanned Aerial Vehicle object
 *
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#ifndef _UAS_H_
#define _UAS_H_

#include "UASInterface.h"
#include "MG.h"
#include <MAVLinkProtocol.h>
#include "QGCMAVLink.h"

/**
 * @brief A generic MAVLINK-connected MAV/UAV
 *
 * This class represents one vehicle. It can be used like the real vehicle, e.g. a call to halt()
 * will automatically send the appropriate messages to the vehicle. The vehicle state will also be
 * automatically updated by the comm architecture, so when writing code to e.g. control the vehicle
 * no knowledge of the communication infrastructure is needed.
 */
class UAS : public UASInterface
{
    Q_OBJECT
public:
    UAS(MAVLinkProtocol* protocol, int id = 0);
    ~UAS();

    enum BatteryType {
        NICD = 0,
        NIMH = 1,
        LIION = 2,
        LIPOLY = 3,
        LIFE = 4,
        AGZN = 5
    }; ///< The type of battery used

    static const int lipoFull = 4.2f;  ///< 100% charged voltage
    static const int lipoEmpty = 3.5f; ///< Discharged voltage

    /* MANAGEMENT */

    /** @brief The name of the robot */
    QString getUASName(void) const;
    /** @brief Get short state */
    const QString& getShortState() const;
    /** @brief Get short mode */
    const QString& getShortMode() const;
    /** @brief Get the unique system id */
    int getUASID() const;
    /** @brief Get the airframe */
    int getAirframe() const {
        return airframe;
    }
    /** @brief The time interval the robot is switched on */
    quint64 getUptime() const;
    /** @brief Get the status flag for the communication */
    int getCommunicationStatus() const;
    /** @brief Add one measurement and get low-passed voltage */
    float filterVoltage(float value) const;
    /** @brief Get the links associated with this robot */
    QList<LinkInterface*>* getLinks();

    double getLocalX() const {
        return localX;
    }
    double getLocalY() const {
        return localY;
    }
    double getLocalZ() const {
        return localZ;
    }
    double getLatitude() const {
        return latitude;
    }
    double getLongitude() const {
        return longitude;
    }
    double getAltitude() const {
        return altitude;
    }

    double getRoll() const {
        return roll;
    }
    double getPitch() const {
        return pitch;
    }
    double getYaw() const {
        return yaw;
    }
    bool getSelected() const;

    friend class UASWaypointManager;

protected: //COMMENTS FOR TEST UNIT
    int uasId;                    ///< Unique system ID
    unsigned char type;           ///< UAS type (from type enum)
    quint64 startTime;            ///< The time the UAS was switched on
    CommStatus commStatus;        ///< Communication status
    QString name;                 ///< Human-friendly name of the vehicle, e.g. bravo
    int autopilot;                ///< Type of the Autopilot: -1: None, 0: Generic, 1: PIXHAWK, 2: SLUGS, 3: Ardupilot (up to 15 types), defined in MAV_AUTOPILOT_TYPE ENUM
    QList<LinkInterface*>* links; ///< List of links this UAS can be reached by
    QList<int> unknownPackets;    ///< Packet IDs which are unknown and have been received
    MAVLinkProtocol* mavlink;     ///< Reference to the MAVLink instance
    BatteryType batteryType;      ///< The battery type
    int cells;                    ///< Number of cells

    UASWaypointManager waypointManager;

    QList<double> actuatorValues;
    QList<QString> actuatorNames;

    QList<double> motorValues;
    QList<QString> motorNames;

    double thrustSum;           ///< Sum of forward/up thrust of all thrust actuators, in Newtons
    double thrustMax;           ///< Maximum forward/up thrust of this vehicle, in Newtons

    // Battery stats
    float fullVoltage;          ///< Voltage of the fully charged battery (100%)
    float emptyVoltage;         ///< Voltage of the empty battery (0%)
    float startVoltage;         ///< Voltage at system start
    float warnVoltage;          ///< Voltage where QGC will start to warn about low battery
    float warnLevelPercent;     ///< Warning level, in percent
    double currentVoltage;      ///< Voltage currently measured
    float lpVoltage;            ///< Low-pass filtered voltage
    bool batteryRemainingEstimateEnabled; ///< If the estimate is enabled, QGC will try to estimate the remaining battery life
    float chargeLevel;          ///< Charge level of battery, in percent
    int timeRemaining;          ///< Remaining time calculated based on previous and current
    int mode;                   ///< The current mode of the MAV
    int status;                 ///< The current status of the MAV
    int navMode;                ///< The current navigation mode of the MAV
    quint64 onboardTimeOffset;

    bool controlRollManual;     ///< status flag, true if roll is controlled manually
    bool controlPitchManual;    ///< status flag, true if pitch is controlled manually
    bool controlYawManual;      ///< status flag, true if yaw is controlled manually
    bool controlThrustManual;   ///< status flag, true if thrust is controlled manually

    double manualRollAngle;     ///< Roll angle set by human pilot (radians)
    double manualPitchAngle;    ///< Pitch angle set by human pilot (radians)
    double manualYawAngle;      ///< Yaw angle set by human pilot (radians)
    double manualThrust;        ///< Thrust set by human pilot (radians)
    float receiveDropRate;      ///< Percentage of packets that were dropped on the MAV's receiving link (from GCS and other MAVs)
    float sendDropRate;         ///< Percentage of packets that were not received from the MAV by the GCS
    bool lowBattAlarm;          ///< Switch if battery is low
    bool positionLock;          ///< Status if position information is available or not
    double localX;
    double localY;
    double localZ;
    double latitude;
    double longitude;
    double altitude;
    double speedX;              ///< True speed in X axis
    double speedY;              ///< True speed in Y axis
    double speedZ;              ///< True speed in Z axis
    double roll;
    double pitch;
    double yaw;
    quint64 lastHeartbeat;      ///< Time of the last heartbeat message
    QTimer* statusTimeout;      ///< Timer for various status timeouts

    int imageSize;              ///< Image size being transmitted (bytes)
    int imagePackets;           ///< Number of data packets being sent for this image
    int imagePacketsArrived;    ///< Number of data packets recieved
    int imagePayload;           ///< Payload size per transmitted packet (bytes). Standard is 254, and decreases when image resolution increases.
    int imageQuality;           ///< Quality of the transmitted image (percentage)
    int imageType;              ///< Type of the transmitted image (BMP, PNG, JPEG, RAW 8 bit, RAW 32 bit)
    QByteArray imageRecBuffer;  ///< Buffer for the incoming bytestream
    QImage image;               ///< Image data of last completely transmitted image
    quint64 imageStart;

    QMap<int, QMap<QString, float>* > parameters; ///< All parameters
    bool paramsOnceRequested;   ///< If the parameter list has been read at least once
    int airframe;               ///< The airframe type
    bool attitudeKnown;         ///< True if attitude was received, false else
    QGCUASParamManager* paramManager; ///< Parameter manager class
    QString shortStateText;     ///< Short textual state description
    QString shortModeText;      ///< Short textual mode description

public:
    /** @brief Set the current battery type */
    void setBattery(BatteryType type, int cells);
    /** @brief Estimate how much flight time is remaining */
    int calculateTimeRemaining();
    /** @brief Get the current charge level */
    float getChargeLevel();
    /** @brief Get the human-readable status message for this code */
    void getStatusForCode(int statusCode, QString& uasState, QString& stateDescription);
    /** @brief Get the human-readable navigation mode translation for this mode */
    QString getNavModeText(int mode);
    /** @brief Check if vehicle is in autonomous mode */
    bool isAuto();

    UASWaypointManager* getWaypointManager() {
        return &waypointManager;
    }
    /** @brief Get reference to the param manager **/
    QGCUASParamManager* getParamManager() const {
        return paramManager;
    }
    // TODO Will be removed
    /** @brief Set reference to the param manager **/
    void setParamManager(QGCUASParamManager* manager) {
        paramManager = manager;
    }
    int getSystemType();
    QImage getImage();
    void requestImage();
    int getAutopilotType() {
        return autopilot;
    }

public slots:
    /** @brief Set the autopilot type */
    void setAutopilotType(int apType) {
        autopilot = apType;
        emit systemSpecsChanged(uasId);
    }
    /** @brief Set the type of airframe */
    void setSystemType(int systemType);
    /** @brief Set the specific airframe type */
    void setAirframe(int airframe) {
        this->airframe = airframe;
        emit systemSpecsChanged(uasId);
    }
    /** @brief Set a new name **/
    void setUASName(const QString& name);
    /** @brief Executes an action **/
    void setAction(MAV_ACTION action);
    /** @brief Executes a command **/
    void executeCommand(MAV_CMD command);
    /** @brief Executes a command **/
    void executeCommand(MAV_CMD command, int confirmation, float param1, float param2, float param3, float param4, int component);
    /** @brief Set the current battery type and voltages */
    void setBatterySpecs(const QString& specs);
    /** @brief Get the current battery type and specs */
    QString getBatterySpecs();

    /** @brief Launches the system **/
    void launch();
    /** @brief Write this waypoint to the list of waypoints */
    //void setWaypoint(Waypoint* wp); FIXME tbd
    /** @brief Set currently active waypoint */
    //void setWaypointActive(int id); FIXME tbd
    /** @brief Order the robot to return home / to land on the runway **/
    void home();
    void halt();
    void go();
    /** @brief Places the UAV in Hardware-in-the-Loop simulation status **/
    void startHil();

    /** @brief Stops the UAV's Hardware-in-the-Loop simulation status **/
    void stopHil();


    /** @brief Stops the robot system. If it is an MAV, the robot starts the emergency landing procedure **/
    void emergencySTOP();

    /** @brief Kills the robot. All systems are immediately shut down (e.g. the main power line is cut). This might lead to a crash **/
    bool emergencyKILL();

    /** @brief Shut the system cleanly down. Will shut down any onboard computers **/
    void shutdown();

    /** @brief Set the target position for the robot to navigate to. */
    void setTargetPosition(float x, float y, float z, float yaw);

    void startLowBattAlarm();
    void stopLowBattAlarm();

    //void requestWaypoints();  FIXME tbd
    //void clearWaypointList();   FIXME tbd

    /** @brief Enable the motors */
    void enable_motors();
    /** @brief Disable the motors */
    void disable_motors();

    /** @brief Set the values for the manual control of the vehicle */
    void setManualControlCommands(double roll, double pitch, double yaw, double thrust);
    /** @brief Receive a button pressed event from an input device, e.g. joystick */
    void receiveButton(int buttonIndex);

    /** @brief Add a link associated with this robot */
    void addLink(LinkInterface* link);
    /** @brief Remove a link associated with this robot */
    void removeLink(QObject* object);

    /** @brief Receive a message from one of the communication links. */
    virtual void receiveMessage(LinkInterface* link, mavlink_message_t message);

    /** @brief Send a message over this link (to this or to all UAS on this link) */
    void sendMessage(LinkInterface* link, mavlink_message_t message);
    /** @brief Send a message over all links this UAS can be reached with (!= all links) */
    void sendMessage(mavlink_message_t message);

    /** @brief Temporary Hack for sending packets to patch Antenna. Send a message over all serial links except for this UAS's */
    void forwardMessage(mavlink_message_t message);

    /** @brief Set this UAS as the system currently in focus, e.g. in the main display widgets */
    void setSelected();

    /** @brief Set current mode of operation, e.g. auto or manual */
    void setMode(int mode);

    /** @brief Request all parameters */
    void requestParameters();

    /** @brief Request a single parameter by index */
    void requestParameter(int component, int parameter);

    /** @brief Set a system parameter */
    void setParameter(const int component, const QString& id, const float value);

    /** @brief Write parameters to permanent storage */
    void writeParametersToStorage();
    /** @brief Read parameters from permanent storage */
    void readParametersFromStorage();

    /** @brief Get the names of all parameters */
    QList<QString> getParameterNames(int component);

    /** @brief Get the ids of all components */
    QList<int> getComponentIds();

    void enableAllDataTransmission(int rate);
    void enableRawSensorDataTransmission(int rate);
    void enableExtendedSystemStatusTransmission(int rate);
    void enableRCChannelDataTransmission(int rate);
    void enableRawControllerDataTransmission(int rate);
    //void enableRawSensorFusionTransmission(int rate);
    void enablePositionTransmission(int rate);
    void enableExtra1Transmission(int rate);
    void enableExtra2Transmission(int rate);
    void enableExtra3Transmission(int rate);

    /** @brief Update the system state */
    void updateState();

    /** @brief Set world frame origin at current GPS position */
    void setLocalOriginAtCurrentGPSPosition();
    /** @brief Set world frame origin / home position at this GPS position */
    void setHomePosition(double lat, double lon, double alt);
    /** @brief Set local position setpoint */
    void setLocalPositionSetpoint(float x, float y, float z, float yaw);
    /** @brief Add an offset in body frame to the setpoint */
    void setLocalPositionOffset(float x, float y, float z, float yaw);

    void startRadioControlCalibration();
    void startMagnetometerCalibration();
    void startGyroscopeCalibration();
    void startPressureCalibration();

    void startDataRecording();
    void pauseDataRecording();
    void stopDataRecording();

signals:

    /** @brief The main/battery voltage has changed/was updated */
    void voltageChanged(int uasId, double voltage);
    /** @brief An actuator value has changed */
    void actuatorChanged(UASInterface*, int actId, double value);
    /** @brief An actuator value has changed */
    void actuatorChanged(UASInterface* uas, QString actuatorName, double min, double max, double value);
    void motorChanged(UASInterface* uas, QString motorName, double min, double max, double value);
    /** @brief The system load (MCU/CPU usage) changed */
    void loadChanged(UASInterface* uas, double load);
    /** @brief Propagate a heartbeat received from the system */
    void heartbeat(UASInterface* uas);
    void imageStarted(quint64 timestamp);
    /** @brief A new camera image has arrived */
    void imageReady(UASInterface* uas);

protected:
    /** @brief Get the UNIX timestamp in milliseconds */
    quint64 getUnixTime(quint64 time=0);

protected slots:
    /** @brief Write settings to disk */
    void writeSettings();
    /** @brief Read settings from disk */
    void readSettings();

    // MESSAGE RECEPTION
    /** @brief Receive a named value message */
    void receiveMessageNamedValue(const mavlink_message_t& message);

private:
//    unsigned int mode;          ///< The current mode of the MAV
};


#endif // _UAS_H_
