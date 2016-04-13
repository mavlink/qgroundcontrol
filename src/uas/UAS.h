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
#include "Vehicle.h"
#include "FirmwarePluginManager.h"

#ifndef __mobile__
#include "FileManager.h"
#include "QGCHilLink.h"
#include "QGCFlightGearLink.h"
#include "QGCJSBSimLink.h"
#include "QGCXPlaneLink.h"
#endif

Q_DECLARE_LOGGING_CATEGORY(UASLog)

class Vehicle;

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
    UAS(MAVLinkProtocol* protocol, Vehicle* vehicle, FirmwarePluginManager * firmwarePluginManager);

    float lipoFull;  ///< 100% charged voltage
    float lipoEmpty; ///< Discharged voltage

    /* MANAGEMENT */

    /** @brief Get the unique system id */
    int getUASID() const;
    /** @brief Get the components */
    QMap<int, QString> getComponents();

    /** @brief The time interval the robot is switched on */
    quint64 getUptime() const;

    Q_PROPERTY(double   latitude                READ getLatitude            WRITE setLatitude           NOTIFY latitudeChanged)
    Q_PROPERTY(double   longitude               READ getLongitude           WRITE setLongitude          NOTIFY longitudeChanged)
    Q_PROPERTY(double   satelliteCount          READ getSatelliteCount      WRITE setSatelliteCount     NOTIFY satelliteCountChanged)
    Q_PROPERTY(bool     isGlobalPositionKnown   READ globalPositionKnown)
    Q_PROPERTY(double   roll                    READ getRoll                WRITE setRoll               NOTIFY rollChanged)
    Q_PROPERTY(double   pitch                   READ getPitch               WRITE setPitch              NOTIFY pitchChanged)
    Q_PROPERTY(double   yaw                     READ getYaw                 WRITE setYaw                NOTIFY yawChanged)
    Q_PROPERTY(double   distToWaypoint          READ getDistToWaypoint      WRITE setDistToWaypoint     NOTIFY distToWaypointChanged)
    Q_PROPERTY(double   airSpeed                READ getGroundSpeed         WRITE setGroundSpeed        NOTIFY airSpeedChanged)
    Q_PROPERTY(double   groundSpeed             READ getGroundSpeed         WRITE setGroundSpeed        NOTIFY groundSpeedChanged)
    Q_PROPERTY(double   bearingToWaypoint       READ getBearingToWaypoint   WRITE setBearingToWaypoint  NOTIFY bearingToWaypointChanged)
    Q_PROPERTY(double   altitudeAMSL            READ getAltitudeAMSL        WRITE setAltitudeAMSL       NOTIFY altitudeAMSLChanged)
    Q_PROPERTY(double   altitudeAMSLFT          READ getAltitudeAMSLFT                                  NOTIFY altitudeAMSLFTChanged)
    Q_PROPERTY(double   altitudeRelative        READ getAltitudeRelative    WRITE setAltitudeRelative   NOTIFY altitudeRelativeChanged)
    Q_PROPERTY(double   satRawHDOP              READ getSatRawHDOP                                      NOTIFY satRawHDOPChanged)
    Q_PROPERTY(double   satRawVDOP              READ getSatRawVDOP                                      NOTIFY satRawVDOPChanged)
    Q_PROPERTY(double   satRawCOG               READ getSatRawCOG                                       NOTIFY satRawCOGChanged)

    /// Vehicle is about to go away
    void shutdownVehicle(void);

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

    double getSatRawHDOP() const
    {
        return satRawHDOP;
    }

    double getSatRawVDOP() const
    {
        return satRawVDOP;
    }

    double getSatRawCOG() const
    {
        return satRawCOG;
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

#ifndef __mobile__
    friend class FileManager;
#endif

protected: //COMMENTS FOR TEST UNIT
    /// LINK ID AND STATUS
    int uasId;                    ///< Unique system ID
    QMap<int, QString> components;///< IDs and names of all detected onboard components

    QList<int> unknownPackets;    ///< Packet IDs which are unknown and have been received
    MAVLinkProtocol* mavlink;     ///< Reference to the MAVLink instance
    float receiveDropRate;        ///< Percentage of packets that were dropped on the MAV's receiving link (from GCS and other MAVs)
    float sendDropRate;           ///< Percentage of packets that were not received from the MAV by the GCS

    /// BASIC UAS TYPE, NAME AND STATE
    int status;                   ///< The current status of the MAV

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
    bool isGlobalPositionKnown; ///< If the global position has been received for this MAV

    double localX;
    double localY;
    double localZ;

    double latitude;            ///< Global latitude as estimated by position estimator
    double longitude;           ///< Global longitude as estimated by position estimator
    double altitudeAMSL;        ///< Global altitude as estimated by position estimator, AMSL
    double altitudeAMSLFT;      ///< Global altitude as estimated by position estimator, AMSL
    double altitudeRelative;    ///< Altitude above home as estimated by position estimator

    double satRawHDOP;
    double satRawVDOP;
    double satRawCOG;

    double satelliteCount;      ///< Number of satellites visible to raw GPS
    bool globalEstimatorActive; ///< Global position estimator present, do not fall back to GPS raw for position
    double latitude_gps;        ///< Global latitude as estimated by raw GPS
    double longitude_gps;       ///< Global longitude as estimated by raw GPS
    double altitude_gps;        ///< Global altitude as estimated by raw GPS
    double speedX;              ///< True speed in X axis
    double speedY;              ///< True speed in Y axis
    double speedZ;              ///< True speed in Z axis

    /// WAYPOINT NAVIGATION
    double distToWaypoint;       ///< Distance to next waypoint
    double airSpeed;             ///< Airspeed
    double groundSpeed;          ///< Groundspeed
    double bearingToWaypoint;    ///< Bearing to next waypoint
#ifndef __mobile__
    FileManager   fileManager;
#endif

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
    int imagePacketsArrived;    ///< Number of data packets received
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
    /** @brief Get the human-readable status message for this code */
    void getStatusForCode(int statusCode, QString& uasState, QString& stateDescription);

#ifndef __mobile__
    virtual FileManager* getFileManager() { return &fileManager; }
#endif

    /** @brief Get the HIL simulation */
#ifndef __mobile__
    QGCHilLink* getHILSimulation() const {
        return simulation;
    }
#endif

    QImage getImage();
    void requestImage();

public slots:
    /** @brief Executes a command with 7 params */
    void executeCommand(MAV_CMD command, int confirmation, float param1, float param2, float param3, float param4, float param5, float param6, float param7, int component);

    /** @brief Order the robot to pair its receiver **/
    void pairRX(int rxType, int rxSubType);

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

    /** @brief Set the values for the manual control of the vehicle */
    void setExternalControlSetpoint(float roll, float pitch, float yaw, float thrust, quint16 buttons, int joystickMode);

    /** @brief Set the values for the 6dof manual control of the vehicle */
#ifndef __mobile__
    void setManual6DOFControlCommands(double x, double y, double z, double roll, double pitch, double yaw);
#endif

    /** @brief Receive a message from one of the communication links. */
    virtual void receiveMessage(mavlink_message_t message);

    void startCalibration(StartCalibrationType calType);
    void stopCalibration(void);

    void startBusConfig(StartBusConfigType calType);
    void stopBusConfig(void);

    /** @brief Send command to map a RC channel to a parameter */
    void sendMapRCToParam(QString param_id, float scale, float value0, quint8 param_rc_channel_index, float valueMin, float valueMax);

    /** @brief Send command to disable all bindings/maps between RC and parameters */
    void unsetRCToParameterMap();
signals:
    void loadChanged(UASInterface* uas, double load);
    void imageStarted(quint64 timestamp);
    /** @brief A new camera image has arrived */
    void imageReady(UASInterface* uas);
    /** @brief HIL controls have changed */
    void hilControlsChanged(quint64 time, float rollAilerons, float pitchElevator, float yawRudder, float throttle, quint8 systemMode, quint8 navMode);

    void localXChanged(double val,QString name);
    void localYChanged(double val,QString name);
    void localZChanged(double val,QString name);
    void longitudeChanged(double val,QString name);
    void latitudeChanged(double val,QString name);
    void altitudeAMSLChanged(double val,QString name);
    void altitudeAMSLFTChanged(double val,QString name);
    void altitudeRelativeChanged(double val,QString name);

    void satRawHDOPChanged  (double value);
    void satRawVDOPChanged  (double value);
    void satRawCOGChanged   (double value);

    void rollChanged(double val,QString name);
    void pitchChanged(double val,QString name);
    void yawChanged(double val,QString name);
    void satelliteCountChanged(double val,QString name);
    void distToWaypointChanged(double val,QString name);
    void groundSpeedChanged(double val, QString name);
    void airSpeedChanged(double val, QString name);
    void bearingToWaypointChanged(double val,QString name);
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
    bool hilEnabled;
    bool sensorHil;             ///< True if sensor HIL is enabled
    quint64 lastSendTimeGPS;     ///< Last HIL GPS message sent
    quint64 lastSendTimeSensors; ///< Last HIL Sensors message sent
    quint64 lastSendTimeOpticalFlow; ///< Last HIL Optical Flow message sent

private:
    void _say(const QString& text, int severity = 6);

private:
    Vehicle*                _vehicle;
    FirmwarePluginManager*  _firmwarePluginManager;
};


#endif // _UAS_H_
