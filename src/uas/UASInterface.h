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
 *   @brief Abstract interface, represents one unmanned aerial vehicle
 *
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#ifndef _UASINTERFACE_H_
#define _UASINTERFACE_H_

#include <QObject>
#include <QList>
#include <QAction>
#include <QColor>
#include <QPointer>

#include "LinkInterface.h"
#include "ProtocolInterface.h"
#include "UASWaypointManager.h"

class FileManager;

enum BatteryType
{
    NICD = 0,
    NIMH = 1,
    LIION = 2,
    LIPOLY = 3,
    LIFE = 4,
    AGZN = 5
}; ///< The type of battery used

/*
enum SpeedMeasurementSource
{
    PRIMARY_SPEED = 0,          // ArduPlane: Measured airspeed or estimated airspeed. ArduCopter: Ground (XY) speed.
    GROUNDSPEED_BY_UAV = 1,     // Ground speed as reported by UAS
    GROUNDSPEED_BY_GPS = 2,     // Ground speed as calculated from received GPS velocity data
    LOCAL_SPEED = 3
}; ///< For velocity data, the data source

enum AltitudeMeasurementSource
{
    PRIMARY_ALTITUDE = 0,                  // ArduPlane: air and ground speed mix. This is the altitude used for navigastion.
    BAROMETRIC_ALTITUDE = 1,               // Altitude is pressure altitude. Ardupilot reports no altitude purely by barometer,
                                           // however when ALT_MIX==1, mix-altitude is purely barometric.
    GPS_ALTITUDE = 2                       // GPS ASL altitude
}; ///< For altitude data, the data source

// TODO!!! The different frames are probably represented elsewhere. There should really only
// be one set of frames. We also need to keep track of the home alt. somehow.
enum AltitudeMeasurementFrame
{
    ABSOLUTE = 0,               // Altitude is pressure altitude
    ABOVE_HOME_POSITION = 1
}; ///< For altitude data, a reference frame
*/

/**
 * @brief Interface for all robots.
 *
 * This interface is abstract and thus cannot be instantiated. It serves only as type definition.
 * It represents an unmanned aerial vehicle, e.g. a micro air vehicle.
 **/
class UASInterface : public QObject
{
    Q_OBJECT
public:
    virtual ~UASInterface() {}

    /* MANAGEMENT */

    /** @brief The name of the robot **/
    virtual QString getUASName() const = 0;
    /** @brief Get short state */
    virtual const QString& getShortState() const = 0;
    /** @brief Get short mode */
    virtual const QString& getShortMode() const = 0;
    //virtual QColor getColor() = 0;
    virtual int getUASID() const = 0; ///< Get the ID of the connected UAS
    /** @brief The time interval the robot is switched on **/
    virtual quint64 getUptime() const = 0;

    virtual double getLocalX() const = 0;
    virtual double getLocalY() const = 0;
    virtual double getLocalZ() const = 0;
    virtual bool localPositionKnown() const = 0;

    virtual double getLatitude() const = 0;
    virtual double getLongitude() const = 0;
    virtual double getAltitudeAMSL() const = 0;
    virtual double getAltitudeRelative() const = 0;
    virtual bool globalPositionKnown() const = 0;

    virtual double getRoll() const = 0;
    virtual double getPitch() const = 0;
    virtual double getYaw() const = 0;

    virtual bool getSelected() const = 0;

    virtual bool isArmed() const = 0;

    /** @brief Set the airframe of this MAV */
    virtual int getAirframe() const = 0;

    /** @brief Get reference to the waypoint manager **/
    virtual UASWaypointManager* getWaypointManager(void) = 0;

    virtual FileManager* getFileManager() = 0;

    /** @brief Send a message over this link (to this or to all UAS on this link) */
    virtual void sendMessage(LinkInterface* link, mavlink_message_t message) = 0;
    /** @brief Send a message over all links this UAS can be reached with (!= all links) */
    virtual void sendMessage(mavlink_message_t message) = 0;

