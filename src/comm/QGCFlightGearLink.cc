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
 *   @brief Definition of UDP connection (server) for unmanned vehicles
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#include <QTimer>
#include <QList>
#include <QDebug>
#include <QMutexLocker>
#include <iostream>
#include "QGCFlightGearLink.h"
#include "QGC.h"
#include <QHostInfo>
#include "MainWindow.h"

QGCFlightGearLink::QGCFlightGearLink(UASInterface* mav, QString remoteHost, QHostAddress host, quint16 port)
{
    this->host = host;
    this->port = port;
    this->connectState = false;
    this->currentPort = 49000;
    this->mav = mav;
    // Set unique ID and add link to the list of links
    this->name = tr("FlightGear Link (port:%1)").arg(port);
    setRemoteHost(remoteHost);
}

QGCFlightGearLink::~QGCFlightGearLink()
{
    disconnectSimulation();
}

/**
 * @brief Runs the thread
 *
 **/
void QGCFlightGearLink::run()
{
    exec();
}

void QGCFlightGearLink::setPort(int port)
{
    this->port = port;
    disconnectSimulation();
    connectSimulation();
}

void QGCFlightGearLink::processError(QProcess::ProcessError err)
{
    switch(err)
    {
    case QProcess::FailedToStart:
        MainWindow::instance()->showCriticalMessage(tr("FlightGear Failed to Start"), tr("Please check if the path and command is correct"));
        break;
    case QProcess::Crashed:
        MainWindow::instance()->showCriticalMessage(tr("FlightGear Crashed"), tr("This is a FlightGear-related problem. Please upgrade FlightGear"));
        break;
    case QProcess::Timedout:
        MainWindow::instance()->showCriticalMessage(tr("FlightGear Start Timed Out"), tr("Please check if the path and command is correct"));
        break;
    case QProcess::WriteError:
        MainWindow::instance()->showCriticalMessage(tr("Could not Communicate with FlightGear"), tr("Please check if the path and command is correct"));
        break;
    case QProcess::ReadError:
        MainWindow::instance()->showCriticalMessage(tr("Could not Communicate with FlightGear"), tr("Please check if the path and command is correct"));
        break;
    case QProcess::UnknownError:
        MainWindow::instance()->showCriticalMessage(tr("FlightGear Error"), tr("Please check if the path and command is correct"));
        break;
    default:
        MainWindow::instance()->showCriticalMessage(tr("FlightGear Error"), tr("Please check if the path and command is correct."));
        break;
    }
}

/**
 * @param host Hostname in standard formatting, e.g. localhost:14551 or 192.168.1.1:14551
 */
void QGCFlightGearLink::setRemoteHost(const QString& host)
{
    //qDebug() << "UDP:" << "ADDING HOST:" << host;
    if (host.contains(":"))
    {
        //qDebug() << "HOST: " << host.split(":").first();
        QHostInfo info = QHostInfo::fromName(host.split(":").first());
        if (info.error() == QHostInfo::NoError)
        {
            // Add host
            QList<QHostAddress> hostAddresses = info.addresses();
            QHostAddress address;
            for (int i = 0; i < hostAddresses.size(); i++)
            {
                // Exclude loopback IPv4 and all IPv6 addresses
                if (!hostAddresses.at(i).toString().contains(":"))
                {
                    address = hostAddresses.at(i);
                }
            }
            currentHost = address;
            //qDebug() << "Address:" << address.toString();
            // Set port according to user input
            currentPort = host.split(":").last().toInt();
        }
    }
    else
    {
        QHostInfo info = QHostInfo::fromName(host);
        if (info.error() == QHostInfo::NoError)
        {
            // Add host
            currentHost = info.addresses().first();
        }
    }
}

void QGCFlightGearLink::updateControls(uint64_t time, float rollAilerons, float pitchElevator, float yawRudder, float throttle, uint8_t systemMode, uint8_t navMode)
{
    // 37.613548,-122.357246,-9999.000000,0.000000,0.424000,297.899994,0.000000\n
    // magnetos,aileron,elevator,rudder,throttle\n

    float magnetos = 3.0f;
    Q_UNUSED(time);
    Q_UNUSED(systemMode);
    Q_UNUSED(navMode);

    QString state("%1,%2,%3,%4,%5\n");
    state = state.arg(magnetos).arg(rollAilerons).arg(pitchElevator).arg(yawRudder).arg(throttle);
    writeBytes(state.toAscii().constData(), state.length());
    //qDebug() << "Updated controls" << state;
}

