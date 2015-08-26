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
#include <MAVLinkProtocol.h>
#include <QVector3D>
#include "QGCMAVLink.h"
#include "FileManager.h"
#ifndef __mobile__
#include "JoystickInput.h"
#include "QGCHilLink.h"
#include "QGCFlightGearLink.h"
#include "QGCJSBSimLink.h"
#include "QGCXPlaneLink.h"
#endif

Q_DECLARE_LOGGING_CATEGORY(UASLog)

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
    UAS(MAVLinkProtocol* protocol, int id, MAV_AUTOPILOT autopilotType);
    ~UAS();

    float lipoFull;  ///< 100% charged voltage
    float lipoEmpty; ///< Discharged voltage

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
    int getAirframe() const
    {
        return airframe;
    }
    /** @brief Get the components */
    QMap<int, QString> getComponents();

    /** @brief The time interval the robot is switched on */
    quint64 getUptime() const;
    /** @brief Add one measurement and get low-passed voltage */
    float filterVoltage(float value) const;
    /** @brief Get the links associated with this robot */
    QList<LinkInterface*> getLinks();
    bool isLogReplay(void);

    Q_PROPERTY(double localX READ getLocalX WRITE setLocalX NOTIFY localXChanged)
    Q_PROPERTY(double localY READ getLocalY WRITE setLocalY NOTIFY localYChanged)
    Q_PROPERTY(double localZ READ getLocalZ WRITE setLocalZ NOTIFY localZChanged)
    Q_PROPERTY(double latitude READ getLatitude WRITE setLatitude NOTIFY latitudeChanged)
    Q_PROPERTY(double longitude READ getLongitude WRITE setLongitude NOTIFY longitudeChanged)
    Q_PROPERTY(double satelliteCount READ getSatelliteCount WRITE setSatelliteCount NOTIFY satelliteCountChanged)
    Q_PROPERTY(bool isLocalPositionKnown READ localPositionKnown)
    Q_PROPERTY(bool isGlobalPositionKnown READ globalPositionKnown)
    Q_PROPERTY(double roll READ getRoll WRITE setRoll NOTIFY rollChanged)
    Q_PROPERTY(double pitch READ getPitch WRITE setPitch NOTIFY pitchChanged)
    Q_PROPERTY(double yaw READ getYaw WRITE setYaw NOTIFY yawChanged)
    Q_PROPERTY(double distToWaypoint READ getDistToWaypoint WRITE setDistToWaypoint NOTIFY distToWaypointChanged)
    Q_PROPERTY(double airSpeed READ getGroundSpeed WRITE setGroundSpeed NOTIFY airSpeedChanged)
    Q_PROPERTY(double groundSpeed READ getGroundSpeed WRITE setGroundSpeed NOTIFY groundSpeedChanged)
    Q_PROPERTY(double bearingToWaypoint READ getBearingToWaypoint WRITE setBearingToWaypoint NOTIFY bearingToWaypointChanged)
    Q_PROPERTY(double altitudeAMSL READ getAltitudeAMSL WRITE setAltitudeAMSL NOTIFY altitudeAMSLChanged)
    Q_PROPERTY(double altitudeAMSLFT READ getAltitudeAMSLFT NOTIFY altitudeAMSLFTChanged)
    Q_PROPERTY(double altitudeWGS84 READ getAltitudeWGS84 WRITE setAltitudeWGS84 NOTIFY altitudeWGS84Changed)
    Q_PROPERTY(double altitudeRelative READ getAltitudeRelative WRITE setAltitudeRelative NOTIFY altitudeRelativeChanged)

    void setGroundSpeed(double val)
    {
        groundSpeed = val;
        emit groundSpeedChanged(val,"groundSpeed");
        emit valueChanged(this->uasId,"groundSpeed","m/s",QVariant(val),getUnixTime());
    }
    double getGroundSpeed() const
    {
        return groundSpeed;
    }

    void setAirSpeed(double val)
    {
        airSpeed = val;
        emit airSpeedChanged(val,"airSpeed");
        emit valueChanged(this->uasId,"airSpeed","m/s",QVariant(val),getUnixTime());
    }

    double getAirSpeed() const
    {
        return airSpeed;
    }

    void setLocalX(double val)
    {
        localX = val;
        emit localXChanged(val,"localX");
        emit valueChanged(this->uasId,"localX","m",QVariant(val),getUnixTime());
    }

    double getLocalX() const
    {
        return localX;
    }

    void setLocalY(double val)
    {
        localY = val;
        emit localYChanged(val,"localY");
        emit valueChanged(this->uasId,"localY","m",QVariant(val),getUnixTime());
    }
    double getLocalY() const
    {
        return localY;
    }

    void setLocalZ(double val)
    {
        localZ = val;
        emit localZChanged(val,"localZ");
        emit valueChanged(this->uasId,"localZ","m",QVariant(val),getUnixTime());
    }
    double getLocalZ() const
    {
        return localZ;
    }

    void setLatitude(double val)
    {
        latitude = val;
        emit latitudeChanged(val,"latitude");
        emit valueChanged(this->uasId,"latitude","deg",QVariant(val),getUnixTime());
    }

    double getLatitude() const
    {
        return latitude;
    }

    void setLongitude(double val)
    {
        longitude = val;
        emit longitudeChanged(val,"longitude");
        emit valueChanged(this->uasId,"longitude","deg",QVariant(val),getUnixTime());
    }

    double getLongitude() const
    {
        return longitude;
    }

    void setAltitudeAMSL(double val)
    {
        altitudeAMSL = val;
        emit altitudeAMSLChanged(val, "altitudeAMSL");
        emit valueChanged(this->uasId,"altitudeAMSL","m",QVariant(altitudeAMSL),getUnixTime());
        altitudeAMSLFT = 3.28084 * altitudeAMSL;
        emit altitudeAMSLFTChanged(val, "altitudeAMSLFT");
        emit valueChanged(this->uasId,"altitudeAMSLFT","m",QVariant(altitudeAMSLFT),getUnixTime());
    }

    double getAltitudeAMSL() const
    {
        return altitudeAMSL;
    }

    double getAltitudeAMSLFT() const
    {
        return altitudeAMSLFT;
    }

    void setAltitudeWGS84(double val)
    {
        altitudeWGS84 = val;
        emit altitudeWGS84Changed(val, "altitudeWGS84");
        emit valueChanged(this->uasId,"altitudeWGS84","m",QVariant(val),getUnixTime());
    }

    double getAltitudeWGS84() const
    {
        return altitudeWGS84;
    }


    void setAltitudeRelative(double val)
    {
        altitudeRelative = val;
        emit altitudeRelativeChanged(val, "altitudeRelative");
        emit valueChanged(this->uasId,"altitudeRelative","m",QVariant(val),getUnixTime());
    }

    double getAltitudeRelative() const
    {
        return altitudeRelative;
    }

    void setSatelliteCount(double val)
    {
        satelliteCount = val;
        emit satelliteCountChanged(val,"satelliteCount");
        emit valueChanged(this->uasId,"satelliteCount","",QVariant(val),getUnixTime());
    }

    double getSatelliteCount() const
    {
        return satelliteCount;
    }

    virtual bool localPositionKnown() const
    {
        return isLocalPositionKnown;
    }

    virtual bool globalPositionKnown() const
    {
        return isGlobalPositionKnown;
    }

    void setDistToWaypoint(double val)
    {
        distToWaypoint = val;
        emit distToWaypointChanged(val,"distToWaypoint");
        emit valueChanged(this->uasId,"distToWaypoint","m",QVariant(val),getUnixTime());
    }

    double getDistToWaypoint() const
    {
        return distToWaypoint;
    }

    void setBearingToWaypoint(double val)
    {
        bearingToWaypoint = val;
        emit bearingToWaypointChanged(val,"bearingToWaypoint");
        emit valueChanged(this->uasId,"bearingToWaypoint","deg",QVariant(val),getUnixTime());
    }

    double getBearingToWaypoint() const
    {
        return bearingToWaypoint;
    }


    void setRoll(double val)
    {
        roll = val;
        emit rollChanged(val,"roll");
    }

    double getRoll() const
    {
        return roll;
    }

    void setPitch(double val)
    {
        pitch = val;
        emit pitchChanged(val,"pitch");
    }

    double getPitch() const
    {
        return pitch;
    }

    void setYaw(double val)
    {
        yaw = val;
        emit yawChanged(val,"yaw");
    }

    double getYaw() const
    {
        return yaw;
    }

    bool getSelected() const;
    QVector3D getNedPosGlobalOffset() const
    {
        return nedPosGlobalOffset;
    }

    QVector3D getNedAttGlobalOffset() const
    {
        return nedAttGlobalOffset;
    }


    // Setters for HIL noise variance
    void setXaccVar(float var){
        xacc_var = var;
    }

    void setYaccVar(float var){
        yacc_var = var;
    }

    void setZaccVar(float var){
        zacc_var = var;
    }

    void setRollSpeedVar(float var){
        rollspeed_var = var;
    }

    void setPitchSpeedVar(float var){
        pitchspeed_var = var;
    }

    void setYawSpeedVar(float var){
        pitchspeed_var = var;
    }

    void setXmagVar(float var){
        xmag_var = var;
    }

    void setYmagVar(float var){
        ymag_var = var;
    }

    void setZmagVar(float var){
        zmag_var = var;
    }

    void setAbsPressureVar(float var){
        abs_pressure_var = var;
    }

    void setDiffPressureVar(float var){
        diff_pressure_var = var;
    }

    void setPressureAltVar(float var){
        pressure_alt_var = var;
    }

    void setTemperatureVar(float var){
        temperature_var = var;
    }

    bool isRotaryWing();
    bool isFixedWing();

    friend class UASWaypointManager;
    friend class FileManager;