    enum Airframe {
        QGC_AIRFRAME_GENERIC = 0,
        QGC_AIRFRAME_EASYSTAR,
        QGC_AIRFRAME_TWINSTAR,
        QGC_AIRFRAME_MERLIN,
        QGC_AIRFRAME_CHEETAH,
        QGC_AIRFRAME_MIKROKOPTER,
        QGC_AIRFRAME_REAPER,
        QGC_AIRFRAME_PREDATOR,
        QGC_AIRFRAME_COAXIAL,
        QGC_AIRFRAME_PTERYX,
        QGC_AIRFRAME_TRICOPTER,
        QGC_AIRFRAME_HEXCOPTER,
        QGC_AIRFRAME_X8,
        QGC_AIRFRAME_VIPER_2_0,
        QGC_AIRFRAME_CAMFLYER_Q,
        QGC_AIRFRAME_END_OF_ENUM
    };

    /**
         * @brief Get the links associated with this robot
         *
         * @return List with all links this robot is associated with. Association is usually
         *         based on the fact that a message for this robot has been received through that
         *         interface. The LinkInterface can support multiple protocols.
         **/
    virtual QList<LinkInterface*> getLinks() = 0;
    
    /// @returns true: UAS is connected to log replay link
    virtual bool isLogReplay(void) = 0;

    /**
     * @brief Get the color for this UAS
     *
     * This static function holds a color map that allows to draw a new color for each robot
     *
     * @return The next color in the color map. The map holds 20 colors and starts from the beginning
     *         if the colors are exceeded.
     */
    static QColor getNextColor() {
        /* Create color map */
        static QList<QColor> colors = QList<QColor>()
		<< QColor(231,72,28)
		<< QColor(104,64,240)
		<< QColor(203,254,121)
		<< QColor(161,252,116)
               	<< QColor(232,33,47)
		<< QColor(116,251,110)
		<< QColor(234,38,107)
		<< QColor(104,250,138)
                << QColor(235,43,165)
		<< QColor(98,248,176)
		<< QColor(236,48,221)
		<< QColor(92,247,217)
                << QColor(200,54,238)
		<< QColor(87,231,246)
		<< QColor(151,59,239)
		<< QColor(81,183,244)
                << QColor(75,133,243)
		<< QColor(242,255,128)
		<< QColor(230,126,23);

        static int nextColor = -1;
        if(nextColor == 18){//if at the end of the list
            nextColor = -1;//go back to the beginning
        }
        nextColor++;
        return colors[nextColor];//return the next color
   }

    /** @brief Get the type of the system (airplane, quadrotor, helicopter,..)*/
    virtual int getSystemType() = 0;
    /** @brief Is it an airplane (or like one)?,..)*/
    virtual bool isAirplane() = 0;
    /** @brief Indicates whether this system is capable of controlling a reverse velocity.
     * Used for, among other things, altering joystick input to either -1:1 or 0:1 range.
     */
    virtual bool systemCanReverse() const = 0;

    virtual QString getSystemTypeName() = 0;
    /** @brief Get the type of the autopilot (PIXHAWK, APM, UDB, PPZ,..) */
    virtual int getAutopilotType() = 0;
    virtual QString getAutopilotTypeName() = 0;
    virtual void setAutopilotType(int apType) = 0;

    virtual QMap<int, QString> getComponents() = 0;

    QColor getColor()
    {
        return color;
    }

    /** @brief Returns a list of actions/commands that this vehicle can perform.
     * Used for creating UI elements for built-in functionality for this vehicle.
     * Actions should be mappings to `void f(void);` functions that simply issue
     * a command to the vehicle.
     */
    virtual QList<QAction*> getActions() const = 0;

    static const unsigned int WAYPOINT_RADIUS_DEFAULT_FIXED_WING = 25;
    static const unsigned int WAYPOINT_RADIUS_DEFAULT_ROTARY_WING = 5;
    
    enum StartCalibrationType {
        StartCalibrationRadio,
        StartCalibrationGyro,
        StartCalibrationMag,
        StartCalibrationAirspeed,
        StartCalibrationAccel,
        StartCalibrationLevel,
        StartCalibrationEsc,
        StartCalibrationCopyTrims,
        StartCalibrationUavcanEsc
    };

    enum StartBusConfigType {
        StartBusConfigActuators
    };
    
    /// Starts the specified calibration
    virtual void startCalibration(StartCalibrationType calType) = 0;
    
    /// Ends any current calibration
    virtual void stopCalibration(void) = 0;

    /// Starts the specified bus configuration
    virtual void startBusConfig(StartBusConfigType calType) = 0;

    /// Ends any current bus configuration
    virtual void stopBusConfig(void) = 0;

public slots:

