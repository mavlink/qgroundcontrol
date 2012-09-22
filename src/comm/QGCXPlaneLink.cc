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
#include <QNetworkInterface>
#include <iostream>
#include "QGCXPlaneLink.h"
#include "QGC.h"
#include <QHostInfo>
#include "UAS.h"
#include "MainWindow.h"

QGCXPlaneLink::QGCXPlaneLink(UASInterface* mav, QString remoteHost, QHostAddress localHost, quint16 localPort) :
    mav(mav),
    socket(NULL),
    process(NULL),
    terraSync(NULL)
{
    this->localHost = localHost;
    this->localPort = localPort/*+mav->getUASID()*/;
    this->connectState = false;
    this->name = tr("X-Plane Link (localPort:%1)").arg(localPort);
    setRemoteHost(remoteHost);
}

QGCXPlaneLink::~QGCXPlaneLink()
{
//    if(connectState) {
//       disconnectSimulation();
//    }
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

    //    // Send request to send correct data
    //#pragma pack(push, 1)
    //    struct payload {
    //        char b[5];
    //        int index;
    //        float f[8];
    //    } p;
    //#pragma pack(pop)

    //    p.b[0] = 'D';
    //    p.b[1] = 'A';
    //    p.b[2] = 'T';
    //    p.b[3] = 'A';
    //    p.b[4] = '\0';

    //    p.f[0]

    //    writeBytes((const char*)&p, sizeof(p));

#pragma pack(push, 1)
    struct iset_struct{
        char b[5];
        quint8 index; // (0->20 in the lsit below)
        char str_ipad_them[16]; // IP's we are sending to, in english
        char str_port_them[6]; // ports are easier to work with in STRINGS!
        char use_ip ;} ip; // to use this option, 0 not to.
#pragma pack(pop)

    ip.b[0] = 'I';
    ip.b[1] = 'S';
    ip.b[2] = 'E';
    ip.b[3] = 'T';
    ip.b[4] = '\0';

    QList<QHostAddress> hostAddresses = QNetworkInterface::allAddresses();

    QString localAddrStr;
    QString localPortStr = QString("%1").arg(localPort);

    for (int i = 0; i < hostAddresses.size(); i++)
    {
        // Exclude loopback IPv4 and all IPv6 addresses
        if (hostAddresses.at(i) != QHostAddress("127.0.0.1") && !hostAddresses.at(i).toString().contains(":"))
        {
            localAddrStr = hostAddresses.at(i).toString();
            break;
        }
    }

    qDebug() << "REQ SEND TO:" << localAddrStr << localPortStr;

    ip.index = 0;
    strncpy(ip.str_ipad_them, localAddrStr.toAscii(), sizeof(ip.str_ipad_them));
    strncpy(ip.str_port_them, localPortStr.toAscii(), sizeof(ip.str_port_them));
    ip.use_ip = 1;
}

