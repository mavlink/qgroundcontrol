/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/**
 * @file
 *   @brief UDP connection (server) for unmanned vehicles
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

class QGCJSBSimLink : public QGCHilLink
{
    Q_OBJECT
    //Q_INTERFACES(QGCJSBSimLinkInterface:LinkInterface)

public:
    QGCJSBSimLink(Vehicle* vehicle, QString startupArguments, QString remoteHost=QString("127.0.0.1:49000"), QHostAddress host = QHostAddress::Any, quint16 port = 49005);
    ~QGCJSBSimLink();

    bool isConnected();
    qint64 bytesAvailable();
    int getPort() const {
        return port;
    }

    /**
     * @brief The human readable port name
     */
    QString getName();

    /**
     * @brief Get remote host and port
     * @return string in format <host>:<port>
     */
    QString getRemoteHost();

    QString getVersion()
    {
        return QString("FlightGear %1").arg(flightGearVersion);
    }

    int getAirFrameIndex()
    {
        return -1;
    }

    void run();

    bool sensorHilEnabled() {
        return _sensorHilEnabled;
    }

public slots:
//    void setAddress(QString address);
    void setPort(int port);
    /** @brief Add a new host to broadcast messages to */
    void setRemoteHost(const QString& host);
    /** @brief Send new control states to the simulation */
    void updateControls(quint64 time, float rollAilerons, float pitchElevator, float yawRudder, float throttle, quint8 systemMode, quint8 navMode);
//    /** @brief Remove a host from broadcasting messages to */
//    void removeHost(const QString& host);
    //    void readPendingDatagrams();
    void processError(QProcess::ProcessError err);
    /** @brief Set the simulator version as text string */
    void setVersion(const QString& version)
    {
        Q_UNUSED(version);
    }

    void selectAirframe(const QString& airframe)
    {
        script = airframe;
    }

    void enableSensorHIL(bool enable) {
        if (enable != _sensorHilEnabled)
            _sensorHilEnabled = enable;
            emit sensorHilChanged(enable);
    }

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

    void setStartupArguments(QString startupArguments);

private:
    Vehicle*    _vehicle;
    QString name;
    QHostAddress host;
    QHostAddress currentHost;
    quint16 currentPort;
    quint16 port;
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
    unsigned int flightGearVersion;
    QString startupArguments;
    QString script;
    bool _sensorHilEnabled;

    void setName(QString name);
};