protected: //COMMENTS FOR TEST UNIT
    /// LINK ID AND STATUS
    int uasId;                    ///< Unique system ID
    QMap<int, QString> components;///< IDs and names of all detected onboard components
    
    /// List of all links associated with this UAS. We keep SharedLinkInterface objects which are QSharedPointer's in order to
    /// maintain reference counts across threads. This way Link deletion works correctly.
    QList<SharedLinkInterface> _links;
    
    QList<int> unknownPackets;    ///< Packet IDs which are unknown and have been received
    MAVLinkProtocol* mavlink;     ///< Reference to the MAVLink instance
    float receiveDropRate;        ///< Percentage of packets that were dropped on the MAV's receiving link (from GCS and other MAVs)
    float sendDropRate;           ///< Percentage of packets that were not received from the MAV by the GCS
    quint64 lastHeartbeat;        ///< Time of the last heartbeat message
    QTimer statusTimeout;       ///< Timer for various status timeouts

    /// BASIC UAS TYPE, NAME AND STATE
    QString name;                 ///< Human-friendly name of the vehicle, e.g. bravo
    unsigned char type;           ///< UAS type (from type enum)
    int airframe;                 ///< The airframe type
    int autopilot;                ///< Type of the Autopilot: -1: None, 0: Generic, 1: PIXHAWK, 2: SLUGS, 3: Ardupilot (up to 15 types), defined in MAV_AUTOPILOT_TYPE ENUM
    bool systemIsArmed;           ///< If the system is armed
    uint8_t base_mode;                 ///< The current mode of the MAV
    uint32_t custom_mode;         ///< The current mode of the MAV
    int status;                   ///< The current status of the MAV
    QString shortModeText;        ///< Short textual mode description
    QString shortStateText;       ///< Short textual state description

    /// OUTPUT
    QList<double> actuatorValues;
    QList<QString> actuatorNames;
    QList<double> motorValues;
    QList<QString> motorNames;
    double thrustSum;           ///< Sum of forward/up thrust of all thrust actuators, in Newtons
    double thrustMax;           ///< Maximum forward/up thrust of this vehicle, in Newtons

    // dongfang: This looks like a candidate for being moved off to a separate class.
    /// BATTERY / ENERGY
    float startVoltage;         ///< Voltage at system start
    float tickVoltage;          ///< Voltage where 0.1 V ticks are told
    float lastTickVoltageValue; ///< The last voltage where a tick was announced
    float tickLowpassVoltage;   ///< Lowpass-filtered voltage for the tick announcement
    float warnLevelPercent;     ///< Warning level, in percent
    double currentVoltage;      ///< Voltage currently measured
    float lpVoltage;            ///< Low-pass filtered voltage
    double currentCurrent;      ///< Battery current currently measured
    bool batteryRemainingEstimateEnabled; ///< If the estimate is enabled, QGC will try to estimate the remaining battery life
    float chargeLevel;          ///< Charge level of battery, in percent
    int timeRemaining;          ///< Remaining time calculated based on previous and current
    bool lowBattAlarm;          ///< Switch if battery is low


    /// TIMEKEEPING
    quint64 startTime;            ///< The time the UAS was switched on
    quint64 onboardTimeOffset;

    /// MANUAL CONTROL
    bool controlRollManual;     ///< status flag, true if roll is controlled manually
    bool controlPitchManual;    ///< status flag, true if pitch is controlled manually
    bool controlYawManual;      ///< status flag, true if yaw is controlled manually
    bool controlThrustManual;   ///< status flag, true if thrust is controlled manually

    double manualRollAngle;     ///< Roll angle set by human pilot (radians)
    double manualPitchAngle;    ///< Pitch angle set by human pilot (radians)
    double manualYawAngle;      ///< Yaw angle set by human pilot (radians)
    double manualThrust;        ///< Thrust set by human pilot (radians)

    /// POSITION
    bool positionLock;          ///< Status if position information is available or not
    bool isLocalPositionKnown;  ///< If the local position has been received for this MAV
    bool isGlobalPositionKnown; ///< If the global position has been received for this MAV

    double localX;
    double localY;
    double localZ;

    double latitude;            ///< Global latitude as estimated by position estimator
    double longitude;           ///< Global longitude as estimated by position estimator
    double altitudeAMSL;        ///< Global altitude as estimated by position estimator, AMSL
    double altitudeAMSLFT;        ///< Global altitude as estimated by position estimator, AMSL
    double altitudeWGS84;        ///< Global altitude as estimated by position estimator, WGS84
    double altitudeRelative;    ///< Altitude above home as estimated by position estimator

    double satelliteCount;      ///< Number of satellites visible to raw GPS
    bool globalEstimatorActive; ///< Global position estimator present, do not fall back to GPS raw for position
    double latitude_gps;        ///< Global latitude as estimated by raw GPS
    double longitude_gps;       ///< Global longitude as estimated by raw GPS
    double altitude_gps;        ///< Global altitude as estimated by raw GPS
    double speedX;              ///< True speed in X axis
    double speedY;              ///< True speed in Y axis
    double speedZ;              ///< True speed in Z axis

    QVector3D nedPosGlobalOffset;   ///< Offset between the system's NED position measurements and the swarm / global 0/0/0 origin
    QVector3D nedAttGlobalOffset;   ///< Offset between the system's NED position measurements and the swarm / global 0/0/0 origin

    /// WAYPOINT NAVIGATION
    double distToWaypoint;       ///< Distance to next waypoint
    double airSpeed;             ///< Airspeed
    double groundSpeed;          ///< Groundspeed
    double bearingToWaypoint;    ///< Bearing to next waypoint
    UASWaypointManager waypointManager;
    FileManager   fileManager;

    /// ATTITUDE
    bool attitudeKnown;             ///< True if attitude was received, false else
    bool attitudeStamped;           ///< Should arriving data be timestamped with the last attitude? This helps with broken system time clocks on the MAV
    quint64 lastAttitude;           ///< Timestamp of last attitude measurement
    double roll;
    double pitch;
    double yaw;

    // dongfang: This looks like a candidate for being moved off to a separate class.
    /// IMAGING
    int imageSize;              ///< Image size being transmitted (bytes)
    int imagePackets;           ///< Number of data packets being sent for this image
    int imagePacketsArrived;    ///< Number of data packets recieved
    int imagePayload;           ///< Payload size per transmitted packet (bytes). Standard is 254, and decreases when image resolution increases.
    int imageQuality;           ///< Quality of the transmitted image (percentage)
    int imageType;              ///< Type of the transmitted image (BMP, PNG, JPEG, RAW 8 bit, RAW 32 bit)
    int imageWidth;             ///< Width of the image stream
    int imageHeight;            ///< Width of the image stream
    QByteArray imageRecBuffer;  ///< Buffer for the incoming bytestream
    QImage image;               ///< Image data of last completely transmitted image
    quint64 imageStart;
    bool blockHomePositionChanges;   ///< Block changes to the home position
    bool receivedMode;          ///< True if mode was retrieved from current conenction to UAS

    /// SIMULATION NOISE
    float xacc_var;             ///< variance of x acclerometer noise for HIL sim (mg)
    float yacc_var;             ///< variance of y acclerometer noise for HIL sim (mg)
    float zacc_var;             ///< variance of z acclerometer noise for HIL sim (mg)
    float rollspeed_var;        ///< variance of x gyroscope noise for HIL sim (rad/s)
    float pitchspeed_var;       ///< variance of y gyroscope noise for HIL sim (rad/s)
    float yawspeed_var;         ///< variance of z gyroscope noise for HIL sim (rad/s)
    float xmag_var;             ///< variance of x magnatometer noise for HIL sim (???)
    float ymag_var;             ///< variance of y magnatometer noise for HIL sim (???)
    float zmag_var;             ///< variance of z magnatometer noise for HIL sim (???)
    float abs_pressure_var;     ///< variance of absolute pressure noise for HIL sim (hPa)
    float diff_pressure_var;    ///< variance of differential pressure noise for HIL sim (hPa)
    float pressure_alt_var;     ///< variance of altitude pressure noise for HIL sim (hPa)
    float temperature_var;      ///< variance of temperature noise for HIL sim (C)

    /// SIMULATION