void QGCXPlaneLink::updateControls(uint64_t time, float rollAilerons, float pitchElevator, float yawRudder, float throttle, uint8_t systemMode, uint8_t navMode)
{

    #pragma pack(push, 1)
    struct payload {
        char b[5];
        int index;
        float f[8];
    } p;
    #pragma pack(pop)

    p.b[0] = 'D';
    p.b[1] = 'A';
    p.b[2] = 'T';
    p.b[3] = 'A';
    p.b[4] = '\0';

    p.index = 12;
    p.f[0] = rollAilerons;
    p.f[1] = pitchElevator;
    p.f[2] = yawRudder;

    Q_UNUSED(time);
    Q_UNUSED(systemMode);
    Q_UNUSED(navMode);

    // Ail / Elevon / Rudder
    writeBytes((const char*)&p, sizeof(p));
    p.index = 8;
    writeBytes((const char*)&p, sizeof(p));

    p.index = 25;
    memset(p.f, 0, sizeof(p.f));
    p.f[0] = 0.5f;//throttle;
    p.f[1] = 0.5f;//throttle;
    p.f[2] = 0.5f;//throttle;
    p.f[3] = 0.5f;//throttle;
    // Throttle
    writeBytes((const char*)&p, sizeof(p));

    qDebug() << "CTRLS SENT:" << rollAilerons;
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
    //qDebug() << "Sent" << size << "bytes to" << remoteHost.toString() << ":" << remotePort << "data:";
    //qDebug() << bytes;
    //qDebug() << "ASCII:" << ascii;
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

    //qDebug() << "XPLANE:" << "LEN:" << s << "segs:" << nsegs;

    #pragma pack(push, 1)
    struct payload {
        int index;
        float f[8];
    } p;
    #pragma pack(pop)

    float roll, pitch, yaw, rollspeed, pitchspeed, yawspeed;
    double lat, lon, alt;
    float vx, vy, vz, xacc, yacc, zacc;
    float airspeed;
    float groundspeed;

    float man_roll, man_pitch, man_yaw;

    if (data[0] == 'D' &&
            data[1] == 'A' &&
            data[2] == 'T' &&
            data[3] == 'A')
    {

        for (unsigned i = 0; i < nsegs; i++)
        {
            // Get index
            unsigned ioff = (5+i*36);;
            memcpy(&(p), data+ioff, sizeof(p));

            if (p.index == 3)
            {
                airspeed = p.f[6] * 0.44704f;
                groundspeed = p.f[7] * 0.44704;

                //qDebug() << "SPEEDS:" << "airspeed" << airspeed << "m/s, groundspeed" << groundspeed << "m/s";
            }
            else if (p.index == 8)
            {
                //qDebug() << "MAN:" << p.f[0] << p.f[3] << p.f[7];
                man_roll = p.f[0];
                man_pitch = p.f[1];
                man_yaw = p.f[2];
                UAS* uas = dynamic_cast<UAS*>(mav);
                if (uas) uas->setManualControlCommands(man_roll, man_pitch, man_yaw, 0.6);
            }
            else if (p.index == 16)
            {
                //qDebug() << "ANG VEL:" << p.f[0] << p.f[3] << p.f[7];
                rollspeed = p.f[2];
                pitchspeed = p.f[1];
                yawspeed = p.f[0];
            }
            else if (p.index == 17)
            {
                //qDebug() << "HDNG" << "pitch" << p.f[0] << "roll" << p.f[1] << "hding true" << p.f[2] << "hding mag" << p.f[3];
                // XXX Feeding true heading - might need fix
                pitch = (p.f[0] - 180.0f) / 180.0f * M_PI;
                roll = (p.f[1] - 180.0f) / 180.0f * M_PI;
                yaw = (p.f[2] - 180.0f) / 180.0f * M_PI;
            }

//            else if (p.index == 19)
//            {
//                qDebug() << "ATT:" << p.f[0] << p.f[1] << p.f[2];
//            }
            else if (p.index == 20)
            {
                //qDebug() << "LAT/LON/ALT:" << p.f[0] << p.f[1] << p.f[2];
                lat = p.f[0];
                lon = p.f[1];
                alt = p.f[2] * 0.3048f; // convert feet (MSL) to meters
            }
            else if (p.index == 12)
            {
                //qDebug() << "AIL/ELEV/RUD" << p.f[0] << p.f[1] << p.f[2];
            }
            else if (p.index == 25)
            {
                //qDebug() << "THROTTLE" << p.f[0] << p.f[1] << p.f[2] << p.f[3];
            }
            else if (p.index == 0)
            {
                //qDebug() << "STATS" << "fgraphics/s" << p.f[0] << "fsim/s" << p.f[2] << "t frame" << p.f[3] << "cpu load" << p.f[4] << "grnd ratio" << p.f[5] << "filt ratio" << p.f[6];
            }
            else if (p.index == 11)
            {
                //qDebug() << "CONTROLS" << "ail" << p.f[0] << "elev" << p.f[1] << "rudder" << p.f[2] << "nwheel" << p.f[3];
            }
            else
            {
                //qDebug() << "UNKNOWN #" << p.index << p.f[0] << p.f[1] << p.f[2] << p.f[3];
            }
        }
    }
    else
    {
        qDebug() << "UNKNOWN PACKET:" << data;
    }

    // Send updated state
    emit hilStateChanged(QGC::groundTimeUsecs(), roll, pitch, yaw, rollspeed,
                         pitchspeed, yawspeed, lat*1E7, lon*1E7, alt*1E3,
                         vx, vy, vz, xacc*1000, yacc*1000, zacc*1000);

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
    if (!connectState) return true;

    connectState = false;

    if (process) disconnect(process, SIGNAL(error(QProcess::ProcessError)),
               this, SLOT(processError(QProcess::ProcessError)));
    if (mav)
    {
        disconnect(mav, SIGNAL(hilControlsChanged(uint64_t, float, float, float, float, uint8_t, uint8_t)), this, SLOT(updateControls(uint64_t,float,float,float,float,uint8_t,uint8_t)));
        disconnect(this, SIGNAL(hilStateChanged(uint64_t,float,float,float,float,float,float,int32_t,int32_t,int32_t,int16_t,int16_t,int16_t,int16_t,int16_t,int16_t)), mav, SLOT(sendHilState(uint64_t,float,float,float,float,float,float,int32_t,int32_t,int32_t,int16_t,int16_t,int16_t,int16_t,int16_t,int16_t)));
        UAS* uas = dynamic_cast<UAS*>(mav);
        if (uas)
        {
            uas->stopHil();
        }
    }

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

    emit simulationDisconnected();
    emit simulationConnected(false);
    return !connectState;
}