void QGCFlightGearLink::writeBytes(const char* data, qint64 size)
{
    //#define QGCFlightGearLink_DEBUG
#ifdef QGCFlightGearLink_DEBUG
    QString bytes;
    QString ascii;
    for (int i=0; i<size; i++)
    {
        unsigned char v = data[i];
        bytes.append(QString().sprintf("%02x ", v));
        if (data[i] > 31 && data[i] < 127)
        {
            ascii.append(data[i]);
        }
        else
        {
            ascii.append(219);
        }
    }
    qDebug() << "Sent" << size << "bytes to" << currentHost.toString() << ":" << currentPort << "data:";
    qDebug() << bytes;
    qDebug() << "ASCII:" << ascii;
#endif
    if (connectState && socket) socket->writeDatagram(data, size, currentHost, currentPort);
}

/**
 * @brief Read a number of bytes from the interface.
 *
 * @param data Pointer to the data byte array to write the bytes to
 * @param maxLength The maximum number of bytes to write
 **/
void QGCFlightGearLink::readBytes()
{
    const qint64 maxLength = 65536;
    static char data[maxLength];
    QHostAddress sender;
    quint16 senderPort;

    unsigned int s = socket->pendingDatagramSize();
    if (s > maxLength) std::cerr << __FILE__ << __LINE__ << " UDP datagram overflow, allowed to read less bytes than datagram size" << std::endl;
    socket->readDatagram(data, maxLength, &sender, &senderPort);

    QByteArray b(data, s);

    // Print string
    QString state(b);
    //qDebug() << "FG LINK GOT:" << state;

    QStringList values = state.split(",");

    // Parse string
    float roll, pitch, yaw, rollspeed, pitchspeed, yawspeed;
    int32_t lat, lon, alt;
    int16_t vx, vy, vz, xacc, yacc, zacc;

    lat = values.at(0).toDouble();
    lon = values.at(1).toDouble();
    alt = values.at(2).toDouble();
    roll = values.at(3).toDouble();
    pitch = values.at(4).toDouble();
    yaw = values.at(5).toDouble();



    // Send updated state
    emit hilStateChanged(QGC::groundTimeUsecs(), roll, pitch, yaw, rollspeed,
                         pitchspeed, yawspeed, lat, lon, alt,
                         vx, vy, vz, xacc, yacc, zacc);

    //    // Echo data for debugging purposes
    //    std::cerr << __FILE__ << __LINE__ << "Received datagram:" << std::endl;
    //    int i;
    //    for (i=0; i<s; i++)
    //    {
    //        unsigned int v=data[i];
    //        fprintf(stderr,"%02x ", v);
    //    }
    //    std::cerr << std::endl;
}


/**
 * @brief Get the number of bytes to read.
 *
 * @return The number of bytes to read
 **/
qint64 QGCFlightGearLink::bytesAvailable()
{
    return socket->pendingDatagramSize();
}

/**
 * @brief Disconnect the connection.
 *
 * @return True if connection has been disconnected, false if connection couldn't be disconnected.
 **/
bool QGCFlightGearLink::disconnectSimulation()
{
    disconnect(process, SIGNAL(error(QProcess::ProcessError)),
                          this, SLOT(processError(QProcess::ProcessError)));
    disconnect(mav, SIGNAL(hilControlsChanged(uint64_t, float, float, float, float, uint8_t, uint8_t)), this, SLOT(updateControls(uint64_t,float,float,float,float,uint8_t,uint8_t)));
    disconnect(this, SIGNAL(hilStateChanged(uint64_t,float,float,float,float,float,float,int32_t,int32_t,int32_t,int16_t,int16_t,int16_t,int16_t,int16_t,int16_t)), mav, SLOT(sendHilState(uint64_t,float,float,float,float,float,float,int32_t,int32_t,int32_t,int16_t,int16_t,int16_t,int16_t,int16_t,int16_t)));

    process->close();
    delete process;
    process = NULL;
    delete socket;
    socket = NULL;

    connectState = false;

    emit flightGearDisconnected();
    emit flightGearConnected(false);
    return !connectState;
}

/**
 * @brief Connect the connection.
 *
 * @return True if connection has been established, false if connection couldn't be established.
 **/