    /** @brief Set a new name for the system */
    virtual void setUASName(const QString& name) = 0;
    /** @brief Execute command immediately **/
    virtual void executeCommand(MAV_CMD command) = 0;
    /** @brief Executes a command **/
    virtual void executeCommand(MAV_CMD command, int confirmation, float param1, float param2, float param3, float param4, float param5, float param6, float param7, int component) = 0;
    /** @brief Executes a command ack, with success boolean **/
    virtual void executeCommandAck(int num, bool success) = 0;

    /** @brief Selects the airframe */
    virtual void setAirframe(int airframe) = 0;

    /** @brief Launches the system/Liftof **/
    virtual void launch() = 0;
    /** @brief Set a new waypoint **/
    //virtual void setWaypoint(Waypoint* wp) = 0;
    /** @brief Set this waypoint as next waypoint to fly to */
    //virtual void setWaypointActive(int wp) = 0;
    /** @brief Order the robot to return home / to land on the runway **/
    virtual void home() = 0;
    /** @brief Order the robot to land **/
    virtual void land() = 0;
    /** @brief Order the robot to pair its receiver **/
    virtual void pairRX(int rxType, int rxSubType) = 0;
    /** @brief Halt the system */
    virtual void halt() = 0;
    /** @brief Start/continue the current robot action */
    virtual void go() = 0;
    /** @brief Set the current mode of operation */
    virtual void setMode(uint8_t newBaseMode, uint32_t newCustomMode) = 0;
    /** Stops the robot system. If it is an MAV, the robot starts the emergency landing procedure **/
    virtual void emergencySTOP() = 0;
    /** Kills the robot. All systems are immediately shut down (e.g. the main power line is cut). This might lead to a crash **/
    virtual bool emergencyKILL() = 0;
    /**
     * @brief Shut down the system's computers
     *
     * Works only if already landed and will cleanly shut down all onboard computers.
     */
    virtual void shutdown() = 0;
    /** @brief Set the target position for the robot to navigate to.
     *  @param x x-coordinate of the target position
     *  @param y y-coordinate of the target position
     *  @param z z-coordinate of the target position
     *  @param yaw heading of the target position
     */
    virtual void setTargetPosition(float x, float y, float z, float yaw) = 0;
    /** @brief Request the list of stored waypoints from the robot */
    //virtual void requestWaypoints() = 0;
    /** @brief Clear all existing waypoints on the robot */
    //virtual void clearWaypointList() = 0;
    /** @brief Set world frame origin at current GPS position */
    virtual void setLocalOriginAtCurrentGPSPosition() = 0;
    /** @brief Set world frame origin / home position at this GPS position */
    virtual void setHomePosition(double lat, double lon, double alt) = 0;

    /**
     * @brief Add a link to the list of current links
     *
     * Adding the link to the robot's internal link list will allow him so send its own messages
     * over all registered links. Usually a link is added once a message for this particular robot
     * has been received over a link, but adding could also be done manually.
     * @warning Not all links should be added to all robots by default, because else all robots will
     *          attempt to send over all links, typically choking wireless serial links.
     */
    virtual void addLink(LinkInterface* link) = 0;

    /**
     * @brief Set the current robot as focused in the user interface
     */
    virtual void setSelected() = 0;

    virtual void enableAllDataTransmission(int rate) = 0;
    virtual void enableRawSensorDataTransmission(int rate) = 0;
    virtual void enableExtendedSystemStatusTransmission(int rate) = 0;
    virtual void enableRCChannelDataTransmission(int rate) = 0;
    virtual void enableRawControllerDataTransmission(int rate) = 0;
    //virtual void enableRawSensorFusionTransmission(int rate) = 0;
    virtual void enablePositionTransmission(int rate) = 0;
    virtual void enableExtra1Transmission(int rate) = 0;
    virtual void enableExtra2Transmission(int rate) = 0;
    virtual void enableExtra3Transmission(int rate) = 0;

    virtual void setLocalPositionSetpoint(float x, float y, float z, float yaw) = 0;
    virtual void setLocalPositionOffset(float x, float y, float z, float yaw) = 0;

    /** @brief Return if this a rotary wing */
    virtual bool isRotaryWing() = 0;
    /** @brief Return if this is a fixed wing */
    virtual bool isFixedWing() = 0;