#ifndef __mobile__
    QGCHilLink* simulation;         ///< Hardware in the loop simulation link
#endif

public:
    /** @brief Set the current battery type */
    void setBattery(BatteryType type, int cells);
    /** @brief Estimate how much flight time is remaining */
    int calculateTimeRemaining();
    /** @brief Get the current charge level */
    float getChargeLevel();
    /** @brief Get the human-readable status message for this code */
    void getStatusForCode(int statusCode, QString& uasState, QString& stateDescription);
    /** @brief Check if vehicle is in autonomous mode */
    bool isAuto();
    /** @brief Check if vehicle is armed */
    bool isArmed() const { return systemIsArmed; }
    /** @brief Check if vehicle is supposed to be in HIL mode by the GS */
    bool isHilEnabled() const { return hilEnabled; }
    /** @brief Check if vehicle is in HIL mode */
    bool isHilActive() const { return base_mode & MAV_MODE_FLAG_HIL_ENABLED; }

    /** @brief Get reference to the waypoint manager **/
    UASWaypointManager* getWaypointManager() {
        return &waypointManager;
    }

    virtual FileManager* getFileManager() {
        return &fileManager;
    }

    /** @brief Get the HIL simulation */
#ifndef __mobile__
    QGCHilLink* getHILSimulation() const {
        return simulation;
    }
