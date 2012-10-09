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
    remoteHost(QHostAddress("127.0.0.1")),
    remotePort(49000),
    socket(NULL),
    process(NULL),
    terraSync(NULL),
    airframeID(QGCXPlaneLink::AIRFRAME_UNKNOWN),
    xPlaneConnected(false),
    xPlaneVersion(0)
{
    this->localHost = localHost;
    this->localPort = localPort/*+mav->getUASID()*/;
    this->connectState = false;
    this->name = tr("X-Plane Link (localPort:%1)").arg(localPort);
    setRemoteHost(remoteHost);
    loadSettings();
}

QGCXPlaneLink::~QGCXPlaneLink()
{
    storeSettings();
//    if(connectState) {
//       disconnectSimulation();
//    }
}

void QGCXPlaneLink::loadSettings()
{
    // Load defaults from settings
    QSettings settings;
    settings.sync();
    settings.beginGroup("QGC_XPLANE_LINK");
    setRemoteHost(settings.value("REMOTE_HOST", QString("%1:%2").arg(remoteHost.toString()).arg(remotePort)).toString());
    setVersion(settings.value("XPLANE_VERSION", 10).toInt());
    selectPlane(settings.value("AIRFRAME", "default").toString());
    settings.endGroup();
}

void QGCXPlaneLink::storeSettings()
{
    // Store settings
    QSettings settings;
    settings.beginGroup("QGC_XPLANE_LINK");
    settings.setValue("REMOTE_HOST", QString("%1:%2").arg(remoteHost.toString()).arg(remotePort));
    settings.setValue("XPLANE_VERSION", xPlaneVersion);
    settings.setValue("AIRFRAME", airframeName);
    settings.endGroup();
    settings.sync();
}

void QGCXPlaneLink::setVersion(const QString& version)
{
    unsigned int oldVersion = xPlaneVersion;
    if (version.contains("9"))
    {
        xPlaneVersion = 9;
    }
    else if (version.contains("10"))
    {
        xPlaneVersion = 10;
    }
    else if (version.contains("11"))
    {
        xPlaneVersion = 11;
    }
    else if (version.contains("12"))
    {
        xPlaneVersion = 12;
    }

    if (oldVersion != xPlaneVersion)
    {
        emit versionChanged(QString("X-Plane %1").arg(xPlaneVersion));
    }
}

