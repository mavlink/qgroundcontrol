/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

// NO NEW CODE HERE
// UASInterface, UAS.h/cc are deprecated. All new functionality should go into Vehicle.h/cc
//

#ifndef _UASINTERFACE_H_
#define _UASINTERFACE_H_

#include <QObject>
#include <QList>
#include <QAction>
#include <QColor>
#include <QPointer>

#include "LinkInterface.h"
#include "ProtocolInterface.h"

#ifndef __mobile__
class FileManager;
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

    virtual int getUASID() const = 0; ///< Get the ID of the connected UAS
    /** @brief The time interval the robot is switched on **/
    virtual quint64 getUptime() const = 0;

    virtual double getRoll() const = 0;
    virtual double getPitch() const = 0;
    virtual double getYaw() const = 0;

#ifndef __mobile__
    virtual FileManager* getFileManager() = 0;
#endif

    /**
     * @brief Get the color for this UAS
     *
     * This static function holds a color map that allows to draw a new color for each robot
     *
     * @return The next color in the color map. The map holds 20 colors and starts from the beginning
     *         if the colors are exceeded.
     */
#if !defined(__mobile__)
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
#endif

    virtual QMap<int, QString> getComponents() = 0;

    QColor getColor()
    {
        return color;
    }

    enum StartCalibrationType {
        StartCalibrationRadio,
        StartCalibrationGyro,
        StartCalibrationMag,
        StartCalibrationAirspeed,
        StartCalibrationAccel,
        StartCalibrationLevel,
        StartCalibrationPressure,
        StartCalibrationEsc,
        StartCalibrationCopyTrims,
        StartCalibrationUavcanEsc,
        StartCalibrationCompassMot,
    };

    enum StartBusConfigType {
        StartBusConfigActuators,
        EndBusConfigActuators,
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
    /** @brief Order the robot to pair its receiver **/
    virtual void pairRX(int rxType, int rxSubType) = 0;

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
    /** @brief A text message from the system has been received */
    void textMessageReceived(int uasid, int componentid, int severity, QString text);

    /**
     * @brief Update the error count of a device
     *
     * The error count indicates how many errors occurred during the use of a device.
     * Usually a random error from time to time is acceptable, e.g. through electromagnetic
     * interferences on device lines like I2C and SPI. A constantly and rapidly increasing
     * error count however can help to identify broken cables or misbehaving drivers.
     *
     * @param uasid System ID
     * @param component Name of the component, e.g. "IMU"
     * @param device Name of the device, e.g. "SPI0" or "I2C1"
     * @param count Errors occurred since system startup
     */
    void errCountChanged(int uasid, QString component, QString device, int count);

    /** @brief The robot is connected **/
    void connected();
    /** @brief The robot is disconnected **/
    void disconnected();

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
    void statusChanged(UASInterface* uas, QString status);
    void attitudeChanged(UASInterface*, double roll, double pitch, double yaw, quint64 usec);
    void attitudeChanged(UASInterface*, int component, double roll, double pitch, double yaw, quint64 usec);

    void imageStarted(int imgid, int width, int height, int depth, int channels);
    void imageDataReceived(int imgid, const unsigned char* imageData, int length, int startIndex);

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

    // ERROR AND STATUS SIGNALS
    /** @brief Name of system changed */
    void nameChanged(QString newName);
    /** @brief Core specifications have changed */
    void systemSpecsChanged(int uasId);

    // Log Download Signals
    void logEntry   (UASInterface* uas, uint32_t time_utc, uint32_t size, uint16_t id, uint16_t num_logs, uint16_t last_log_num);
    void logData    (UASInterface* uas, uint32_t ofs, uint16_t id, uint8_t count, const uint8_t* data);

};

Q_DECLARE_INTERFACE(UASInterface, "org.qgroundcontrol/1.0")

#endif // _UASINTERFACE_H_
