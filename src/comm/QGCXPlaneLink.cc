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
 * @file QGCXPlaneLink.cc
 *   Implementation of X-Plane interface
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#include <QTimer>
#include <QList>
#include <QDebug>
#include <QMutexLocker>
#include <iostream>
#include "QGCXPlaneLink.h"
#include "QGC.h"
#include <QHostInfo>
#include "MainWindow.h"

QGCXPlaneLink::QGCXPlaneLink(UASInterface* mav, QString remoteHost, QHostAddress localHost, quint16 localPort) :
    process(NULL),
    terraSync(NULL)
{
    this->localHost = localHost;
    this->localPort = localPort/*+mav->getUASID()*/;
    this->connectState = false;
    this->mav = mav;
    this->name = tr("X-Plane Link (localPort:%1)").arg(localPort);
    setRemoteHost(remoteHost);
}

QGCXPlaneLink::~QGCXPlaneLink()
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
void QGCXPlaneLink::run()
{
    exec();
}

void QGCXPlaneLink::setPort(int localPort)
{
    this->localPort = localPort;
    disconnectSimulation();
    connectSimulation();
}

void QGCXPlaneLink::processError(QProcess::ProcessError err)
{
    switch(err)
    {
    case QProcess::FailedToStart:
        MainWindow::instance()->showCriticalMessage(tr("X-Plane Failed to Start"), tr("Please check if the path and command is correct"));
        break;
    case QProcess::Crashed:
        MainWindow::instance()->showCriticalMessage(tr("X-Plane Crashed"), tr("This is a X-Plane-related problem. Please upgrade X-Plane"));
        break;
    case QProcess::Timedout:
        MainWindow::instance()->showCriticalMessage(tr("X-Plane Start Timed Out"), tr("Please check if the path and command is correct"));
        break;
    case QProcess::WriteError:
        MainWindow::instance()->showCriticalMessage(tr("Could not Communicate with X-Plane"), tr("Please check if the path and command is correct"));
        break;
    case QProcess::ReadError:
        MainWindow::instance()->showCriticalMessage(tr("Could not Communicate with X-Plane"), tr("Please check if the path and command is correct"));
        break;
    case QProcess::UnknownError:
    default:
        MainWindow::instance()->showCriticalMessage(tr("X-Plane Error"), tr("Please check if the path and command is correct."));
        break;
    }
}

/**
 * @param localHost Hostname in standard formatting, e.g. locallocalHost:14551 or 192.168.1.1:14551
 */
void QGCXPlaneLink::setRemoteHost(const QString& localHost)
{
    if (localHost.contains(":"))
    {
        //qDebug() << "HOST: " << localHost.split(":").first();
        QHostInfo info = QHostInfo::fromName(localHost.split(":").first());
        if (info.error() == QHostInfo::NoError)
        {
            // Add localHost
            QList<QHostAddress> localHostAddresses = info.addresses();
            QHostAddress address;
            for (int i = 0; i < localHostAddresses.size(); i++)
            {
                // Exclude loopback IPv4 and all IPv6 addresses
                if (!localHostAddresses.at(i).toString().contains(":"))
                {
                    address = localHostAddresses.at(i);
                }
            }
            remoteHost = address;
            //qDebug() << "Address:" << address.toString();
            // Set localPort according to user input
            remotePort = localHost.split(":").last().toInt();
        }
    }
    else
    {
        QHostInfo info = QHostInfo::fromName(localHost);
        if (info.error() == QHostInfo::NoError)
        {
            // Add localHost
            remoteHost = info.addresses().first();
        }
    }

}

void QGCXPlaneLink::updateControls(uint64_t time, float rollAilerons, float pitchElevator, float yawRudder, float throttle, uint8_t systemMode, uint8_t navMode)
{
    // magnetos,aileron,elevator,rudder,throttle\n

    //float magnetos = 3.0f;
    Q_UNUSED(time);
    Q_UNUSED(systemMode);
    Q_UNUSED(navMode);

    QString state("%1\t%2\t%3\t%4\t%5\n");
    state = state.arg(rollAilerons).arg(pitchElevator).arg(yawRudder).arg(true).arg(throttle);
    writeBytes(state.toAscii().constData(), state.length());
    qDebug() << "Updated controls" << state;
}