void QGCXPlaneLink::selectPlane(const QString& plane)
{

}

void QGCXPlaneLink::setPositionAttitude(double lat, double lon, double alt, double roll, double pitch, double yaw)
{
    struct VEH1_struct
    {
        quint32 p;
        double lat_lon_ele[3];
        float psi_the_phi[3];
        float gear_flap_vect[3];
    } pos;

    pos.p = 0;
    pos.lat_lon_ele[0] = lat;
    pos.lat_lon_ele[1] = lon;
    pos.lat_lon_ele[2] = alt;
}

/**
 * Sets a random position with an offset of max 1/1000 degree
 * and max 100 m altitude
 */
void QGCXPlaneLink::setRandomPosition()
{
    // Initialize generator
    srand(0);

    double offLat = rand() / static_cast<double>(RAND_MAX) / 500.0 + 1.0/500.0;
    double offLon = rand() / static_cast<double>(RAND_MAX) / 500.0 + 1.0/500.0;
    double offAlt = rand() / static_cast<double>(RAND_MAX) * 200.0 + 100.0;

    if (mav->getAltitude() + offAlt < 0)
    {
        offAlt *= -1.0;
    }

    setPositionAttitude(mav->getLatitude() + offLat,
                        mav->getLongitude() + offLon,
                        mav->getAltitude() + offAlt,
                        mav->getRoll(),
                        mav->getPitch(),
                        mav->getYaw());
}

void QGCXPlaneLink::setRandomAttitude()
{
    // Initialize generator
    srand(0);

    double roll = rand() / static_cast<double>(RAND_MAX) * 2.0 - 1.0;
    double pitch = rand() / static_cast<double>(RAND_MAX) * 2.0 - 1.0;
    double yaw = rand() / static_cast<double>(RAND_MAX) * 2.0 - 1.0;

    setPositionAttitude(mav->getLatitude(),
                        mav->getLongitude(),
                        mav->getAltitude(),
                        roll,
                        pitch,
                        yaw);
}

/**
 * @brief Connect the connection.
 *
 * @return True if connection has been established, false if connection couldn't be established.
 **/
bool QGCXPlaneLink::connectSimulation()
{
    if (!mav) return false;
    if (connectState) return false;

    socket = new QUdpSocket(this);
    connectState = socket->bind(localHost, localPort);
    if (!connectState) return false;

    QObject::connect(socket, SIGNAL(readyRead()), this, SLOT(readBytes()));

    //process = new QProcess(this);
    //terraSync = new QProcess(this);

    connect(mav, SIGNAL(hilControlsChanged(uint64_t, float, float, float, float, uint8_t, uint8_t)), this, SLOT(updateControls(uint64_t,float,float,float,float,uint8_t,uint8_t)));
    connect(this, SIGNAL(hilStateChanged(uint64_t,float,float,float,float,float,float,int32_t,int32_t,int32_t,int16_t,int16_t,int16_t,int16_t,int16_t,int16_t)), mav, SLOT(sendHilState(uint64_t,float,float,float,float,float,float,int32_t,int32_t,int32_t,int16_t,int16_t,int16_t,int16_t,int16_t,int16_t)));

    UAS* uas = dynamic_cast<UAS*>(mav);
    if (uas)
    {
        uas->startHil();
    }

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

    qDebug() << "STARTING X-PLANE LINK, CONNECTING TO" << remoteHost << ":" << remotePort;

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
