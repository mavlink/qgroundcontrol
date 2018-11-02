#pragma once

#include <QThread>
#include <QProcess>
#include "inttypes.h"

class QGCHilLink : public QThread
{
    Q_OBJECT
public:
    
    virtual bool isConnected() = 0;
    virtual qint64 bytesAvailable() = 0;
    virtual int getPort() const = 0;

    /**
     * @brief The human readable port name
     */
    virtual QString getName() = 0;

    /**
     * @brief Get remote host and port
     * @return string in format <host>:<port>
     */
    virtual QString getRemoteHost() = 0;

    /**
     * @brief Get the application name and version
     * @return A string containing a unique application name and compatibility version
     */
    virtual QString getVersion() = 0;

    /**
     * @brief Get index of currently selected airframe
     * @return -1 if default is selected, index else
     */
    virtual int getAirFrameIndex() = 0;

    /**
     * @brief Check if sensor level HIL is enabled
     * @return true if sensor HIL is enabled
     */
    virtual bool sensorHilEnabled() = 0;

public slots:
    virtual void setPort(int port) = 0;
    /** @brief Add a new host to broadcast messages to */
    virtual void setRemoteHost(const QString& host) = 0;
    /** @brief Send new control states to the simulation */
    virtual void updateControls(quint64 time, float rollAilerons, float pitchElevator, float yawRudder, float throttle, quint8 systemMode, quint8 navMode) = 0;
    virtual void processError(QProcess::ProcessError err) = 0;
    /** @brief Set the simulator version as text string */
    virtual void setVersion(const QString& version) = 0;
    /** @brief Enable sensor-level HIL (instead of state-level HIL) */
    virtual void enableSensorHIL(bool enable) = 0;

    virtual void selectAirframe(const QString& airframe) = 0;

    virtual void readBytes() = 0;
    /**
     * @brief Write a number of bytes to the interface.
     *
     * @param data Pointer to the data byte array
     * @param size The size of the bytes array
     **/
    void writeBytesSafe(const char* data, int length)
    {
        emit _invokeWriteBytes(QByteArray(data, length));
    }

    virtual bool connectSimulation() = 0;
    virtual bool disconnectSimulation() = 0;

private slots:
    virtual void _writeBytes(const QByteArray) = 0;

protected:
    virtual void setName(QString name) = 0;

    QGCHilLink() :
        QThread()
    {
        connect(this, &QGCHilLink::_invokeWriteBytes, this, &QGCHilLink::_writeBytes);
    }

signals:
    /**
     * @brief This signal is emitted instantly when the link is connected
     **/
    void simulationConnected();

    /**
     * @brief This signal is emitted instantly when the link is disconnected
     **/
    void simulationDisconnected();

    /**
     * @brief Thread safe signal to disconnect simulator from other threads
     **/
    void disconnectSim();

    /**
     * @brief This signal is emitted instantly when the link status changes
     **/
    void simulationConnected(bool connected);

    /** @brief State update from simulation */
    void hilStateChanged(quint64 time_us, float roll, float pitch, float yaw, float rollspeed,
                                          float pitchspeed, float yawspeed, double lat, double lon, double alt,
                                          float vx, float vy, float vz, float ind_airspeed, float true_airspeed, float xacc, float yacc, float zacc);

    void hilGroundTruthChanged(quint64 time_us, float roll, float pitch, float yaw, float rollspeed,
                              float pitchspeed, float yawspeed, double lat, double lon, double alt,
                              float vx, float vy, float vz, float ind_airspeed, float true_airspeed, float xacc, float yacc, float zacc);

    void sensorHilGpsChanged(quint64 time_us, double lat, double lon, double alt, int fix_type, float eph, float epv, float vel, float vn, float ve, float vd, float cog, int satellites);

    void sensorHilRawImuChanged(quint64 time_us, float xacc, float yacc, float zacc,
                                                  float xgyro, float ygyro, float zgyro,
                                                  float xmag, float ymag, float zmag,
                                                  float abs_pressure, float diff_pressure,
                                                  float pressure_alt, float temperature,
                                                  quint32 fields_updated);

    void sensorHilOpticalFlowChanged(quint64 time_us, qint16 flow_x, qint16 flow_y, float flow_comp_m_x,
                                     float flow_comp_m_y, quint8 quality, float ground_distance);
    
    /** @brief Remote host and port changed */
    void remoteChanged(const QString& hostPort);

    /** @brief Status text message from link */
    void statusMessage(const QString& message);

    /** @brief Airframe changed */
    void airframeChanged(const QString& airframe);

    /** @brief Selected sim version changed */
    void versionChanged(const QString& version);

    /** @brief Selected sim version changed */
    void versionChanged(const int version);

    /** @brief Sensor leve HIL state changed */
    void sensorHilChanged(bool enabled);

    /** @brief Helper signal to force execution on the correct thread */
    void _invokeWriteBytes(QByteArray);
};