void QGCXPlaneLink::writeBytes(const char* data, qint64 size)
{
    //#define QGCXPlaneLink_DEBUG
#if 1
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
    qDebug() << "Sent" << size << "bytes to" << remoteHost.toString() << ":" << remotePort << "data:";
    qDebug() << bytes;
    qDebug() << "ASCII:" << ascii;
#endif
    if (connectState && socket) socket->writeDatagram(data, size, remoteHost, remotePort);
}

/**
 * @brief Read a number of bytes from the interface.
 *
 * @param data Pointer to the data byte array to write the bytes to
 * @param maxLength The maximum number of bytes to write
 **/
void QGCXPlaneLink::readBytes()
{
    const qint64 maxLength = 65536;
    char data[maxLength];
    QHostAddress sender;
    quint16 senderPort;

    unsigned int s = socket->pendingDatagramSize();
    if (s > maxLength) std::cerr << __FILE__ << __LINE__ << " UDP datagram overflow, allowed to read less bytes than datagram size" << std::endl;
    socket->readDatagram(data, maxLength, &sender, &senderPort);

    QByteArray b(data, s);

    /*// Print string
    QString state(b)*/;

    // Calculate the number of data segments a 36 bytes
    // XPlane always has 5 bytes header: 'DATA@'
    unsigned nsegs = (s-5)/36;

    qDebug() << "XPLANE:" << "LEN:" << s << "segs:" << nsegs;

    union dataconv {
        char c[8];
        float f;
        double d;
        int i;
    } u;

    for (unsigned i = 0; i < nsegs; i++)
    {
        // Get index
        unsigned ioff = (5+i*36);
        memcpy(&(u.i), data+ioff, sizeof(u.i));
        qDebug() << "ind:" << u.i;
        unsigned doff = ioff+sizeof(u.i);
        unsigned dsize = sizeof(u.d);
        memcpy(&(u.d), data+doff, dsize);
        doff += dsize;
        qDebug() << "val1:" << u.d;
    }

    return;

    // Parse string
    double time;
    float roll, pitch, yaw, rollspeed, pitchspeed, yawspeed;
    double lat, lon, alt;
    double vx, vy, vz, xacc, yacc, zacc;
    double airspeed;

//    time = values.at(0).toDouble();
//    lat = values.at(1).toDouble();
//    lon = values.at(2).toDouble();
//    alt = values.at(3).toDouble();
//    roll = values.at(4).toDouble();
//    pitch = values.at(5).toDouble();
//    yaw = values.at(6).toDouble();
//    rollspeed = values.at(7).toDouble();
//    pitchspeed = values.at(8).toDouble();
//    yawspeed = values.at(9).toDouble();

//    xacc = values.at(10).toDouble();
//    yacc = values.at(11).toDouble();
//    zacc = values.at(12).toDouble();

//    vx = values.at(13).toDouble();
//    vy = values.at(14).toDouble();
//    vz = values.at(15).toDouble();

//    airspeed = values.at(16).toDouble();

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
qint64 QGCXPlaneLink::bytesAvailable()
{
    return socket->pendingDatagramSize();
}

/**
 * @brief Disconnect the connection.
 *
 * @return True if connection has been disconnected, false if connection couldn't be disconnected.
 **/
bool QGCXPlaneLink::disconnectSimulation()
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
bool QGCXPlaneLink::connectSimulation()
{
    if (!mav) return false;
    socket = new QUdpSocket(this);
    connectState = socket->bind(localHost, localPort);

    QObject::connect(socket, SIGNAL(readyRead()), this, SLOT(readBytes()));

    //process = new QProcess(this);
    //terraSync = new QProcess(this);

    connect(mav, SIGNAL(hilControlsChanged(uint64_t, float, float, float, float, uint8_t, uint8_t)), this, SLOT(updateControls(uint64_t,float,float,float,float,uint8_t,uint8_t)));
    connect(this, SIGNAL(hilStateChanged(uint64_t,float,float,float,float,float,float,int32_t,int32_t,int32_t,int16_t,int16_t,int16_t,int16_t,int16_t,int16_t)), mav, SLOT(sendHilState(uint64_t,float,float,float,float,float,float,int32_t,int32_t,int32_t,int16_t,int16_t,int16_t,int16_t,int16_t,int16_t)));

    // XXX ugly hack to set MAVLINK_MODE_HIL_FLAG_ENABLED
    // without pulling MAVLINK dependencies in here
    mav->setMode(32);

    // XXX This will later be enabled to start X-Plane from within QGroundControl with the right arguments

//    //connect(&refreshTimer, SIGNAL(timeout()), this, SLOT(sendUAVUpdate()));
//    // Catch process error
//    QObject::connect( process, SIGNAL(error(QProcess::ProcessError)),
//                      this, SLOT(processError(QProcess::ProcessError)));
//    QObject::connect( terraSync, SIGNAL(error(QProcess::ProcessError)),
//                      this, SLOT(processError(QProcess::ProcessError)));
//    // Start X-Plane
//    QStringList processCall;
//    QString processFgfs;
//    QString processTerraSync;
//    QString fgRoot;
//    QString fgScenery;
//    QString aircraft;

//    if (mav->getSystemType() == MAV_TYPE_FIXED_WING)
//    {
//        aircraft = "Rascal110-JSBSim";
//    }
//    else if (mav->getSystemType() == MAV_TYPE_QUADROTOR)
//    {
//        aircraft = "arducopter";
//    }
//    else
//    {
//        aircraft = "Rascal110-JSBSim";
//    }

//#ifdef Q_OS_MACX
//    processFgfs = "/Applications/X-Plane.app/Contents/Resources/fgfs";
//    processTerraSync = "/Applications/X-Plane.app/Contents/Resources/terrasync";
//    fgRoot = "/Applications/X-Plane.app/Contents/Resources/data";
//    //fgScenery = "/Applications/X-Plane.app/Contents/Resources/data/Scenery";
//    fgScenery = "/Applications/X-Plane.app/Contents/Resources/data/Scenery-TerraSync";
//    //   /Applications/X-Plane.app/Contents/Resources/data/Scenery:
//#endif

//#ifdef Q_OS_WIN32
//    processFgfs = "C:\\Program Files (x86)\\X-Plane\\bin\\Win32\\fgfs";
//    fgRoot = "C:\\Program Files (x86)\\X-Plane\\data";
//    fgScenery = "C:\\Program Files (x86)\\X-Plane\\data\\Scenery-Terrasync";
//#endif

//#ifdef Q_OS_LINUX
//    processFgfs = "fgfs";
//    fgRoot = "/usr/share/X-Plane/data";
//    fgScenery = "/usr/share/X-Plane/data/Scenery-Terrasync";
//#endif

//    // Sanity checks
//    bool sane = true;
//    QFileInfo executable(processFgfs);
//    if (!executable.isExecutable())
//    {
//        MainWindow::instance()->showCriticalMessage(tr("X-Plane Failed to Start"), tr("X-Plane was not found at %1").arg(processFgfs));
//        sane = false;
//    }

//    QFileInfo root(fgRoot);
//    if (!root.isDir())
//    {
//        MainWindow::instance()->showCriticalMessage(tr("X-Plane Failed to Start"), tr("X-Plane data directory was not found at %1").arg(fgRoot));
//        sane = false;
//    }

//    QFileInfo scenery(fgScenery);
//    if (!scenery.isDir())
//    {
//        MainWindow::instance()->showCriticalMessage(tr("X-Plane Failed to Start"), tr("X-Plane scenery directory was not found at %1").arg(fgScenery));
//        sane = false;
//    }

//    if (!sane) return false;

//    // --atlas=socket,out,1,locallocalHost,5505,udp
//    // terrasync -p 5505 -S -d /usr/local/share/TerraSync

//    processCall << QString("--fg-root=%1").arg(fgRoot);
//    processCall << QString("--fg-scenery=%1").arg(fgScenery);
//    if (mav->getSystemType() == MAV_TYPE_QUADROTOR)
//    {
//        // FIXME ADD QUAD-Specific protocol here
//        processCall << QString("--generic=socket,out,50,127.0.0.1,%1,udp,qgroundcontrol").arg(localPort);
//        processCall << QString("--generic=socket,in,50,127.0.0.1,%1,udp,qgroundcontrol").arg(remotePort);
//    }
//    else
//    {
//        processCall << QString("--generic=socket,out,50,127.0.0.1,%1,udp,qgroundcontrol").arg(localPort);
//        processCall << QString("--generic=socket,in,50,127.0.0.1,%1,udp,qgroundcontrol").arg(remotePort);
//    }
//    processCall << "--atlas=socket,out,1,locallocalHost,5505,udp";
//    processCall << "--in-air";
//    processCall << "--roll=0";
//    processCall << "--pitch=0";
//    processCall << "--vc=90";
//    processCall << "--heading=300";
//    processCall << "--timeofday=noon";
//    processCall << "--disable-hud-3d";
//    processCall << "--disable-fullscreen";
//    processCall << "--geometry=400x300";
//    processCall << "--disable-anti-alias-hud";
//    processCall << "--wind=0@0";
//    processCall << "--turbulence=0.0";
//    processCall << "--prop:/sim/frame-rate-throttle-hz=30";
//    processCall << "--control=mouse";
//    processCall << "--disable-intro-music";
//    processCall << "--disable-sound";
//    processCall << "--disable-random-objects";
//    processCall << "--disable-ai-models";
//    processCall << "--shading-flat";
//    processCall << "--fog-disable";
//    processCall << "--disable-specular-highlight";
//    //processCall << "--disable-skyblend";
//    processCall << "--disable-random-objects";
//    processCall << "--disable-panel";
//    //processCall << "--disable-horizon-effect";
//    processCall << "--disable-clouds";
//    processCall << "--fdm=jsb";
//    processCall << "--units-meters";
//    if (mav->getSystemType() == MAV_TYPE_QUADROTOR)
//    {
//        // Start all engines of the quad
//        processCall << "--prop:/engines/engine[0]/running=true";
//        processCall << "--prop:/engines/engine[1]/running=true";
//        processCall << "--prop:/engines/engine[2]/running=true";
//        processCall << "--prop:/engines/engine[3]/running=true";
//    }
//    else
//    {
//        processCall << "--prop:/engines/engine/running=true";
//    }
//    processCall << QString("--lat=%1").arg(UASManager::instance()->getHomeLatitude());
//    processCall << QString("--lon=%1").arg(UASManager::instance()->getHomeLongitude());
//    processCall << QString("--altitude=%1").arg(UASManager::instance()->getHomeAltitude());
//    // Add new argument with this: processCall << "";
//    processCall << QString("--aircraft=%2").arg(aircraft);


//    QStringList terraSyncArguments;
//    terraSyncArguments << "-p 5505";
//    terraSyncArguments << "-S";
//    terraSyncArguments << QString("-d=%1").arg(fgScenery);

//    terraSync->start(processTerraSync, terraSyncArguments);
//    process->start(processFgfs, processCall);



//    emit X-PlaneConnected(connectState);
//    if (connectState) {
//        emit X-PlaneConnected();
//        connectionStartTime = QGC::groundTimeUsecs()/1000;
//    }
//    qDebug() << "STARTING SIM";

//        qDebug() << "STARTING: " << processFgfs << processCall;

    qDebug() << "STARTING X-PLANE LINK";

    start(LowPriority);
    return connectState;
}

/**
 * @brief Check if connection is active.
 *
 * @return True if link is connected, false otherwise.
 **/
bool QGCXPlaneLink::isConnected()
{
    return connectState;
}

QString QGCXPlaneLink::getName()
{
    return name;
}

void QGCXPlaneLink::setName(QString name)
{
    this->name = name;
    //    emit nameChanged(this->name);
}