    /** @brief Set the current battery type and voltages */
    virtual void setBatterySpecs(const QString& specs) = 0;
    /** @brief Get the current battery type and specs */
    virtual QString getBatterySpecs() = 0;

    /** @brief Send the full HIL state to the MAV */
#ifndef __mobile__
    virtual void sendHilState(quint64 time_us, float roll, float pitch, float yaw, float rollspeed,
                        float pitchspeed, float yawspeed, double lat, double lon, double alt,
                        float vx, float vy, float vz, float ind_airspeed, float true_airspeed, float xacc, float yacc, float zacc) = 0;

    /** @brief RAW sensors for sensor HIL */
    virtual void sendHilSensors(quint64 time_us, float xacc, float yacc, float zacc, float rollspeed, float pitchspeed, float yawspeed,
                                float xmag, float ymag, float zmag, float abs_pressure, float diff_pressure, float pressure_alt, float temperature, quint32 fields_changed) = 0;

    /** @brief Send raw GPS for sensor HIL */
    virtual void sendHilGps(quint64 time_us, double lat, double lon, double alt, int fix_type, float eph, float epv, float vel, float vn, float ve, float vd, float cog, int satellites) = 0;

    /** @brief Send Optical Flow sensor message for HIL, (arguments and units accoding to mavlink documentation*/
    virtual void sendHilOpticalFlow(quint64 time_us, qint16 flow_x, qint16 flow_y, float flow_comp_m_x,
                            float flow_comp_m_y, quint8 quality, float ground_distance) = 0;
#endif

    /** @brief Send command to map a RC channel to a parameter */
    virtual void sendMapRCToParam(QString param_id, float scale, float value0, quint8 param_rc_channel_index, float valueMin, float valueMax) = 0;

    /** @brief Send command to disable all bindings/maps between RC and parameters */
    virtual void unsetRCToParameterMap() = 0;

protected:
    QColor color;

signals:
    /** @brief The robot state has changed */
    void statusChanged(int stateFlag);
    /** @brief A new component was detected or created */
    void componentCreated(int uas, int component, const QString& name);
    /** @brief The robot state has changed
     *
     * @param uas this robot
     * @param status short description of status, e.g. "connected"
     * @param description longer textual description. Should be however limited to a short text, e.g. 200 chars.
     */
    void statusChanged(UASInterface* uas, QString status, QString description);
    /**
     * @brief Received a plain text message from the robot
     * This signal should NOT be used for standard communication, but rather for VERY IMPORTANT
     * messages like critical errors.
     *
     * @param uasid ID of the sending system
     * @param compid ID of the sending component
     * @param text the status text
     * @param severity The severity of the message, 0 for plain debug messages, 10 for very critical messages
     */

    void poiFound(UASInterface* uas, int type, int colorIndex, QString message, float x, float y, float z);
    void poiConnectionFound(UASInterface* uas, int type, int colorIndex, QString message, float x1, float y1, float z1, float x2, float y2, float z2);

    /**
     * @brief A misconfiguration has been detected by the UAS
     */
    void misconfigurationDetected(UASInterface* uas);

    /** @brief A text message from the system has been received */
    void textMessageReceived(int uasid, int componentid, int severity, QString text);

    void navModeChanged(int uasid, int mode, const QString& text);

    /** @brief System is now armed */
    void armed();
    /** @brief System is now disarmed */
    void disarmed();
    /** @brief Arming mode changed */
    void armingChanged(bool armed);

    /**
     * @brief Update the error count of a device
     *
     * The error count indicates how many errors occured during the use of a device.
     * Usually a random error from time to time is acceptable, e.g. through electromagnetic
     * interferences on device lines like I2C and SPI. A constantly and rapidly increasing
     * error count however can help to identify broken cables or misbehaving drivers.
     *
     * @param uasid System ID
     * @param component Name of the component, e.g. "IMU"
     * @param device Name of the device, e.g. "SPI0" or "I2C1"
     * @param count Errors occured since system startup
     */
    void errCountChanged(int uasid, QString component, QString device, int count);

