/****************************************************************************
 *
 *   (c) 2009-2018 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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

#include "LinkInterface.h"
#include "QGCConfig.h"
#include "UASInterface.h"
#include "QGCHilLink.h"
#include "QGCHilFlightGearConfiguration.h"
#include "Vehicle.h"

class QGCFlightGearLink : public QGCHilLink
{
    Q_OBJECT

public:
    QGCFlightGearLink(Vehicle* vehicle, QString startupArguments, QString remoteHost=QString("127.0.0.1:49000"), QHostAddress host = QHostAddress::Any, quint16 port = 49005);
    ~QGCFlightGearLink();

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

    bool sensorHilEnabled() {
        return _sensorHilEnabled;
    }

    void sensorHilEnabled(bool sensorHilEnabled) {
        _sensorHilEnabled = sensorHilEnabled;
    }
    
    static bool parseUIArguments(QString uiArgs, QStringList& argList);

    void run();
    
signals:
    void showCriticalMessageFromThread(const QString& title, const QString& message);

public slots:
//    void setAddress(QString address);
    void setPort(int port);
    /** @brief Add a new host to broadcast messages to */
    void setRemoteHost(const QString& host);
    /** @brief Send new control states to the simulation */
    void updateControls(quint64 time, float rollAilerons, float pitchElevator, float yawRudder, float throttle, quint8 systemMode, quint8 navMode);
    /** @brief Set the simulator version as text string */
    void setVersion(const QString& version)
    {
        Q_UNUSED(version);
    }

    void selectAirframe(const QString& airframe)
    {
        Q_UNUSED(airframe);
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
    void setBarometerOffset(float barometerOffsetkPa);
    void processError(QProcess::ProcessError err);

protected:
    void setName(QString name);
    
private slots:
    void _printFgfsOutput(void);
    void _printFgfsError(void);
    
private:
    static bool _findUIArgument(const QStringList& uiArgList, const QString& argLabel, QString& argValue);

    Vehicle*    _vehicle;
    QString     _fgProcessName;             ///< FlightGear process to start
    QString     _fgProcessWorkingDirPath;   ///< Working directory to start FG process in, empty for none
    QStringList _fgArgList;                 ///< Arguments passed to FlightGear process

    QUdpSocket* _udpCommSocket;             ///< UDP communication sockect between FG and QGC
    QProcess*   _fgProcess;                 ///< FlightGear process
    
    QString     _fgProtocolFileFullyQualified;  ///< Fully qualified file name for protocol file

    QString name;
    QHostAddress host;
    QHostAddress currentHost;
    quint16 currentPort;
    quint16 port;
    int id;
    bool connectState;

    unsigned int flightGearVersion;
    QString startupArguments;
    bool _sensorHilEnabled;
    float barometerOffsetkPa;
};

