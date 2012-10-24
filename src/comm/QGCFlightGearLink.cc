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
 *   @see Flightgear Manual http://mapserver.flightgear.org/getstart.pdf
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

QGCFlightGearLink::QGCFlightGearLink(UASInterface* mav, QString remoteHost, QHostAddress host, quint16 port) :
    process(NULL),
    terraSync(NULL),
    flightGearVersion(0)
{
    this->host = host;
    this->port = port+mav->getUASID();
    this->connectState = false;
    this->currentPort = 49000+mav->getUASID();
    this->mav = mav;
    this->name = tr("FlightGear Link (port:%1)").arg(port);
    setRemoteHost(remoteHost);
}

QGCFlightGearLink::~QGCFlightGearLink()
{   //do not disconnect unless it is connected.
    //disconnectSimulation will delete the memory that was allocated for proces, terraSync and socket
    if(connectState){
       disconnectSimulation();
    }
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
        MainWindow::instance()->showCriticalMessage(tr("FlightGear/TerraSync Failed to Start"), tr("Please check if the path and command is correct"));
        break;
    case QProcess::Crashed:
        MainWindow::instance()->showCriticalMessage(tr("FlightGear/TerraSync Crashed"), tr("This is a FlightGear-related problem. Please upgrade FlightGear"));
        break;
    case QProcess::Timedout:
        MainWindow::instance()->showCriticalMessage(tr("FlightGear/TerraSync Start Timed Out"), tr("Please check if the path and command is correct"));
        break;
    case QProcess::WriteError:
        MainWindow::instance()->showCriticalMessage(tr("Could not Communicate with FlightGear/TerraSync"), tr("Please check if the path and command is correct"));
        break;
    case QProcess::ReadError:
        MainWindow::instance()->showCriticalMessage(tr("Could not Communicate with FlightGear/TerraSync"), tr("Please check if the path and command is correct"));
        break;
    case QProcess::UnknownError:
    default:
        MainWindow::instance()->showCriticalMessage(tr("FlightGear/TerraSync Error"), tr("Please check if the path and command is correct."));
        break;
    }
}

/**
 * @param host Hostname in standard formatting, e.g. localhost:14551 or 192.168.1.1:14551
 */