    /**
     * @brief Drop rate of communication link updated
     *
     * @param systemId id of the air system
     * @param receiveDrop drop rate of packets this MAV receives (sent from GCS or other MAVs)
     */
    void dropRateChanged(int systemId,  float receiveDrop);
    /** @brief Robot mode has changed */
    void modeChanged(int sysId, QString status, QString description);
    /** @brief A command has been issued **/
    void commandSent(int command);
    /** @brief The robot is connecting **/
    void connecting();
    /** @brief The robot is connected **/
    void connected();
    /** @brief The robot is disconnected **/
    void disconnected();
    /** @brief The robot is active **/
    void activated();
    /** @brief The robot is inactive **/
    void deactivated();
    /** @brief The robot is manually controlled **/
    void manualControl();

    /** @brief A value of the robot has changed.
      *
      * Typically this is used to send lowlevel information like the battery voltage to the plotting facilities of
      * the groundstation. The data here should be converted to human-readable values before being passed, so ideally
	  * SI units.
      *
      * @param uasId ID of this system
      * @param name name of the value, e.g. "battery voltage"
	  * @param unit The units this variable is in as an abbreviation. For system-dependent (such as raw ADC values) use "raw", for bitfields use "bits", for true/false or on/off use "bool", for unitless values use "-".
      * @param value the value that changed
      * @param msec the timestamp of the message, in milliseconds
      */
    void valueChanged(const int uasid, const QString& name, const QString& unit, const QVariant &value,const quint64 msecs);

    void voltageChanged(int uasId, double voltage);
    void waypointUpdated(int uasId, int id, double x, double y, double z, double yaw, bool autocontinue, bool active);
    void waypointSelected(int uasId, int id);
    void waypointReached(UASInterface* uas, int id);
    void autoModeChanged(bool autoMode);
    void parameterUpdate(int uas, int component, QString parameterName, int parameterCount, int parameterId, int type, QVariant value);
    /**
     * @brief The battery status has been updated
     *
     * @param uas sending system
     * @param voltage battery voltage
     * @param percent remaining capacity in percent
     * @param seconds estimated remaining flight time in seconds
     */
    void batteryChanged(UASInterface* uas, double voltage, double current, double percent, int seconds);
    void batteryConsumedChanged(UASInterface* uas, double current_consumed);
    void statusChanged(UASInterface* uas, QString status);
    void actuatorChanged(UASInterface*, int actId, double value);
    void thrustChanged(UASInterface*, double thrust);
    void heartbeat(UASInterface* uas);
    void attitudeChanged(UASInterface*, double roll, double pitch, double yaw, quint64 usec);
    void attitudeChanged(UASInterface*, int component, double roll, double pitch, double yaw, quint64 usec);
    void attitudeRotationRatesChanged(int uas, double rollrate, double pitchrate, double yawrate, quint64 usec);
    void attitudeThrustSetPointChanged(UASInterface*, float rollDesired, float pitchDesired, float yawDesired, float thrustDesired, quint64 usec);
    /** @brief The MAV set a new setpoint in the local (not body) NED X, Y, Z frame */
    void positionSetPointsChanged(int uasid, float xDesired, float yDesired, float zDesired, float yawDesired, quint64 usec);
    /** @brief A user (or an autonomous mission or obstacle avoidance planner) requested to set a new setpoint */
    void userPositionSetPointsChanged(int uasid, float xDesired, float yDesired, float zDesired, float yawDesired);
    void localPositionChanged(UASInterface*, double x, double y, double z, quint64 usec);
    void localPositionChanged(UASInterface*, int component, double x, double y, double z, quint64 usec);
    void globalPositionChanged(UASInterface*, double lat, double lon, double altAMSL, double altWGS84, quint64 usec);
    void altitudeChanged(UASInterface*, double altitudeAMSL, double altitudeWGS84, double altitudeRelative, double climbRate, quint64 usec);
    /** @brief Update the status of one satellite used for localization */
    void gpsSatelliteStatusChanged(int uasid, int satid, float azimuth, float direction, float snr, bool used);

    // The horizontal speed (a scalar)
    void speedChanged(UASInterface* uas, double groundSpeed, double airSpeed, quint64 usec);
    // Consider adding a MAV_FRAME parameter to this; could help specifying what the 3 scalars are.
    void velocityChanged_NED(UASInterface*, double vx, double vy, double vz, quint64 usec);

    void navigationControllerErrorsChanged(UASInterface*, double altitudeError, double speedError, double xtrackError);
    void NavigationControllerDataChanged(UASInterface *uas, float navRoll, float navPitch, float navBearing, float targetBearing, float targetDist);

