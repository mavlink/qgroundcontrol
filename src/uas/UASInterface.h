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
#include "QGCUASParamManager.h"
#include "RadioCalibration/RadioCalibrationData.h"

#ifdef QGC_PROTOBUF_ENABLED
#include <tr1/memory>
#ifdef QGC_USE_PIXHAWK_MESSAGES
#include <pixhawk/pixhawk.pb.h>
#endif
#endif

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
    /** @brief Translate mode id into text */
    static QString getShortModeTextFor(int id);
    //virtual QColor getColor() = 0;
    virtual int getUASID() const = 0; ///< Get the ID of the connected UAS
    /** @brief The time interval the robot is switched on **/
    virtual quint64 getUptime() const = 0;
    /** @brief Get the status flag for the communication **/
    virtual int getCommunicationStatus() const = 0;

    virtual double getLocalX() const = 0;
    virtual double getLocalY() const = 0;
    virtual double getLocalZ() const = 0;
    virtual bool localPositionKnown() const = 0;

    virtual double getLatitude() const = 0;
    virtual double getLongitude() const = 0;
    virtual double getAltitude() const = 0;
    virtual bool globalPositionKnown() const = 0;

    virtual double getRoll() const = 0;
    virtual double getPitch() const = 0;
    virtual double getYaw() const = 0;

    virtual bool getSelected() const = 0;

#if defined(QGC_PROTOBUF_ENABLED) && defined(QGC_USE_PIXHAWK_MESSAGES)
    virtual px::GLOverlay getOverlay() = 0;
    virtual px::GLOverlay getOverlay(qreal& receivedTimestamp) = 0;
    virtual px::ObstacleList getObstacleList() = 0;
    virtual px::ObstacleList getObstacleList(qreal& receivedTimestamp) = 0;
    virtual px::Path getPath() = 0;
    virtual px::Path getPath(qreal& receivedTimestamp) = 0;
    virtual px::PointCloudXYZRGB getPointCloud() = 0;
    virtual px::PointCloudXYZRGB getPointCloud(qreal& receivedTimestamp) = 0;
    virtual px::RGBDImage getRGBDImage() = 0;
    virtual px::RGBDImage getRGBDImage(qreal& receivedTimestamp) = 0;
#endif

    virtual bool isArmed() const = 0;

    /** @brief Set the airframe of this MAV */
    virtual int getAirframe() const = 0;

    /** @brief Get reference to the waypoint manager **/
    virtual UASWaypointManager* getWaypointManager(void) = 0;
    /** @brief Get reference to the param manager **/
    virtual QGCUASParamManager* getParamManager() const = 0;
    // TODO Will be removed
    /** @brief Set reference to the param manager **/
    virtual void setParamManager(QGCUASParamManager* manager) = 0;

    /* COMMUNICATION FLAGS */

    enum CommStatus {
        /** Unit is disconnected, no failure state reached so far **/
        COMM_DISCONNECTED = 0,
        /** The communication is established **/
        COMM_CONNECTING = 1,
        /** The communication link is up **/
        COMM_CONNECTED = 2,
        /** The connection is closed **/
        COMM_DISCONNECTING = 3,
        COMM_FAIL = 4,
        COMM_TIMEDOUT = 5///< Communication link failed
    };

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
        QGC_AIRFRAME_HEXCOPTER
    };

    /**
         * @brief Get the links associated with this robot
         *
         * @return List with all links this robot is associated with. Association is usually
         *         based on the fact that a message for this robot has been received through that
         *         interface. The LinkInterface can support multiple protocols.
         **/
    virtual QList<LinkInterface*>* getLinks() = 0;

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
        static QList<QColor> colors = QList<QColor>();
        static int nextColor = -1;

        if (nextColor == -1) {
            ///> Color map for plots, includes 20 colors
            ///> Map will start from beginning when the first 20 colors are exceeded

            colors.append(QColor(231,72,28));
            colors.append(QColor(104,64,240));
            colors.append(QColor(203,254,121));
            colors.append(QColor(161,252,116));
            colors.append(QColor(232,33,47));
            colors.append(QColor(116,251,110));
            colors.append(QColor(234,38,107));
            colors.append(QColor(104,250,138));
            colors.append(QColor(235,43,165));
            colors.append(QColor(98,248,176));
            colors.append(QColor(236,48,221));
            colors.append(QColor(92,247,217));
            colors.append(QColor(200,54,238));
            colors.append(QColor(87,231,246));
            colors.append(QColor(151,59,239));
            colors.append(QColor(81,183,244));
            colors.append(QColor(75,133,243));
            colors.append(QColor(242,255,128));
            colors.append(QColor(230,126,23));
            nextColor = 0;
        }
        return colors[nextColor++];
    }

    /** @brief Get the type of the system (airplane, quadrotor, helicopter,..)*/
    virtual int getSystemType() = 0;
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

