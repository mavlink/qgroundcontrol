/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/**
 * @file QGCXPlaneLink.h
 *   @brief X-Plane simulation link
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#pragma once

#include <QString>
#include <QList>
#include <QMap>
#include <QMutex>
#include <QUdpSocket>
#include <QTimer>
#include <QProcess>
#include <LinkInterface.h>
#include "QGCConfig.h"
#include "QGCHilLink.h"
#include "Vehicle.h"

class QGCXPlaneLink : public QGCHilLink
{
    Q_OBJECT
    //Q_INTERFACES(QGCXPlaneLinkInterface:LinkInterface)

public:
    QGCXPlaneLink(Vehicle* vehicle, QString remoteHost=QString("127.0.0.1:49000"), QHostAddress localHost = QHostAddress::Any, quint16 localPort = 49005);
    ~QGCXPlaneLink();

    /**
     * @brief Load X-Plane HIL settings
     */
    void loadSettings();

    /**
     * @brief Store X-Plane HIL settings
     */
    void storeSettings();

    bool isConnected();
    qint64 bytesAvailable();
    int getPort() const {
        return localPort;
    }

    /**
     * @brief The human readable port name
     */
    QString getName();

    void run();

    /**
     * @brief Get remote host and port
     * @return string in format <host>:<port>
     */
    QString getRemoteHost();

    enum AIRFRAME
    {
        AIRFRAME_UNKNOWN = 0,
        AIRFRAME_QUAD_DJI_F450_PWM,
        AIRFRAME_QUAD_X_MK_10INCH_I2C,
        AIRFRAME_QUAD_X_ARDRONE,
        AIRFRAME_FIXED_WING_BIXLER_II,
        AIRFRAME_FIXED_WING_BIXLER_II_AILERONS
    };

    QString getVersion()
    {
        return QString("X-Plane %1").arg(xPlaneVersion);
    }

    int getAirFrameIndex()
    {
        return (int)airframeID;
    }

    bool sensorHilEnabled() {
        return _sensorHilEnabled;
    }

    bool useHilActuatorControls() {
        return _useHilActuatorControls;
    }

signals:
    /** @brief Sensor leve HIL state changed */
    void useHilActuatorControlsChanged(bool enabled);

public slots:
//    void setAddress(QString address);
    void setPort(int port);
    /** @brief Add a new host to broadcast messages to */
    void setRemoteHost(const QString& host);
    /** @brief Send new control states to the simulation */
    void updateControls(quint64 time, float rollAilerons, float pitchElevator, float yawRudder, float throttle, quint8 systemMode, quint8 navMode);
    /** @brief Send new control commands to the simulation */
    void updateActuatorControls(quint64 time, quint64 flags,
                                float ctl_0,
                                float ctl_1,
                                float ctl_2,
                                float ctl_3,
                                float ctl_4,
                                float ctl_5,
                                float ctl_6,
                                float ctl_7,
                                float ctl_8,
                                float ctl_9,
                                float ctl_10,
                                float ctl_11,
                                float ctl_12,
                                float ctl_13,
                                float ctl_14,
                                float ctl_15,
                                quint8 mode);
    /** @brief Set the simulator version as text string */
    void setVersion(const QString& version);
    /** @brief Set the simulator version as integer */
    void setVersion(unsigned int version);

    void enableSensorHIL(bool enable) {
        if (enable != _sensorHilEnabled)
            _sensorHilEnabled = enable;
            emit sensorHilChanged(enable);
    }

    void enableHilActuatorControls(bool enable);

    void processError(QProcess::ProcessError err);

    void readBytes();

private slots:
    /**
     * @brief Write a number of bytes to the interface.
     *
     * @param data Pointer to the data byte array
     * @param size The size of the bytes array
     **/
    void _writeBytes(const QByteArray data);

public slots:
    bool connectSimulation();
    bool disconnectSimulation();
    /**
     * @brief Select airplane model
     * @param plane the name of the airplane
     */
    void selectAirframe(const QString& airframe);
    /**
     * @brief Set the airplane position and attitude
     * @param lat
     * @param lon
     * @param alt
     * @param roll
     * @param pitch
     * @param yaw
     */
    void setPositionAttitude(double lat, double lon, double alt, double roll, double pitch, double yaw);

    /**
     * @brief Set a random position
     */
    void setRandomPosition();

    /**
     * @brief Set a random attitude
     */
    void setRandomAttitude();

protected:
    Vehicle* _vehicle;
    QString name;
    QHostAddress localHost;
    quint16 localPort;
    QHostAddress remoteHost;
    quint16 remotePort;
    int id;
    QUdpSocket* socket;
    bool connectState;

    quint64 bitsSentTotal;
    quint64 bitsSentCurrent;
    quint64 bitsSentMax;
    quint64 bitsReceivedTotal;
    quint64 bitsReceivedCurrent;
    quint64 bitsReceivedMax;
    quint64 connectionStartTime;
    QMutex statisticsMutex;
    QMutex dataMutex;
    QTimer refreshTimer;
    QProcess* process;
    QProcess* terraSync;

    bool gpsReceived;
    bool attitudeReceived;

    float roll, pitch, yaw, rollspeed, pitchspeed, yawspeed;
    double lat, lon, alt, alt_agl;
    float vx, vy, vz, xacc, yacc, zacc;
    float ind_airspeed;
    float true_airspeed;
    float groundspeed;
    float xmag, ymag, zmag, abs_pressure, diff_pressure, pressure_alt, temperature;
    float barometerOffsetkPa;

    float man_roll, man_pitch, man_yaw;
    QString airframeName;
    enum AIRFRAME airframeID;
    bool xPlaneConnected;
    unsigned int xPlaneVersion;
    quint64 simUpdateLast;
    quint64 simUpdateFirst;
    quint64 simUpdateLastText;
    quint64 simUpdateLastGroundTruth;
    float simUpdateHz;
    bool _sensorHilEnabled;
    bool _useHilActuatorControls;
    bool _should_exit;

    void setName(QString name);
    void sendDataRef(QString ref, float value);
};

