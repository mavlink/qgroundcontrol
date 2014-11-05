/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009 - 2011 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

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
 *   @brief UDP connection (server) for unmanned vehicles
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#ifndef QGCFLIGHTGEARLINK_H
#define QGCFLIGHTGEARLINK_H

#include <QString>
#include <QList>
#include <QMap>
#include <QMutex>
#include <QUdpSocket>
#include <QTimer>
#include <QProcess>
#include <LinkInterface.h>
#include "QGCConfig.h"
#include "UASInterface.h"
#include "QGCHilLink.h"
#include <QGCHilFlightGearConfiguration.h>

class QGCFlightGearLink : public QGCHilLink
{
    Q_OBJECT

public:
    QGCFlightGearLink(UASInterface* mav, QString startupArguments, QString remoteHost=QString("127.0.0.1:49000"), QHostAddress host = QHostAddress::Any, quint16 port = 49005);
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
    void updateActuators(quint64 time, float act1, float act2, float act3, float act4, float act5, float act6, float act7, float act8);
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
    /**
     * @brief Write a number of bytes to the interface.
     *
     * @param data Pointer to the data byte array
     * @param size The size of the bytes array
     **/
    void writeBytes(const char* data, qint64 length);
    bool connectSimulation();
    bool disconnectSimulation();

    void setStartupArguments(QString startupArguments);
    void setBarometerOffset(float barometerOffsetkPa);
    void processError(QProcess::ProcessError err);

protected:
    QString name;
    QHostAddress host;
    QHostAddress currentHost;
    quint16 currentPort;
    quint16 port;
    int id;
    bool connectState;

    UASInterface* mav;
    unsigned int flightGearVersion;
    QString startupArguments;
    bool _sensorHilEnabled;
    float barometerOffsetkPa;

    void setName(QString name);
    
private slots:
    void _printFgfsOutput(void);
    void _printFgfsError(void);
    
private:
    static bool _findUIArgument(const QStringList& uiArgList, const QString& argLabel, QString& argValue);

    QString     _fgProcessName;             ///< FlightGear process to start
    QString     _fgProcessWorkingDirPath;   ///< Working directory to start FG process in, empty for none
    QStringList _fgArgList;                 ///< Arguments passed to FlightGear process

    QUdpSocket* _udpCommSocket;             ///< UDP communication sockect between FG and QGC
    QProcess*   _fgProcess;                 ///< FlightGear process
    
    QString     _fgProtocolFileFullyQualified;  ///< Fully qualified file name for protocol file
};

#endif // QGCFLIGHTGEARLINK_H