#endif

    int  getSystemType();
    bool isAirplane();

    /**
     * @brief Returns true for systems that can reverse. If the system has no control over position, it returns false as
     * @return If the specified vehicle type can
     */
    bool systemCanReverse() const
    {
        switch(type)
        {
        case MAV_TYPE_GENERIC:
        case MAV_TYPE_FIXED_WING:
        case MAV_TYPE_ROCKET:
        case MAV_TYPE_FLAPPING_WING:

        // System types that don't have movement
        case MAV_TYPE_ANTENNA_TRACKER:
        case MAV_TYPE_GCS:
        case MAV_TYPE_FREE_BALLOON:
        default:
            return false;
        case MAV_TYPE_QUADROTOR:
        case MAV_TYPE_COAXIAL:
        case MAV_TYPE_HELICOPTER:
        case MAV_TYPE_AIRSHIP:
        case MAV_TYPE_GROUND_ROVER:
        case MAV_TYPE_SURFACE_BOAT:
        case MAV_TYPE_SUBMARINE:
        case MAV_TYPE_HEXAROTOR:
        case MAV_TYPE_OCTOROTOR:
        case MAV_TYPE_TRICOPTER:
            return true;
        }
    }

    QString getSystemTypeName()
    {
        switch(type)
        {
        case MAV_TYPE_GENERIC:
            return "GENERIC";
            break;
        case MAV_TYPE_FIXED_WING:
            return "FIXED_WING";
            break;
        case MAV_TYPE_QUADROTOR:
            return "QUADROTOR";
            break;
        case MAV_TYPE_COAXIAL:
            return "COAXIAL";
            break;
        case MAV_TYPE_HELICOPTER:
            return "HELICOPTER";
            break;
        case MAV_TYPE_ANTENNA_TRACKER:
            return "ANTENNA_TRACKER";
            break;
        case MAV_TYPE_GCS:
            return "GCS";
            break;
        case MAV_TYPE_AIRSHIP:
            return "AIRSHIP";
            break;
        case MAV_TYPE_FREE_BALLOON:
            return "FREE_BALLOON";
            break;
        case MAV_TYPE_ROCKET:
            return "ROCKET";
            break;
        case MAV_TYPE_GROUND_ROVER:
            return "GROUND_ROVER";
            break;
        case MAV_TYPE_SURFACE_BOAT:
            return "BOAT";
            break;
        case MAV_TYPE_SUBMARINE:
            return "SUBMARINE";
            break;
        case MAV_TYPE_HEXAROTOR:
            return "HEXAROTOR";
            break;
        case MAV_TYPE_OCTOROTOR:
            return "OCTOROTOR";
            break;
        case MAV_TYPE_TRICOPTER:
            return "TRICOPTER";
            break;
        case MAV_TYPE_FLAPPING_WING:
            return "FLAPPING_WING";
            break;
        default:
            return "";
            break;
        }
    }

    QImage getImage();
    void requestImage();
    int getAutopilotType(){
        return autopilot;
    }
    QString getAutopilotTypeName()
    {
        switch (autopilot)
        {
        case MAV_AUTOPILOT_GENERIC:
            return "GENERIC";
            break;
        case MAV_AUTOPILOT_SLUGS:
            return "SLUGS";
            break;
        case MAV_AUTOPILOT_ARDUPILOTMEGA:
            return "ARDUPILOTMEGA";
            break;
        case MAV_AUTOPILOT_OPENPILOT:
            return "OPENPILOT";
            break;
        case MAV_AUTOPILOT_GENERIC_WAYPOINTS_ONLY:
            return "GENERIC_WAYPOINTS_ONLY";
            break;
        case MAV_AUTOPILOT_GENERIC_WAYPOINTS_AND_SIMPLE_NAVIGATION_ONLY:
            return "GENERIC_MISSION_NAVIGATION_ONLY";
            break;
        case MAV_AUTOPILOT_GENERIC_MISSION_FULL:
            return "GENERIC_MISSION_FULL";
            break;
        case MAV_AUTOPILOT_INVALID:
            return "NO AP";
            break;
        case MAV_AUTOPILOT_PPZ:
            return "PPZ";
            break;
        case MAV_AUTOPILOT_UDB:
            return "UDB";
            break;
        case MAV_AUTOPILOT_FP:
            return "FP";
            break;
        case MAV_AUTOPILOT_PX4:
            return "PX4";
            break;
        default:
            return "UNKNOWN";
            break;
        }
    }
    /** From UASInterface */
    QList<QAction*> getActions() const
    {
        return actions;
    }