public slots:

    /** @brief Set a new name for the system */
    virtual void setUASName(const QString& name) = 0;
    /** @brief Execute command immediately **/
    virtual void executeCommand(MAV_CMD command) = 0;
    /** @brief Executes a command **/
    virtual void executeCommand(MAV_CMD command, int confirmation, float param1, float param2, float param3, float param4, float param5, float param6, float param7, int component) = 0;

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
    /** @brief Halt the system */
    virtual void halt() = 0;
    /** @brief Start/continue the current robot action */
    virtual void go() = 0;
    /** @brief Set the current mode of operation */
    virtual void setMode(int mode) = 0;
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
    /** @brief Request all onboard parameters of all components */
    virtual void requestParameters() = 0;
    /** @brief Request one specific onboard parameter */
    virtual void requestParameter(int component, const QString& parameter) = 0;
    /** @brief Write parameter to permanent storage */
    virtual void writeParametersToStorage() = 0;
    /** @brief Read parameter from permanent storage */
    virtual void readParametersFromStorage() = 0;
    /** @brief Set a system parameter
     * @param component ID of the system component to write the parameter to
     * @param id String identifying the parameter
     * @warning The length of the ID string is limited by the MAVLink format! Take care to not exceed it
     * @param value Value of the parameter, IEEE 754 single precision floating point
     */
    virtual void setParameter(const int component, const QString& id, const QVariant& value) = 0;

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

    virtual void startRadioControlCalibration() = 0;
    virtual void startMagnetometerCalibration() = 0;
    virtual void startGyroscopeCalibration() = 0;
    virtual void startPressureCalibration() = 0;

    /** @brief Set the current battery type and voltages */
    virtual void setBatterySpecs(const QString& specs) = 0;
    /** @brief Get the current battery type and specs */
    virtual QString getBatterySpecs() = 0;

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
    /** @brief System has been removed / disconnected / shutdown cleanly, remove */
    void systemRemoved(UASInterface* uas);
    void systemRemoved();
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
    /** @brief Robot armed state has changed */
    void armingChanged(int sysId, QString armingState);
    /** @brief A command has been issued **/
    void commandSent(int command);
    /** @brief The connection status has changed **/
    void connectionChanged(CommStatus connectionFlag);
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
    void valueChanged(const int uasId, const QString& name, const QString& unit, const quint8 value, const quint64 msec);
    void valueChanged(const int uasId, const QString& name, const QString& unit, const qint8 value, const quint64 msec);
    void valueChanged(const int uasId, const QString& name, const QString& unit, const quint16 value, const quint64 msec);
    void valueChanged(const int uasId, const QString& name, const QString& unit, const qint16 value, const quint64 msec);
    void valueChanged(const int uasId, const QString& name, const QString& unit, const quint32 value, const quint64 msec);
    void valueChanged(const int uasId, const QString& name, const QString& unit, const qint32 value, const quint64 msec);
    void valueChanged(const int uasId, const QString& name, const QString& unit, const quint64 value, const quint64 msec);
    void valueChanged(const int uasId, const QString& name, const QString& unit, const qint64 value, const quint64 msec);
    void valueChanged(const int uasId, const QString& name, const QString& unit, const double value, const quint64 msec);

    void voltageChanged(int uasId, double voltage);
    void waypointUpdated(int uasId, int id, double x, double y, double z, double yaw, bool autocontinue, bool active);
    void waypointSelected(int uasId, int id);
    void waypointReached(UASInterface* uas, int id);
    void autoModeChanged(bool autoMode);
    void parameterChanged(int uas, int component, QString parameterName, QVariant value);
    void parameterChanged(int uas, int component, int parameterCount, int parameterId, QString parameterName, QVariant value);
    void patternDetected(int uasId, QString patternPath, float confidence, bool detected);
    void letterDetected(int uasId, QString letter, float confidence, bool detected);
    /**
     * @brief The battery status has been updated
     *
     * @param uas sending system
     * @param voltage battery voltage
     * @param percent remaining capacity in percent
     * @param seconds estimated remaining flight time in seconds
     */
    void batteryChanged(UASInterface* uas, double voltage, double percent, int seconds);
    void statusChanged(UASInterface* uas, QString status);
    void actuatorChanged(UASInterface*, int actId, double value);
    void thrustChanged(UASInterface*, double thrust);
    void heartbeat(UASInterface* uas);
    void attitudeChanged(UASInterface*, double roll, double pitch, double yaw, quint64 usec);
    void attitudeChanged(UASInterface*, int component, double roll, double pitch, double yaw, quint64 usec);
    void attitudeSpeedChanged(int uas, double rollspeed, double pitchspeed, double yawspeed, quint64 usec);
    void attitudeThrustSetPointChanged(UASInterface*, double rollDesired, double pitchDesired, double yawDesired, double thrustDesired, quint64 usec);
    /** @brief The MAV set a new setpoint in the local (not body) NED X, Y, Z frame */
    void positionSetPointsChanged(int uasid, float xDesired, float yDesired, float zDesired, float yawDesired, quint64 usec);
    /** @brief A user (or an autonomous mission or obstacle avoidance planner) requested to set a new setpoint */
    void userPositionSetPointsChanged(int uasid, float xDesired, float yDesired, float zDesired, float yawDesired);
    void localPositionChanged(UASInterface*, double x, double y, double z, quint64 usec);
    void localPositionChanged(UASInterface*, int component, double x, double y, double z, quint64 usec);
    void globalPositionChanged(UASInterface*, double lat, double lon, double alt, quint64 usec);
    void altitudeChanged(int uasid, double altitude);
    /** @brief Update the status of one satellite used for localization */
    void gpsSatelliteStatusChanged(int uasid, int satid, float azimuth, float direction, float snr, bool used);
    void speedChanged(UASInterface*, double x, double y, double z, quint64 usec);
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
    /** @brief Value of a remote control channel (raw) */
    void remoteControlChannelRawChanged(int channelId, float raw);
    /** @brief Value of a remote control channel (scaled)*/
    void remoteControlChannelScaledChanged(int channelId, float normalized);
    /** @brief Remote control RSSI changed */
    void remoteControlRSSIChanged(float rssi);
    /** @brief Radio Calibration Data has been received from the MAV*/
    void radioCalibrationReceived(const QPointer<RadioCalibrationData>&);

#if defined(QGC_PROTOBUF_ENABLED) && defined(QGC_USE_PIXHAWK_MESSAGES)
    /** @brief Overlay data has been changed */
    void overlayChanged(UASInterface* uas);
    /** @brief Obstacle list data has been changed */
    void obstacleListChanged(UASInterface* uas);
    /** @brief Path data has been changed */
    void pathChanged(UASInterface* uas);
    /** @brief Point cloud data has been changed */
    void pointCloudChanged(UASInterface* uas);
    /** @brief RGBD image data has been changed */
    void rgbdImageChanged(UASInterface* uas);
#endif

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
    /** @brief Heartbeat timed out */
    void heartbeatTimeout();
    /** @brief Heartbeat timed out */
    void heartbeatTimeout(unsigned int ms);
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

protected:

    // TIMEOUT CONSTANTS
    static const unsigned int timeoutIntervalHeartbeat = 2000 * 1000; ///< Heartbeat timeout is 1.5 seconds

};

Q_DECLARE_INTERFACE(UASInterface, "org.qgroundcontrol/1.0")

#endif // _UASINTERFACE_H_