void QGCXPlaneLink::setVersion(unsigned int version)
{
    bool changed = (xPlaneVersion != version);
    xPlaneVersion = version;
    if (changed) emit versionChanged(QString("X-Plane %1").arg(xPlaneVersion));
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

QString QGCXPlaneLink::getRemoteHost()
{
    return QString("%1:%2").arg(remoteHost.toString()).arg(remotePort);
}

/**
 * @param newHost Hostname in standard formatting, e.g. localhost:14551 or 192.168.1.1:14551
 */
void QGCXPlaneLink::setRemoteHost(const QString& newHost)
{
    if (newHost.length() == 0)
        return;

    if (newHost.contains(":"))
    {
        //qDebug() << "HOST: " << newHost.split(":").first();
        QHostInfo info = QHostInfo::fromName(newHost.split(":").first());
        if (info.error() == QHostInfo::NoError)
        {
            // Add newHost
            QList<QHostAddress> newHostAddresses = info.addresses();
            QHostAddress address;
            for (int i = 0; i < newHostAddresses.size(); i++)
            {
                // Exclude loopback IPv4 and all IPv6 addresses
                if (!newHostAddresses.at(i).toString().contains(":"))
                {
                    address = newHostAddresses.at(i);
                }
            }
            remoteHost = address;
            //qDebug() << "Address:" << address.toString();
            // Set localPort according to user input
            remotePort = newHost.split(":").last().toInt();
        }
    }
    else
    {
        QHostInfo info = QHostInfo::fromName(newHost);
        if (info.error() == QHostInfo::NoError)
        {
            // Add newHost
            remoteHost = info.addresses().first();
            if (remotePort == 0) remotePort = 49000;
        }
    }

    if (isConnected())
    {
        disconnectSimulation();
        connectSimulation();
    }

    emit remoteChanged(QString("%1:%2").arg(remoteHost.toString()).arg(remotePort));
}

void QGCXPlaneLink::updateActuators(uint64_t time, float act1, float act2, float act3, float act4, float act5, float act6, float act7, float act8)
{
    // Only update this for multirotors
    if (airframeID == AIRFRAME_QUAD_X_MK_10INCH_I2C ||
        airframeID == AIRFRAME_QUAD_X_ARDRONE ||
        airframeID == AIRFRAME_QUAD_DJI_F450_PWM)
    {

        Q_UNUSED(time);
        Q_UNUSED(act5);
        Q_UNUSED(act6);
        Q_UNUSED(act7);
        Q_UNUSED(act8);

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

        p.index = 25;
        memset(p.f, 0, sizeof(p.f));

        if (airframeID == AIRFRAME_QUAD_X_MK_10INCH_I2C)
        {
            p.f[0] = act1 / 255.0f;
            p.f[1] = act2 / 255.0f;
            p.f[2] = act3 / 255.0f;
            p.f[3] = act4 / 255.0f;
        }
        else if (airframeID == AIRFRAME_QUAD_X_ARDRONE)
        {
            p.f[0] = act1 / 500.0f;
            p.f[1] = act2 / 500.0f;
            p.f[2] = act3 / 500.0f;
            p.f[3] = act4 / 500.0f;
        }
        else
        {
            p.f[0] = (act1 - 1000.0f) / 1000.0f;
            p.f[1] = (act2 - 1000.0f) / 1000.0f;
            p.f[2] = (act3 - 1000.0f) / 1000.0f;
            p.f[3] = (act4 - 1000.0f) / 1000.0f;
        }
        // Throttle
        writeBytes((const char*)&p, sizeof(p));
    }
}

void QGCXPlaneLink::updateControls(uint64_t time, float rollAilerons, float pitchElevator, float yawRudder, float throttle, uint8_t systemMode, uint8_t navMode)
{
    // Do not update this control type for
    // all multirotors

    if (airframeID == AIRFRAME_QUAD_X_MK_10INCH_I2C ||
        airframeID == AIRFRAME_QUAD_X_ARDRONE ||
        airframeID == AIRFRAME_QUAD_DJI_F450_PWM)
    {
        return;
    }

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
    p.f[0] = pitchElevator;
    p.f[1] = rollAilerons;
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
    p.f[0] = throttle;
    p.f[1] = throttle;
    p.f[2] = throttle;
    p.f[3] = throttle;
    // Throttle
    writeBytes((const char*)&p, sizeof(p));
}

void QGCXPlaneLink::writeBytes(const char* data, qint64 size)
{
    if (!data) return;
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
    // Only emit updates on attitude message
    bool emitUpdate = false;

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

    bool oldConnectionState = xPlaneConnected;

    if (data[0] == 'D' &&
            data[1] == 'A' &&
            data[2] == 'T' &&
            data[3] == 'A')
    {
        xPlaneConnected = true;

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
            // Forward controls from X-Plane to MAV, not very useful
            // better: Connect Joystick to QGroundControl
//            else if (p.index == 8)
//            {
//                //qDebug() << "MAN:" << p.f[0] << p.f[3] << p.f[7];
//                man_roll = p.f[0];
//                man_pitch = p.f[1];
//                man_yaw = p.f[2];
//                UAS* uas = dynamic_cast<UAS*>(mav);
//                if (uas) uas->setManualControlCommands(man_roll, man_pitch, man_yaw, 0.6);
//            }
            else if (xPlaneVersion == 10 && p.index == 16)
            {
                //qDebug() << "ANG VEL:" << p.f[0] << p.f[3] << p.f[7];
                rollspeed = p.f[2];
                pitchspeed = p.f[1];
                yawspeed = p.f[0];
            }
            else if ((xPlaneVersion == 10 && p.index == 17) || (xPlaneVersion == 9 && p.index == 18))
            {
                //qDebug() << "HDNG" << "pitch" << p.f[0] << "roll" << p.f[1] << "hding true" << p.f[2] << "hding mag" << p.f[3];
                pitch = p.f[0] / 180.0f * M_PI;
                roll = p.f[1] / 180.0f * M_PI;
                yaw = p.f[2] / 180.0f * M_PI;
                emitUpdate = true;
            }
            else if ((xPlaneVersion == 9 && p.index == 17))
            {
                rollspeed = p.f[2];
                pitchspeed = p.f[1];
                yawspeed = p.f[0];
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
    else if (data[0] == 'S' &&
             data[1] == 'N' &&
             data[2] == 'A' &&
             data[3] == 'P')
    {

    }
    else if (data[0] == 'S' &&
               data[1] == 'T' &&
               data[2] == 'A' &&
               data[3] == 'T')
    {

    }
    else
    {
        qDebug() << "UNKNOWN PACKET:" << data;
    }

    // Send updated state
    if (emitUpdate)
    {
        emit hilStateChanged(QGC::groundTimeUsecs(), roll, pitch, yaw, rollspeed,
                         pitchspeed, yawspeed, lat*1E7, lon*1E7, alt*1E3,
                         vx, vy, vz, xacc*1000, yacc*1000, zacc*1000);
    }

    if (!oldConnectionState && xPlaneConnected)
    {
        emit statusMessage("Receiving from XPlane.");
    }

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
        disconnect(mav, SIGNAL(hilActuatorsChanged(uint64_t, float, float, float, float, float, float, float, float)), this, SLOT(updateActuators(uint64_t,float,float,float,float,float,float,float,float)));
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
    airframeName = plane;

    if (plane.contains("QRO"))
    {
        if (plane.contains("MK") && airframeID != AIRFRAME_QUAD_X_MK_10INCH_I2C)
        {
            airframeID = AIRFRAME_QUAD_X_MK_10INCH_I2C;
            emit airframeChanged("QRO_X / MK");
        }
        else if (plane.contains("ARDRONE") && airframeID != AIRFRAME_QUAD_X_ARDRONE)
        {
            airframeID = AIRFRAME_QUAD_X_ARDRONE;
            emit airframeChanged("QRO_X / ARDRONE");
        }
        else
        {
            bool changed = (airframeID != AIRFRAME_QUAD_DJI_F450_PWM);
            airframeID = AIRFRAME_QUAD_DJI_F450_PWM;
            if (changed) emit airframeChanged("QRO_X / DJI-F450 / PWM");
        }
    }
    else
    {
        bool changed = (airframeID != AIRFRAME_UNKNOWN);
        airframeID = AIRFRAME_UNKNOWN;
        if (changed) emit airframeChanged("X Plane default");
    }
}

void QGCXPlaneLink::setPositionAttitude(double lat, double lon, double alt, double roll, double pitch, double yaw)
{
    #pragma pack(push, 1)
    struct VEH1_struct
    {
        char header[5];
        quint32 p;
        double lat_lon_ele[3];
        float psi_the_phi[3];
        float gear_flap_vect[3];
    } pos;
    #pragma pack(pop)

    pos.header[0] = 'V';
    pos.header[1] = 'E';
    pos.header[2] = 'H';
    pos.header[3] = '1';
    pos.header[4] = '0';
    pos.p = 0;
    pos.lat_lon_ele[0] = lat;
    pos.lat_lon_ele[1] = lon;
    pos.lat_lon_ele[2] = alt;
    pos.psi_the_phi[0] = roll;
    pos.psi_the_phi[1] = pitch;
    pos.psi_the_phi[2] = yaw;
    pos.gear_flap_vect[0] = 0.0f;
    pos.gear_flap_vect[1] = 0.0f;
    pos.gear_flap_vect[2] = 0.0f;

    writeBytes((const char*)&pos, sizeof(pos));

//    pos.header[0] = 'V';
//    pos.header[1] = 'E';
//    pos.header[2] = 'H';
//    pos.header[3] = '1';
//    pos.header[4] = '0';
//    pos.p = 0;
//    pos.lat_lon_ele[0] = -999;
//    pos.lat_lon_ele[1] = -999;
//    pos.lat_lon_ele[2] = -999;
//    pos.psi_the_phi[0] = -999;
//    pos.psi_the_phi[1] = -999;
//    pos.psi_the_phi[2] = -999;
//    pos.gear_flap_vect[0] = -999;
//    pos.gear_flap_vect[1] = -999;
//    pos.gear_flap_vect[2] = -999;

//    writeBytes((const char*)&pos, sizeof(pos));
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
    qDebug() << "STARTING X-PLANE LINK, CONNECTING TO" << remoteHost << ":" << remotePort;

    start(LowPriority);

    if (!mav) return false;
    if (connectState) return false;

    socket = new QUdpSocket(this);
    connectState = socket->bind(localHost, localPort);
    if (!connectState) return false;

    QObject::connect(socket, SIGNAL(readyRead()), this, SLOT(readBytes()));

    //process = new QProcess(this);
    //terraSync = new QProcess(this);

    connect(mav, SIGNAL(hilControlsChanged(uint64_t, float, float, float, float, uint8_t, uint8_t)), this, SLOT(updateControls(uint64_t,float,float,float,float,uint8_t,uint8_t)));
    connect(mav, SIGNAL(hilActuatorsChanged(uint64_t, float, float, float, float, float, float, float, float)), this, SLOT(updateActuators(uint64_t,float,float,float,float,float,float,float,float)));
    connect(this, SIGNAL(hilStateChanged(uint64_t,float,float,float,float,float,float,int32_t,int32_t,int32_t,int16_t,int16_t,int16_t,int16_t,int16_t,int16_t)), mav, SLOT(sendHilState(uint64_t,float,float,float,float,float,float,int32_t,int32_t,int32_t,int16_t,int16_t,int16_t,int16_t,int16_t,int16_t)));

    UAS* uas = dynamic_cast<UAS*>(mav);
    if (uas)
    {
        uas->startHil();
    }

#pragma pack(push, 1)
    struct iset_struct
    {
        char b[5];
        int index; // (0->20 in the lsit below)
        char str_ipad_them[16];
        char str_port_them[6];
        char padding[2];
        int use_ip;
    } ip; // to use this option, 0 not to.
#pragma pack(pop)

    ip.b[0] = 'I';
    ip.b[1] = 'S';
    ip.b[2] = 'E';
    ip.b[3] = 'T';
    ip.b[4] = '0';

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

    //qDebug() << "REQ SEND TO:" << localAddrStr << localPortStr;

    ip.index = 0;
    strncpy(ip.str_ipad_them, localAddrStr.toAscii(), qMin((int)sizeof(ip.str_ipad_them), 16));
    strncpy(ip.str_port_them, localPortStr.toAscii(), qMin((int)sizeof(ip.str_port_them), 6));
    ip.use_ip = 1;

    writeBytes((const char*)&ip, sizeof(ip));
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