public slots:
    /** @brief Set the autopilot type */
    void setAutopilotType(int apType)
    {
        autopilot = apType;
        emit systemSpecsChanged(uasId);
    }
    /** @brief Set the type of airframe */
    void setSystemType(int systemType);
    /** @brief Set the specific airframe type */
    void setAirframe(int airframe)
    {
        if((airframe >= QGC_AIRFRAME_GENERIC) && (airframe < QGC_AIRFRAME_END_OF_ENUM))
        {
          this->airframe = airframe;
          emit systemSpecsChanged(uasId);
        }

    }
    /** @brief Set a new name **/
    void setUASName(const QString& name);
    /** @brief Executes a command **/
    void executeCommand(MAV_CMD command);
    /** @brief Executes a command with 7 params */
    void executeCommand(MAV_CMD command, int confirmation, float param1, float param2, float param3, float param4, float param5, float param6, float param7, int component);
    /** @brief Executes a command ack, with success boolean **/
    void executeCommandAck(int num, bool success);
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
    /** @brief Order the robot to return home **/
    void home();
    /** @brief Order the robot to land **/
    void land();
    /** @brief Order the robot to pair its receiver **/
    void pairRX(int rxType, int rxSubType);

    void halt();
    void go();

    /** @brief Enable / disable HIL */