void QGCFlightGearLink::setRemoteHost(const QString& host)
{
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

void QGCFlightGearLink::updateActuators(uint64_t time, float act1, float act2, float act3, float act4, float act5, float act6, float act7, float act8)
{

}

void QGCFlightGearLink::updateControls(uint64_t time, float rollAilerons, float pitchElevator, float yawRudder, float throttle, uint8_t systemMode, uint8_t navMode)
{
    // magnetos,aileron,elevator,rudder,throttle\n

    //float magnetos = 3.0f;
    Q_UNUSED(time);
    Q_UNUSED(systemMode);
    Q_UNUSED(navMode);

    QString state("%1\t%2\t%3\t%4\t%5\n");
    state = state.arg(rollAilerons).arg(pitchElevator).arg(yawRudder).arg(true).arg(throttle);
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
    char data[maxLength];
    QHostAddress sender;
    quint16 senderPort;

    unsigned int s = socket->pendingDatagramSize();
    if (s > maxLength) std::cerr << __FILE__ << __LINE__ << " UDP datagram overflow, allowed to read less bytes than datagram size" << std::endl;
    socket->readDatagram(data, maxLength, &sender, &senderPort);

    QByteArray b(data, s);

    // Print string
    QString state(b);
    //qDebug() << "FG LINK GOT:" << state;

    QStringList values = state.split("\t");

    // Check length
    if (values.size() != 17)
    {
        qDebug() << "RETURN LENGTH MISMATCHING EXPECTED" << 17 << "BUT GOT" << values.size();
        qDebug() << state;
        return;
    }

    // Parse string
    double time;
    float roll, pitch, yaw, rollspeed, pitchspeed, yawspeed;
    double lat, lon, alt;
    double vx, vy, vz, xacc, yacc, zacc;
    double airspeed;

    time = values.at(0).toDouble();
    lat = values.at(1).toDouble() * 1e7;
    lon = values.at(2).toDouble() * 1e7;
    alt = values.at(3).toDouble() * 1e3;
    roll = values.at(4).toDouble();
    pitch = values.at(5).toDouble();
    yaw = values.at(6).toDouble();
    rollspeed = values.at(7).toDouble();
    pitchspeed = values.at(8).toDouble();
    yawspeed = values.at(9).toDouble();

    xacc = values.at(10).toDouble();
    yacc = values.at(11).toDouble();
    zacc = values.at(12).toDouble();

    vx = values.at(13).toDouble() * 1e2;
    vy = values.at(14).toDouble() * 1e2;
    vz = values.at(15).toDouble() * 1e2;

    airspeed = values.at(16).toDouble();

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

    if (process)
    {
        process->close();
        delete process;
        process = NULL;
    }
    if (terraSync)
    {
        terraSync->close();
        delete terraSync;
        terraSync = NULL;
    }
    if (socket)
    {
        socket->close();
        delete socket;
        socket = NULL;
    }

    connectState = false;

    emit simulationDisconnected();
    emit simulationConnected(false);
    return !connectState;
}

/**
 * @brief Connect the connection.
 *
 * @return True if connection has been established, false if connection couldn't be established.
 **/
bool QGCFlightGearLink::connectSimulation()
{
    qDebug() << "STARTING FLIGHTGEAR LINK";

    if (!mav) return false;
    socket = new QUdpSocket(this);
    connectState = socket->bind(host, port);

    QObject::connect(socket, SIGNAL(readyRead()), this, SLOT(readBytes()));

    process = new QProcess(this);
    terraSync = new QProcess(this);

    connect(mav, SIGNAL(hilControlsChanged(uint64_t, float, float, float, float, uint8_t, uint8_t)), this, SLOT(updateControls(uint64_t,float,float,float,float,uint8_t,uint8_t)));
    connect(this, SIGNAL(hilStateChanged(uint64_t,float,float,float,float,float,float,int32_t,int32_t,int32_t,int16_t,int16_t,int16_t,int16_t,int16_t,int16_t)), mav, SLOT(sendHilState(uint64_t,float,float,float,float,float,float,int32_t,int32_t,int32_t,int16_t,int16_t,int16_t,int16_t,int16_t,int16_t)));

    //connect(&refreshTimer, SIGNAL(timeout()), this, SLOT(sendUAVUpdate()));
    // Catch process error
    QObject::connect( process, SIGNAL(error(QProcess::ProcessError)),
                      this, SLOT(processError(QProcess::ProcessError)));
    QObject::connect( terraSync, SIGNAL(error(QProcess::ProcessError)),
                      this, SLOT(processError(QProcess::ProcessError)));
    // Start Flightgear
    QStringList flightGearArguments;
    QString processFgfs;
    QString processTerraSync;
    QString fgRoot;
    QString fgScenery;
    QString terraSyncScenery;
    QString aircraft;

    if (mav->getSystemType() == MAV_TYPE_FIXED_WING)
    {
        aircraft = "Rascal110-JSBSim";
    }
    else if (mav->getSystemType() == MAV_TYPE_QUADROTOR)
    {
        aircraft = "arducopter";
    }
    else
    {
        aircraft = "Rascal110-JSBSim";
    }

#ifdef Q_OS_MACX
    processFgfs = "/Applications/FlightGear.app/Contents/Resources/fgfs";
    processTerraSync = "/Applications/FlightGear.app/Contents/Resources/terrasync";
    fgRoot = "/Applications/FlightGear.app/Contents/Resources/data";
    //fgScenery = "/Applications/FlightGear.app/Contents/Resources/data/Scenery";
    fgScenery = "/Applications/FlightGear.app/Contents/Resources/data/Scenery-TerraSync";
    //   /Applications/FlightGear.app/Contents/Resources/data/Scenery:
#endif

#ifdef Q_OS_WIN32
    processFgfs = "C:\\Program Files (x86)\\FlightGear\\bin\\Win32\\fgfs";
    fgRoot = "C:\\Program Files (x86)\\FlightGear\\data";
    fgScenery = "C:\\Program Files (x86)\\FlightGear\\data\\Scenery-Terrasync";
#endif

#ifdef Q_OS_LINUX
    processFgfs = "/usr/games/fgfs";
    fgRoot = "/usr/share/games/flightgear";
    fgScenery = "/usr/share/games/flightgear/Scenery/";
    processTerraSync = "/usr/bin/nice"; //according to http://wiki.flightgear.org/TerraSync, run with lower priority
    terraSyncScenery = QDir::homePath() + "/.terrasync/Scenery"; //according to http://wiki.flightgear.org/TerraSync a separate directory is used
#endif

    // Sanity checks
    bool sane = true;
    QFileInfo executable(processFgfs);
    if (!executable.isExecutable())
    {
        MainWindow::instance()->showCriticalMessage(tr("FlightGear Failed to Start"), tr("FlightGear was not found at %1").arg(processFgfs));
        sane = false;
    }

    QFileInfo root(fgRoot);
    if (!root.isDir())
    {
        MainWindow::instance()->showCriticalMessage(tr("FlightGear Failed to Start"), tr("FlightGear data directory was not found at %1").arg(fgRoot));
        sane = false;
    }

    QFileInfo scenery(fgScenery);
    if (!scenery.isDir())
    {
        MainWindow::instance()->showCriticalMessage(tr("FlightGear Failed to Start"), tr("FlightGear scenery directory was not found at %1").arg(fgScenery));
        sane = false;
    }

    QFileInfo terraSyncExecutableInfo(processTerraSync);
    if (!terraSyncExecutableInfo.isExecutable())
    {
        MainWindow::instance()->showCriticalMessage(tr("FlightGear Failed to Start"), tr("TerraSync was not found at %1").arg(processTerraSync));
        sane = false;
    }


    if (!sane) return false;

    // --atlas=socket,out,1,localhost,5505,udp
    // terrasync -p 5505 -S -d /usr/local/share/TerraSync

    /*Prepare FlightGear Arguments */
    flightGearArguments << QString("--fg-root=%1").arg(fgRoot);
    flightGearArguments << QString("--fg-scenery=%1:%2").arg(fgScenery).arg(terraSyncScenery); //according to http://wiki.flightgear.org/TerraSync a separate directory is used
    if (mav->getSystemType() == MAV_TYPE_QUADROTOR)
    {
        // FIXME ADD QUAD-Specific protocol here
        flightGearArguments << QString("--generic=socket,out,50,127.0.0.1,%1,udp,qgroundcontrol").arg(port);
        flightGearArguments << QString("--generic=socket,in,50,127.0.0.1,%1,udp,qgroundcontrol").arg(currentPort);
    }
    else
    {
        flightGearArguments << QString("--generic=socket,out,50,127.0.0.1,%1,udp,qgroundcontrol").arg(port);
        flightGearArguments << QString("--generic=socket,in,50,127.0.0.1,%1,udp,qgroundcontrol").arg(currentPort);
    }
    flightGearArguments << "--atlas=socket,out,1,localhost,5505,udp";
    flightGearArguments << "--in-air";
    flightGearArguments << "--roll=0";
    flightGearArguments << "--pitch=0";
    flightGearArguments << "--vc=90";
    flightGearArguments << "--heading=300";
    flightGearArguments << "--timeofday=noon";
    flightGearArguments << "--disable-hud-3d";
    flightGearArguments << "--disable-fullscreen";
    flightGearArguments << "--geometry=400x300";
    flightGearArguments << "--disable-anti-alias-hud";
    flightGearArguments << "--wind=0@0";
    flightGearArguments << "--turbulence=0.0";
    flightGearArguments << "--prop:/sim/frame-rate-throttle-hz=30";
    flightGearArguments << "--control=mouse";
    flightGearArguments << "--disable-intro-music";
    flightGearArguments << "--disable-sound";
    flightGearArguments << "--disable-random-objects";
    flightGearArguments << "--disable-ai-models";
    flightGearArguments << "--shading-flat";
    flightGearArguments << "--fog-disable";
    flightGearArguments << "--disable-specular-highlight";
    //flightGearArguments << "--disable-skyblend";
    flightGearArguments << "--disable-random-objects";
    flightGearArguments << "--disable-panel";
    //flightGearArguments << "--disable-horizon-effect";
    flightGearArguments << "--disable-clouds";
    flightGearArguments << "--fdm=jsb";
    flightGearArguments << "--units-meters"; //XXX: check: the protocol xml has already a conversion from feet to m?
    flightGearArguments << "--notrim";
    if (mav->getSystemType() == MAV_TYPE_QUADROTOR)
    {
        // Start all engines of the quad
        flightGearArguments << "--prop:/engines/engine[0]/running=true";
        flightGearArguments << "--prop:/engines/engine[1]/running=true";
        flightGearArguments << "--prop:/engines/engine[2]/running=true";
        flightGearArguments << "--prop:/engines/engine[3]/running=true";
    }
    else
    {
        flightGearArguments << "--prop:/engines/engine/running=true";
    }
    flightGearArguments << QString("--lat=%1").arg(UASManager::instance()->getHomeLatitude());
    flightGearArguments << QString("--lon=%1").arg(UASManager::instance()->getHomeLongitude());
    flightGearArguments << QString("--altitude=%1").arg(UASManager::instance()->getHomeAltitude());
    // Add new argument with this: flightGearArguments << "";
    flightGearArguments << QString("--aircraft=%2").arg(aircraft);

    /*Prepare TerraSync Arguments */
    QStringList terraSyncArguments;
#ifdef Q_OS_LINUX
    terraSyncArguments << "/usr/games/terrasync";
#endif
    terraSyncArguments << "-p";
    terraSyncArguments << "5505";
    terraSyncArguments << "-S";
    terraSyncArguments << "-d";
    terraSyncArguments << terraSyncScenery; //according to http://wiki.flightgear.org/TerraSync a separate directory is used

//    connect (terraSync, SIGNAL(readyReadStandardOutput()), this, SLOT(printTerraSyncOutput()));
//    connect (terraSync, SIGNAL(readyReadStandardError()), this, SLOT(printTerraSyncError()));
    terraSync->start(processTerraSync, terraSyncArguments);
//    qDebug() << "STARTING: " << processTerraSync << terraSyncArguments;

    process->start(processFgfs, flightGearArguments);



    emit simulationConnected(connectState);
    if (connectState) {
        emit simulationConnected();
        connectionStartTime = QGC::groundTimeUsecs()/1000;
    }
    qDebug() << "STARTING SIM";

//    qDebug() << "STARTING: " << processFgfs << flightGearArguments;


    start(LowPriority);
    return connectState;
}

void QGCFlightGearLink::printTerraSyncOutput()
{
   qDebug() << "TerraSync stdout:";
   QByteArray byteArray = terraSync->readAllStandardOutput();
   QStringList strLines = QString(byteArray).split("\n");

   foreach (QString line, strLines){
    qDebug() << line;
   }
}

void QGCFlightGearLink::printTerraSyncError()
{
   qDebug() << "TerraSync stderr:";

   QByteArray byteArray = terraSync->readAllStandardError();
   QStringList strLines = QString(byteArray).split("\n");

   foreach (QString line, strLines){
    qDebug() << line;
   }
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

QString QGCFlightGearLink::getRemoteHost()
{
    return QString("%1:%2").arg(currentHost.toString(), currentPort);
}

void QGCFlightGearLink::setName(QString name)
{
    this->name = name;
    //    emit nameChanged(this->name);
}
