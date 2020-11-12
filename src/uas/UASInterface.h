/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

// NO NEW CODE HERE
// UASInterface, UAS.h/cc are deprecated. All new functionality should go into Vehicle.h/cc
//

#pragma once

#include <QObject>
#include <QList>
#include <QAction>
#include <QColor>
#include <QPointer>

#include "LinkInterface.h"

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

public slots:
    /** @brief Order the robot to pair its receiver **/
    virtual void pairRX(int rxType, int rxSubType) = 0;

signals:
    /** @brief The robot is connected **/
    void connected();
    /** @brief The robot is disconnected **/
    void disconnected();

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