#ifndef __mobile__
    void enableHilFlightGear(bool enable, QString options, bool sensorHil, QObject * configuration);
    void enableHilJSBSim(bool enable, QString options);
    void enableHilXPlane(bool enable);

    /** @brief Send the full HIL state to the MAV */
    void sendHilState(quint64 time_us, float roll, float pitch, float yaw, float rollRotationRate,
                        float pitchRotationRate, float yawRotationRate, double lat, double lon, double alt,
                        float vx, float vy, float vz, float ind_airspeed, float true_airspeed, float xacc, float yacc, float zacc);

    void sendHilGroundTruth(quint64 time_us, float roll, float pitch, float yaw, float rollRotationRate,
                        float pitchRotationRate, float yawRotationRate, double lat, double lon, double alt,
                        float vx, float vy, float vz, float ind_airspeed, float true_airspeed, float xacc, float yacc, float zacc);

    /** @brief RAW sensors for sensor HIL */
    void sendHilSensors(quint64 time_us, float xacc, float yacc, float zacc, float rollspeed, float pitchspeed, float yawspeed,
                        float xmag, float ymag, float zmag, float abs_pressure, float diff_pressure, float pressure_alt, float temperature, quint32 fields_changed);

    /** @brief Send Optical Flow sensor message for HIL, (arguments and units accoding to mavlink documentation*/
    void sendHilOpticalFlow(quint64 time_us, qint16 flow_x, qint16 flow_y, float flow_comp_m_x,
                            float flow_comp_m_y, quint8 quality, float ground_distance);

    float addZeroMeanNoise(float truth_meas, float noise_var);

    /**
     * @param time_us
     * @param lat
     * @param lon
     * @param alt
     * @param fix_type
     * @param eph
     * @param epv
     * @param vel
     * @param cog course over ground, in radians, -pi..pi
     * @param satellites
     */
    void sendHilGps(quint64 time_us, double lat, double lon, double alt, int fix_type, float eph, float epv, float vel, float vn, float ve, float vd,  float cog, int satellites);


    /** @brief Places the UAV in Hardware-in-the-Loop simulation status **/
    void startHil();

    /** @brief Stops the UAV's Hardware-in-the-Loop simulation status **/
    void stopHil();