bool QGCFlightGearLink::connectSimulation()
{
    if (!mav) return false;
    socket = new QUdpSocket(this);

    //Check if we are using a multicast-address
    //    bool multicast = false;
    //    if (host.isInSubnet(QHostAddress("224.0.0.0"),4))
    //    {
    //        multicast = true;
    //        connectState = socket->bind(port, QUdpSocket::ShareAddress);
    //    }
    //    else
    //    {
    connectState = socket->bind(host, port);
    //    }

    //Provides Multicast functionality to UdpSocket
    /* not working yet
    if (multicast)
    {
        int sendingFd = socket->socketDescriptor();

        if (sendingFd != -1)
        {
            // set up destination address
            struct sockaddr_in sendAddr;
            memset(&sendAddr,0,sizeof(sendAddr));
            sendAddr.sin_family=AF_INET;
            sendAddr.sin_addr.s_addr=inet_addr(HELLO_GROUP);
            sendAddr.sin_port=htons(port);

            // set TTL
            unsigned int ttl = 1; // restricted to the same subnet
            if (setsockopt(sendingFd, IPPROTO_IP, IP_MULTICAST_TTL, (unsigned int*)&ttl, sizeof(ttl) ) < 0)
            {
                std::cout << "TTL failed\n";
            }
        }
    }
    */

    //QObject::connect(socket, SIGNAL(readyRead()), this, SLOT(readPendingDatagrams()));
    QObject::connect(socket, SIGNAL(readyRead()), this, SLOT(readBytes()));

    process = new QProcess(this);

//    forever
//    {
//        QGC::SLEEP::msleep(5000);
//    }
//refreshTimer.start(5); // 200 Hz UAV -> Simulation update rate
connect(mav, SIGNAL(hilControlsChanged(uint64_t, float, float, float, float, uint8_t, uint8_t)), this, SLOT(updateControls(uint64_t,float,float,float,float,uint8_t,uint8_t)));
connect(this, SIGNAL(hilStateChanged(uint64_t,float,float,float,float,float,float,int32_t,int32_t,int32_t,int16_t,int16_t,int16_t,int16_t,int16_t,int16_t)), mav, SLOT(sendHilState(uint64_t,float,float,float,float,float,float,int32_t,int32_t,int32_t,int16_t,int16_t,int16_t,int16_t,int16_t,int16_t)));

//connect(&refreshTimer, SIGNAL(timeout()), this, SLOT(sendUAVUpdate()));
// Catch process error
QObject::connect( process, SIGNAL(error(QProcess::ProcessError)),
                      this, SLOT(processError(QProcess::ProcessError)));
// Start Flightgear
QStringList processCall;
QString processFgfs;
QString fgRoot;
QString fgScenery;
QString aircraft("Rascal110-JSBSim");

#ifdef Q_OS_MACX
processFgfs = "/Applications/FlightGear.app/Contents/Resources/fgfs";
fgRoot = "--fg-root=/Applications/FlightGear.app/Contents/Resources/data";
fgScenery = "--fg-scenery=/Applications/FlightGear.app/Contents/Resources/data/Scenery";
#endif

#ifdef Q_OS_WIN32
processFgfs = "C:\Program Files (x86)\FlightGear\bin\Win32\fgfs";
fgRoot = "--fg-root=C:\Program Files (x86)\FlightGear\data";
#endif

#ifdef Q_OS_LINUX
processFgfs = "fgfs";
fgRoot = "--fg-root=/usr/share/flightgear/data";
#endif

processCall << fgRoot;
processCall << fgScenery;
processCall << "--generic=socket,out,50,127.0.0.1,49005,udp,qgroundcontrol";
processCall << "--generic=socket,in,50,127.0.0.1,49000,udp,qgroundcontrol";
processCall << "--in-air";
processCall << "--vc=90";
processCall << "--heading=300";
processCall << "--timeofday=noon";
processCall << "--disable-hud-3d";
processCall << "--control=mouse";
processCall << "--disable-intro-music";
processCall << "--disable-sound";
processCall << "--disable-anti-alias-hud";
processCall << "--disable-fullscreen";
processCall << "--disable-random-objects";
processCall << "--disable-ai-models";
processCall << "--wind=0@0";
processCall << "--fdm=jsb";
processCall << "--prop:/engines/engine[0]/running=true";
if (mav->getSystemType() == MAV_TYPE_QUADROTOR)
{
    // Start the remaining three motors of the quad
    processCall << "--prop:/engines/engine[1]/running=true";
    processCall << "--prop:/engines/engine[2]/running=true";
    processCall << "--prop:/engines/engine[3]/running=true";
}
processCall << QString("--lat=%1").arg(UASManager::instance()->getHomeLatitude());
processCall << QString("--lon=%1").arg(UASManager::instance()->getHomeLongitude());
processCall << QString("--altitude=%1").arg(UASManager::instance()->getHomeAltitude());
// Add new argument with this: processCall << "";
processCall << QString("--aircraft=%2").arg(aircraft);

process->start(processFgfs, processCall);

qDebug() << "STARTING: " << processFgfs << processCall;

//if (!process->waitForStarted())
//{
//    qDebug() << "PROCESS START FAILED!";
//}

    emit flightGearConnected(connectState);
    if (connectState) {
        emit flightGearConnected();
        connectionStartTime = QGC::groundTimeUsecs()/1000;
    }
    qDebug() << "STARTING SIM";

    start(LowPriority);
    return connectState;
}

/**
 * @brief Check if connection is active.
 *
 * @return True if link is connected, false otherwise.
 **/
bool QGCFlightGearLink::isConnected()
{
    return connectState;
}

QString QGCFlightGearLink::getName()
{
    return name;
}

void QGCFlightGearLink::setName(QString name)
{
    this->name = name;
    //    emit nameChanged(this->name);
}
