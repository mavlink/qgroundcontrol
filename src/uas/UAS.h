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
#include "QGCHilLink.h"
#include "QGCFlightGearLink.h"
#include "QGCJSBSimLink.h"
#include "QGCXPlaneLink.h"


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

    float lipoFull;  ///< 100% charged voltage
    float lipoEmpty; ///< Discharged voltage

    /* MANAGEMENT */

    /** @brief The name of the robot */
    QString getUASName(void) const;
    /** @brief Get short state */
    const QString& getShortState() const;
    /** @brief Get short mode */
    const QString& getShortMode() const;
    /** @brief Translate from mode id to text */
    static QString getShortModeTextFor(uint8_t base_mode, uint32_t custom_mode, int autopilot);
    /** @brief Translate from mode id to audio text */
    static QString getAudioModeTextFor(int id);
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
    /** @brief Get the status flag for the communication */
    int getCommunicationStatus() const;
    /** @brief Add one measurement and get low-passed voltage */
    float filterVoltage(float value) const;
    /** @brief Get the links associated with this robot */
    QList<LinkInterface*>* getLinks();

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
    Q_PROPERTY(double groundSpeed READ getGroundSpeed WRITE setGroundSpeed NOTIFY groundSpeedChanged)

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
    Q_PROPERTY(double bearingToWaypoint READ getBearingToWaypoint WRITE setBearingToWaypoint NOTIFY bearingToWaypointChanged)

    // dongfang: There is not only one altitude; there are at least (APM) GPS altitude, mix altitude and mix-altitude relative to home.
    // I have made this property refer to the mix-altitude ASL as this is the one actually used in navigation by APM.
    Q_PROPERTY(double altitude READ getAltitude WRITE setAltitude NOTIFY altitudeChanged)

    void setLocalX(double val)
    {
        localX = val;
        emit localXChanged(val,"localX");
        emit valueChanged(this->uasId,"localX","M",QVariant(val),getUnixTime());
    }

    double getLocalX() const
    {
        return localX;
    }

    void setLocalY(double val)
    {
        localY = val;
        emit localYChanged(val,"localY");
        emit valueChanged(this->uasId,"localY","M",QVariant(val),getUnixTime());
    }
    double getLocalY() const
    {
        return localY;
    }

    void setLocalZ(double val)
    {
        localZ = val;
        emit localZChanged(val,"localZ");
        emit valueChanged(this->uasId,"localZ","M",QVariant(val),getUnixTime());
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

    void setAltitude(double val)
    {
        altitude = val;
        emit altitudeChanged(val, "altitude");
        emit valueChanged(this->uasId,"altitude","M",QVariant(val),getUnixTime());
    }

    double getAltitude() const
    {
        return altitude;
    }

    void setSatelliteCount(double val)
    {
        satelliteCount = val;
        emit satelliteCountChanged(val,"satelliteCount");
        emit valueChanged(this->uasId,"satelliteCount","M",QVariant(val),getUnixTime());
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
        emit valueChanged(this->uasId,"distToWaypoint","M",QVariant(val),getUnixTime());
    }

    double getDistToWaypoint() const
    {
        return distToWaypoint;
    }

    void setBearingToWaypoint(double val)
    {
        bearingToWaypoint = val;
        emit bearingToWaypointChanged(val,"bearingToWaypoint");
        emit valueChanged(this->uasId,"bearingToWaypoint","M",QVariant(val),getUnixTime());
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

#if defined(QGC_PROTOBUF_ENABLED) && defined(QGC_USE_PIXHAWK_MESSAGES)
    px::GLOverlay getOverlay()
    {
        QMutexLocker locker(&overlayMutex);
        return overlay;
    }

    px::GLOverlay getOverlay(qreal& receivedTimestamp)
    {
        receivedTimestamp = receivedOverlayTimestamp;
        QMutexLocker locker(&overlayMutex);
        return overlay;
    }

    px::ObstacleList getObstacleList() {
        QMutexLocker locker(&obstacleListMutex);
        return obstacleList;
    }

    px::ObstacleList getObstacleList(qreal& receivedTimestamp) {
        receivedTimestamp = receivedObstacleListTimestamp;
        QMutexLocker locker(&obstacleListMutex);
        return obstacleList;
    }

    px::Path getPath() {
        QMutexLocker locker(&pathMutex);
        return path;
    }

    px::Path getPath(qreal& receivedTimestamp) {
        receivedTimestamp = receivedPathTimestamp;
        QMutexLocker locker(&pathMutex);
        return path;
    }

    px::PointCloudXYZRGB getPointCloud() {
        QMutexLocker locker(&pointCloudMutex);
        return pointCloud;
    }

    px::PointCloudXYZRGB getPointCloud(qreal& receivedTimestamp) {
        receivedTimestamp = receivedPointCloudTimestamp;
        QMutexLocker locker(&pointCloudMutex);
        return pointCloud;
    }

    px::RGBDImage getRGBDImage() {
        QMutexLocker locker(&rgbdImageMutex);
        return rgbdImage;
    }

    px::RGBDImage getRGBDImage(qreal& receivedTimestamp) {
        receivedTimestamp = receivedRGBDImageTimestamp;
        QMutexLocker locker(&rgbdImageMutex);
        return rgbdImage;
    }
#endif

    friend class UASWaypointManager;

protected: //COMMENTS FOR TEST UNIT
    /// LINK ID AND STATUS
    int uasId;                    ///< Unique system ID
    QMap<int, QString> components;///< IDs and names of all detected onboard components
    QList<LinkInterface*>* links; ///< List of links this UAS can be reached by
    QList<int> unknownPackets;    ///< Packet IDs which are unknown and have been received
    MAVLinkProtocol* mavlink;     ///< Reference to the MAVLink instance
    CommStatus commStatus;        ///< Communication status
    float receiveDropRate;        ///< Percentage of packets that were dropped on the MAV's receiving link (from GCS and other MAVs)
    float sendDropRate;           ///< Percentage of packets that were not received from the MAV by the GCS
    quint64 lastHeartbeat;        ///< Time of the last heartbeat message
    QTimer* statusTimeout;        ///< Timer for various status timeouts

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
    BatteryType batteryType;    ///< The battery type
    int cells;                  ///< Number of cells
    float fullVoltage;          ///< Voltage of the fully charged battery (100%)
    float emptyVoltage;         ///< Voltage of the empty battery (0%)
    float startVoltage;         ///< Voltage at system start
    float tickVoltage;          ///< Voltage where 0.1 V ticks are told
    float lastTickVoltageValue; ///< The last voltage where a tick was announced
    float tickLowpassVoltage;   ///< Lowpass-filtered voltage for the tick announcement
    float warnVoltage;          ///< Voltage where QGC will start to warn about low battery
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
    bool isLocalPositionKnown;      ///< If the local position has been received for this MAV
    bool isGlobalPositionKnown;     ///< If the global position has been received for this MAV

    double localX;
    double localY;
    double localZ;

    double latitude;            ///< Global latitude as estimated by position estimator
    double longitude;           ///< Global longitude as estimated by position estimator
    double altitude;            ///< Global altitude as estimated by position estimator

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
    double groundSpeed;         ///< GPS Groundspeed
    double bearingToWaypoint;    ///< Bearing to next waypoint
    UASWaypointManager waypointManager;

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

#if defined(QGC_PROTOBUF_ENABLED) && defined(QGC_USE_PIXHAWK_MESSAGES)
    px::GLOverlay overlay;
    QMutex overlayMutex;
    qreal receivedOverlayTimestamp;

    px::ObstacleList obstacleList;
    QMutex obstacleListMutex;
    qreal receivedObstacleListTimestamp;

    px::Path path;
    QMutex pathMutex;
    qreal receivedPathTimestamp;

    px::PointCloudXYZRGB pointCloud;
    QMutex pointCloudMutex;
    qreal receivedPointCloudTimestamp;

    px::RGBDImage rgbdImage;
    QMutex rgbdImageMutex;
    qreal receivedRGBDImageTimestamp;
#endif

    /// PARAMETERS
    QMap<int, QMap<QString, QVariant>* > parameters; ///< All parameters
    bool paramsOnceRequested;       ///< If the parameter list has been read at least once
    QGCUASParamManager paramMgr; ///< Parameter manager for this UAS

    /// SIMULATION
    QGCHilLink* simulation;         ///< Hardware in the loop simulation link

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

    /** @brief Get reference to the waypoint manager **/
    UASWaypointManager* getWaypointManager() {
        return &waypointManager;
    }

    /** @brief Get reference to the param manager **/
    virtual QGCUASParamManager* getParamManager()  {
        return &paramMgr;
    }

    /** @brief Get the HIL simulation */
    QGCHilLink* getHILSimulation() const {
        return simulation;
    }


    int getSystemType();

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
        case MAV_AUTOPILOT_PIXHAWK:
            return "PIXHAWK";
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
            return "";
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
    void enableHilFlightGear(bool enable, QString options, bool sensorHil);
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
    void setManualControlCommands(double roll, double pitch, double yaw, double thrust, int xHat, int yHat, int buttons);

    /** @brief Set the values for the 6dof manual control of the vehicle */
    void setManual6DOFControlCommands(double x, double y, double z, double roll, double pitch, double yaw);

    /** @brief Add a link associated with this robot */
    void addLink(LinkInterface* link);
    /** @brief Remove a link associated with this robot */
    void removeLink(QObject* object);

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

    /** @brief Temporary Hack for sending packets to patch Antenna. Send a message over all serial links except for this UAS's */
    void forwardMessage(mavlink_message_t message);

    /** @brief Set this UAS as the system currently in focus, e.g. in the main display widgets */
    void setSelected();

    /** @brief Set current mode of operation, e.g. auto or manual */
    void setMode(uint8_t newBaseMode, uint32_t newCustomMode);

    /** @brief Request all parameters */
    void requestParameters();

    /** @brief Request a single parameter by name */
    void requestParameter(int component, const QString& parameter);
    /** @brief Request a single parameter by index */
    void requestParameter(int component, int id);

    /** @brief Set a system parameter */
    void setParameter(const int compId, const QString& paramId, const QVariant& value);

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
    void stopDataRecording();
    void deleteSettings();

    /** @brief Triggers the action associated with the given ID. */
    void triggerAction(int action);
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
    void hilControlsChanged(uint64_t time, float rollAilerons, float pitchElevator, float yawRudder, float throttle, uint8_t systemMode, uint8_t navMode);
    /** @brief HIL actuator outputs have changed */
    void hilActuatorsChanged(uint64_t time, float act1, float act2, float act3, float act4, float act5, float act6, float act7, float act8);

    void localXChanged(double val,QString name);
    void localYChanged(double val,QString name);
    void localZChanged(double val,QString name);
    void longitudeChanged(double val,QString name);
    void latitudeChanged(double val,QString name);
    void altitudeChanged(double val,QString name);
    void rollChanged(double val,QString name);
    void pitchChanged(double val,QString name);
    void yawChanged(double val,QString name);
    void satelliteCountChanged(double val,QString name);
    void distToWaypointChanged(double val,QString name);
    void groundSpeedChanged(double val, QString name);
    void bearingToWaypointChanged(double val,QString name);

    //void primaryAltitudeChanged(UASInterface*, double altitude, quint64 usec);
    //void gpsAltitudeChanged(UASInterface*, double altitude, quint64 usec);
    //void velocityChanged_NED(UASInterface*, double vx, double vy, double vz, quint64 usec);
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
    quint64 lastSendTimeSensors;
    QList<QAction*> actions; ///< A list of actions that this UAS can perform.


protected slots:
    /** @brief Write settings to disk */
    void writeSettings();
    /** @brief Read settings from disk */
    void readSettings();

//    // MESSAGE RECEPTION
//    /** @brief Receive a named value message */
//    void receiveMessageNamedValue(const mavlink_message_t& message);

private:
//    unsigned int mode;          ///< The current mode of the MAV
};


#endif // _UAS_H_