#endif

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

    /** @brief Arm system */
    void armSystem();
    /** @brief Disable the motors */
    void disarmSystem();
    /** @brief Toggle the armed state of the system. */
    void toggleArmedState();
    /**
     * @brief Tell the UAS to switch into a completely-autonomous mode, so disable manual input.
     */
    void goAutonomous();
    /**
     * @brief Tell the UAS to switch to manual control. Stabilized attitude may simultaneously be engaged.
     */
    void goManual();
    /**
     * @brief Tell the UAS to switch between manual and autonomous control.
     */
    void toggleAutonomy();

    /** @brief Set the values for the manual control of the vehicle */
#ifndef __mobile__
    void setExternalControlSetpoint(float roll, float pitch, float yaw, float thrust, qint8 xHat, qint8 yHat, quint16 buttons, quint8);
#endif

    /** @brief Set the values for the 6dof manual control of the vehicle */
#ifndef __mobile__
    void setManual6DOFControlCommands(double x, double y, double z, double roll, double pitch, double yaw);
#endif

    /** @brief Add a link associated with this robot */
    void addLink(LinkInterface* link);

    /** @brief Receive a message from one of the communication links. */
    virtual void receiveMessage(LinkInterface* link, mavlink_message_t message);

#ifdef QGC_PROTOBUF_ENABLED
    /** @brief Receive a message from one of the communication links. */
    virtual void receiveExtendedMessage(LinkInterface* link, std::tr1::shared_ptr<google::protobuf::Message> message);
#endif

    /** @brief Send a message over this link (to this or to all UAS on this link) */
    void sendMessage(LinkInterface* link, mavlink_message_t message);
    /** @brief Send a message over all links this UAS can be reached with (!= all links) */
    void sendMessage(mavlink_message_t message);

    /** @brief Set this UAS as the system currently in focus, e.g. in the main display widgets */
    void setSelected();

    /** @brief Set current mode of operation, e.g. auto or manual, always uses the current arming status for safety reason */
    void setMode(uint8_t newBaseMode, uint32_t newCustomMode);

    /** @brief Set current mode of operation, e.g. auto or manual, does not check the arming status, for anything else than arming/disarming operations use setMode instead */
    void setModeArm(uint8_t newBaseMode, uint32_t newCustomMode);

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

    void startCalibration(StartCalibrationType calType);
    void stopCalibration(void);

    void startBusConfig(StartBusConfigType calType);
    void stopBusConfig(void);

    void startDataRecording();
    void stopDataRecording();
    void deleteSettings();

    /** @brief Triggers the action associated with the given ID. */
    void triggerAction(int action);

    /** @brief Send command to map a RC channel to a parameter */
    void sendMapRCToParam(QString param_id, float scale, float value0, quint8 param_rc_channel_index, float valueMin, float valueMax);

    /** @brief Send command to disable all bindings/maps between RC and parameters */
    void unsetRCToParameterMap();
