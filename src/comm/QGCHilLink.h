#ifndef QGCHILLINK_H
#define QGCHILLINK_H

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

public slots:
    virtual void setPort(int port) = 0;
    /** @brief Add a new host to broadcast messages to */
    virtual void setRemoteHost(const QString& host) = 0;
    /** @brief Send new control states to the simulation */
    virtual void updateControls(uint64_t time, float rollAilerons, float pitchElevator, float yawRudder, float throttle, uint8_t systemMode, uint8_t navMode) = 0;
    virtual void processError(QProcess::ProcessError err) = 0;

    virtual void readBytes() = 0;
    /**
     * @brief Write a number of bytes to the interface.
     *
     * @param data Pointer to the data byte array
     * @param size The size of the bytes array
     **/
    virtual void writeBytes(const char* data, qint64 length) = 0;
    virtual bool connectSimulation() = 0;
    virtual bool disconnectSimulation() = 0;

protected:
    virtual void setName(QString name) = 0;

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
     * @brief This signal is emitted instantly when the link status changes
     **/
    void simulationConnected(bool connected);

    /** @brief State update from FlightGear */
    void hilStateChanged(uint64_t time_us, float roll, float pitch, float yaw, float rollspeed,
                        float pitchspeed, float yawspeed, int32_t lat, int32_t lon, int32_t alt,
                        int16_t vx, int16_t vy, int16_t vz, int16_t xacc, int16_t yacc, int16_t zacc);
    
};

#endif // QGCHILLINK_H