    void imageStarted(int imgid, int width, int height, int depth, int channels);
    void imageDataReceived(int imgid, const unsigned char* imageData, int length, int startIndex);
    /** @brief Emit the new system type */
    void systemTypeSet(UASInterface* uas, unsigned int type);

    /** @brief Attitude control enabled/disabled */
    void attitudeControlEnabled(bool enabled);
    /** @brief Position 2D control enabled/disabled */
    void positionXYControlEnabled(bool enabled);
    /** @brief Altitude control enabled/disabled */
    void positionZControlEnabled(bool enabled);
    /** @brief Heading control enabled/disabled */
    void positionYawControlEnabled(bool enabled);
    /** @brief Optical flow status changed */
    void opticalFlowStatusChanged(bool supported, bool enabled, bool ok);
    /** @brief Vision based localization status changed */
    void visionLocalizationStatusChanged(bool supported, bool enabled, bool ok);
    /** @brief Infrared / Ultrasound status changed */
    void distanceSensorStatusChanged(bool supported, bool enabled, bool ok);
    /** @brief Gyroscope status changed */
    void gyroStatusChanged(bool supported, bool enabled, bool ok);
    /** @brief Accelerometer status changed */
    void accelStatusChanged(bool supported, bool enabled, bool ok);
    /** @brief Magnetometer status changed */
    void magSensorStatusChanged(bool supported, bool enabled, bool ok);
    /** @brief Barometer status changed */
    void baroStatusChanged(bool supported, bool enabled, bool ok);
    /** @brief Differential pressure / airspeed status changed */
    void airspeedStatusChanged(bool supported, bool enabled, bool ok);
    /** @brief Actuator status changed */
    void actuatorStatusChanged(bool supported, bool enabled, bool ok);
    /** @brief Laser scanner status changed */
    void laserStatusChanged(bool supported, bool enabled, bool ok);
    /** @brief Vicon / Leica Geotracker status changed */
    void groundTruthSensorStatusChanged(bool supported, bool enabled, bool ok);


    /** @brief Value of a remote control channel (raw) */
    void remoteControlChannelRawChanged(int channelId, float raw);
    /** @brief Value of a remote control channel (scaled)*/
    void remoteControlChannelScaledChanged(int channelId, float normalized);
    /** @brief Remote control RSSI changed  (0% - 100%)*/
    void remoteControlRSSIChanged(uint8_t rssi);

    /**
     * @brief Localization quality changed
     * @param fix 0: lost, 1: 2D local position hold, 2: 2D localization, 3: 3D localization
     */
    void localizationChanged(UASInterface* uas, int fix);
    /**
     * @brief GPS localization quality changed
     * @param fix 0: lost, 1: at least one satellite, but no GPS fix, 2: 2D localization, 3: 3D localization
     */
    void gpsLocalizationChanged(UASInterface* uas, int fix);
    /**
     * @brief Vision localization quality changed
     * @param fix 0: lost, 1: 2D local position hold, 2: 2D localization, 3: 3D localization
     */
    void visionLocalizationChanged(UASInterface* uas, int fix);
    /**
     * @brief IR/U localization quality changed
     * @param fix 0: No IR/Ultrasound sensor, N > 0: Found N active sensors
     */
    void irUltraSoundLocalizationChanged(UASInterface* uas, int fix);

    // ERROR AND STATUS SIGNALS
    /** @brief Heartbeat timed out or was regained */
    void heartbeatTimeout(bool timeout, unsigned int ms);
    /** @brief Name of system changed */
    void nameChanged(QString newName);
    /** @brief System has been selected as focused system */
    void systemSelected(bool selected);
    /** @brief Core specifications have changed */
    void systemSpecsChanged(int uasId);

    /** @brief Object detected */
    void objectDetected(unsigned int time, int id, int type, const QString& name, int quality, float bearing, float distance);


    // HOME POSITION / ORIGIN CHANGES
    void homePositionChanged(int uas, double lat, double lon, double alt);

    /** @brief The system received an unknown message, which it could not interpret */
    void unknownPacketReceived(int uas, int component, int messageid);

protected:

    // TIMEOUT CONSTANTS
    static const unsigned int timeoutIntervalHeartbeat = 3500 * 1000; ///< Heartbeat timeout is 3.5 seconds

};

Q_DECLARE_INTERFACE(UASInterface, "org.qgroundcontrol/1.0")

#endif // _UASINTERFACE_H_