signals:
    /** @brief The main/battery voltage has changed/was updated */
    //void voltageChanged(int uasId, double voltage); // Defined in UASInterface already
    /** @brief An actuator value has changed */
    //void actuatorChanged(UASInterface*, int actId, double value); // Defined in UASInterface already
    /** @brief An actuator value has changed */
    void actuatorChanged(UASInterface* uas, QString actuatorName, double min, double max, double value);
    void motorChanged(UASInterface* uas, QString motorName, double min, double max, double value);
    /** @brief The system load (MCU/CPU usage) changed */
    void loadChanged(UASInterface* uas, double load);
    /** @brief Propagate a heartbeat received from the system */
    //void heartbeat(UASInterface* uas); // Defined in UASInterface already
    void imageStarted(quint64 timestamp);
    /** @brief A new camera image has arrived */
    void imageReady(UASInterface* uas);
    /** @brief HIL controls have changed */
    void hilControlsChanged(quint64 time, float rollAilerons, float pitchElevator, float yawRudder, float throttle, quint8 systemMode, quint8 navMode);
    /** @brief HIL actuator outputs have changed */
    void hilActuatorsChanged(quint64 time, float act1, float act2, float act3, float act4, float act5, float act6, float act7, float act8);

    void localXChanged(double val,QString name);
    void localYChanged(double val,QString name);
    void localZChanged(double val,QString name);
    void longitudeChanged(double val,QString name);
    void latitudeChanged(double val,QString name);
    void altitudeAMSLChanged(double val,QString name);
    void altitudeAMSLFTChanged(double val,QString name);
    void altitudeWGS84Changed(double val,QString name);
    void altitudeRelativeChanged(double val,QString name);
    void rollChanged(double val,QString name);
    void pitchChanged(double val,QString name);
    void yawChanged(double val,QString name);
    void satelliteCountChanged(double val,QString name);
    void distToWaypointChanged(double val,QString name);
    void groundSpeedChanged(double val, QString name);
    void airSpeedChanged(double val, QString name);
    void bearingToWaypointChanged(double val,QString name);
    void _sendMessageOnThread(mavlink_message_t message);
    void _sendMessageOnThreadLink(LinkInterface* link, mavlink_message_t message);
protected:
    /** @brief Get the UNIX timestamp in milliseconds, enter microseconds */
    quint64 getUnixTime(quint64 time=0);
    /** @brief Get the UNIX timestamp in milliseconds, enter milliseconds */
    quint64 getUnixTimeFromMs(quint64 time);
    /** @brief Get the UNIX timestamp in milliseconds, ignore attitudeStamped mode */
    quint64 getUnixReferenceTime(quint64 time);

    virtual void processParamValueMsg(mavlink_message_t& msg, const QString& paramName,const mavlink_param_value_t& rawValue, mavlink_param_union_t& paramValue);

    int componentID[256];
    bool componentMulti[256];
    bool connectionLost; ///< Flag indicates a timed out connection
    quint64 connectionLossTime; ///< Time the connection was interrupted
    quint64 lastVoltageWarning; ///< Time at which the last voltage warning occured
    quint64 lastNonNullTime;    ///< The last timestamp from the MAV that was not null
    unsigned int onboardTimeOffsetInvalidCount;     ///< Count when the offboard time offset estimation seemed wrong
    bool hilEnabled;            ///< Set to true if HIL mode is enabled from GCS (UAS might be in HIL even if this flag is not set, this defines the GCS HIL setting)
    bool sensorHil;             ///< True if sensor HIL is enabled
    quint64 lastSendTimeGPS;     ///< Last HIL GPS message sent
    quint64 lastSendTimeSensors; ///< Last HIL Sensors message sent
    quint64 lastSendTimeOpticalFlow; ///< Last HIL Optical Flow message sent
    QList<QAction*> actions; ///< A list of actions that this UAS can perform.


protected slots:
    /** @brief Write settings to disk */
    void writeSettings();
    /** @brief Read settings from disk */
    void readSettings();
    /** @brief Send a message over this link (to this or to all UAS on this link) */
    void _sendMessageLink(LinkInterface* link, mavlink_message_t message);
    /** @brief Send a message over all links this UAS can be reached with (!= all links) */
    void _sendMessage(mavlink_message_t message);
    
private slots:
    void _linkDisconnected(LinkInterface* link);
    
private:
    bool _containsLink(LinkInterface* link);
};


#endif // _UAS_H_
