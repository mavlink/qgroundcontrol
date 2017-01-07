/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/**
 * @file QGCXPlaneLink.cc
 *   Implementation of X-Plane interface
 *   @author Lorenz Meier <lm@qgroundcontrol.org>
 *
 */

#include <QTimer>
#include <QList>
#include <QDebug>
#include <QMutexLocker>
#include <QNetworkInterface>
#include <QHostInfo>

#include <iostream>
#include <Eigen/Eigen>

#include "QGCXPlaneLink.h"
#include "QGC.h"
#include "UAS.h"
#include "UASInterface.h"
#include "QGCMessageBox.h"

QGCXPlaneLink::QGCXPlaneLink(Vehicle* vehicle, QString remoteHost, QHostAddress localHost, quint16 localPort) :
    _vehicle(vehicle),
    remoteHost(QHostAddress("127.0.0.1")),
    remotePort(49000),
    socket(NULL),
    process(NULL),
    terraSync(NULL),
    barometerOffsetkPa(-8.0f),
    airframeID(QGCXPlaneLink::AIRFRAME_UNKNOWN),
    xPlaneConnected(false),
    xPlaneVersion(0),
    simUpdateLast(QGC::groundTimeMilliseconds()),
    simUpdateFirst(0),
    simUpdateLastText(QGC::groundTimeMilliseconds()),
    simUpdateLastGroundTruth(QGC::groundTimeMilliseconds()),
    simUpdateHz(0),
    _sensorHilEnabled(true),
    _useHilActuatorControls(true),
    _should_exit(false)
{
    // We're doing it wrong - because the Qt folks got the API wrong:
    // http://blog.qt.digia.com/blog/2010/06/17/youre-doing-it-wrong/
    moveToThread(this);

    setTerminationEnabled(false);

    this->localHost = localHost;
    this->localPort = localPort/*+mav->getUASID()*/;
    connectState = false;

    this->name = tr("X-Plane Link (localPort:%1)").arg(localPort);
    setRemoteHost(remoteHost);
    loadSettings();
}

QGCXPlaneLink::~QGCXPlaneLink()
{
    storeSettings();
    // Tell the thread to exit
    _should_exit = true;

    if (socket) {
        socket->close();
        socket->deleteLater();
        socket = NULL;
    }
}

void QGCXPlaneLink::loadSettings()
{
    // Load defaults from settings
    QSettings settings;
    settings.beginGroup("QGC_XPLANE_LINK");
    setRemoteHost(settings.value("REMOTE_HOST", QString("%1:%2").arg(remoteHost.toString()).arg(remotePort)).toString());
    setVersion(settings.value("XPLANE_VERSION", 10).toInt());
    selectAirframe(settings.value("AIRFRAME", "default").toString());
    _sensorHilEnabled = settings.value("SENSOR_HIL", _sensorHilEnabled).toBool();
    _useHilActuatorControls = settings.value("ACTUATOR_HIL", _useHilActuatorControls).toBool();
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
    settings.setValue("SENSOR_HIL", _sensorHilEnabled);
    settings.setValue("ACTUATOR_HIL", _useHilActuatorControls);
    settings.endGroup();
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

void QGCXPlaneLink::enableHilActuatorControls(bool enable)
{
    if (enable != _useHilActuatorControls) {
        _useHilActuatorControls = enable;
    }

    /* Only use override for new message and specific airframes */
    MAV_TYPE type = _vehicle->vehicleType();
    float value = 0.0f;
    if (type == MAV_TYPE_VTOL_RESERVED2) {
        value = (enable ? 1.0f : 0.0f);
    }

    sendDataRef("sim/operation/override/override_control_surfaces", value);
    emit useHilActuatorControlsChanged(enable);
}


/**
 * @brief Runs the thread
 *
 **/
void QGCXPlaneLink::run()
{
    if (!_vehicle) {
        emit statusMessage("No MAV present");
        return;
    }

    if (connectState) {
        emit statusMessage("Already connected");
        return;
    }

    socket = new QUdpSocket(this);
    socket->moveToThread(this);
    connectState = socket->bind(localHost, localPort, QAbstractSocket::ReuseAddressHint);
    if (!connectState) {

        emit statusMessage("Binding socket failed!");

        socket->deleteLater();
        socket = NULL;
        return;
    }

    emit statusMessage(tr("Waiting for XPlane.."));

    QObject::connect(socket, &QUdpSocket::readyRead, this, &QGCXPlaneLink::readBytes);

    connect(_vehicle->uas(), &UAS::hilControlsChanged, this, &QGCXPlaneLink::updateControls, Qt::QueuedConnection);
    connect(_vehicle, &Vehicle::hilActuatorControlsChanged, this, &QGCXPlaneLink::updateActuatorControls, Qt::QueuedConnection);

    connect(this, &QGCXPlaneLink::hilGroundTruthChanged, _vehicle->uas(), &UAS::sendHilGroundTruth, Qt::QueuedConnection);
    connect(this, &QGCXPlaneLink::hilStateChanged, _vehicle->uas(), &UAS::sendHilState, Qt::QueuedConnection);
    connect(this, &QGCXPlaneLink::sensorHilGpsChanged, _vehicle->uas(), &UAS::sendHilGps, Qt::QueuedConnection);
    connect(this, &QGCXPlaneLink::sensorHilRawImuChanged, _vehicle->uas(), &UAS::sendHilSensors, Qt::QueuedConnection);

    _vehicle->uas()->startHil();

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

    ip.index = 0;
    strncpy(ip.str_ipad_them, localAddrStr.toLatin1(), qMin((int)sizeof(ip.str_ipad_them), 16));
    strncpy(ip.str_port_them, localPortStr.toLatin1(), qMin((int)sizeof(ip.str_port_them), 6));
    ip.use_ip = 1;

    writeBytesSafe((const char*)&ip, sizeof(ip));

    /* Call function which makes sure individual control override is enabled/disabled */
    enableHilActuatorControls(_useHilActuatorControls);

    _should_exit = false;

    while(!_should_exit) {
        QCoreApplication::processEvents();
        QGC::SLEEP::msleep(5);
    }

    disconnect(_vehicle->uas(), &UAS::hilControlsChanged, this, &QGCXPlaneLink::updateControls);

    disconnect(this, &QGCXPlaneLink::hilGroundTruthChanged, _vehicle->uas(), &UAS::sendHilGroundTruth);
    disconnect(this, &QGCXPlaneLink::hilStateChanged, _vehicle->uas(), &UAS::sendHilState);
    disconnect(this, &QGCXPlaneLink::sensorHilGpsChanged, _vehicle->uas(), &UAS::sendHilGps);
    disconnect(this, &QGCXPlaneLink::sensorHilRawImuChanged, _vehicle->uas(), &UAS::sendHilSensors);
    connectState = false;

    disconnect(socket, &QUdpSocket::readyRead, this, &QGCXPlaneLink::readBytes);

    socket->close();
    socket->deleteLater();
    socket = NULL;

    emit simulationDisconnected();
    emit simulationConnected(false);
}

void QGCXPlaneLink::setPort(int localPort)
{
    this->localPort = localPort;
    disconnectSimulation();
    connectSimulation();
}

void QGCXPlaneLink::processError(QProcess::ProcessError err)
{
    QString msg;
    
    switch(err) {
        case QProcess::FailedToStart:
            msg = tr("X-Plane Failed to start. Please check if the path and command is correct");
            break;
            
        case QProcess::Crashed:
            msg = tr("X-Plane crashed. This is an X-Plane-related problem, check for X-Plane upgrade.");
            break;
            
        case QProcess::Timedout:
            msg = tr("X-Plane start timed out. Please check if the path and command is correct");
            break;
            
        case QProcess::ReadError:
        case QProcess::WriteError:
            msg = tr("Could not communicate with X-Plane. Please check if the path and command are correct");
            break;
            
        case QProcess::UnknownError:
        default:
            msg = tr("X-Plane error occurred. Please check if the path and command is correct.");
            break;
    }
    
    
    QGCMessageBox::critical(tr("X-Plane HIL"), msg);
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

void QGCXPlaneLink::updateControls(quint64 time, float rollAilerons, float pitchElevator, float yawRudder, float throttle, quint8 systemMode, quint8 navMode)
{
    /* Only use HIL_CONTROL when the checkbox is unchecked */
    if (_useHilActuatorControls) {
        //qDebug() << "received HIL_CONTROL but not using it";
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

    Q_UNUSED(time);
    Q_UNUSED(systemMode);
    Q_UNUSED(navMode);

    if (_vehicle->vehicleType() == MAV_TYPE_QUADROTOR
        || _vehicle->vehicleType() == MAV_TYPE_HEXAROTOR
        || _vehicle->vehicleType() == MAV_TYPE_OCTOROTOR)
    {
        qDebug() << "MAV_TYPE_QUADROTOR";

        // Individual effort will be provided directly to the actuators on Xplane quadrotor.
        p.f[0] = yawRudder;
        p.f[1] = rollAilerons;
        p.f[2] = throttle;
        p.f[3] = pitchElevator;

        // Direct throttle control
        p.index = 25;
        writeBytesSafe((const char*)&p, sizeof(p));
    }
    else
    {
        // direct pass-through, normal fixed-wing.
        p.f[0] = -pitchElevator;
        p.f[1] = rollAilerons;
        p.f[2] = yawRudder;

        // Ail / Elevon / Rudder

        // Send to group 12
        p.index = 12;
        writeBytesSafe((const char*)&p, sizeof(p));

        // Send to group 8, which equals manual controls
        p.index = 8;
        writeBytesSafe((const char*)&p, sizeof(p));

        // Send throttle to all four motors
        p.index = 25;
        memset(p.f, 0, sizeof(p.f));
        p.f[0] = throttle;
        p.f[1] = throttle;
        p.f[2] = throttle;
        p.f[3] = throttle;
        writeBytesSafe((const char*)&p, sizeof(p));
    }
}

void QGCXPlaneLink::updateActuatorControls(quint64 time, quint64 flags, float ctl_0, float ctl_1, float ctl_2, float ctl_3, float ctl_4, float ctl_5, float ctl_6, float ctl_7, float ctl_8, float ctl_9, float ctl_10, float ctl_11, float ctl_12, float ctl_13, float ctl_14, float ctl_15, quint8 mode)
{
    if (!_useHilActuatorControls) {
        //qDebug() << "received HIL_ACTUATOR_CONTROLS but not using it";
        return;
    }

    Q_UNUSED(time);
    Q_UNUSED(flags);
    Q_UNUSED(mode);
    Q_UNUSED(ctl_12);
    Q_UNUSED(ctl_13);
    Q_UNUSED(ctl_14);
    Q_UNUSED(ctl_15);

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

    /* Initialize with zeroes */
    memset(p.f, 0, sizeof(p.f));

    switch (_vehicle->vehicleType()) {
        case MAV_TYPE_QUADROTOR:
        case MAV_TYPE_HEXAROTOR:
        case MAV_TYPE_OCTOROTOR:
        {
            p.f[0] = ctl_0;         ///< X-Plane Engine 1
            p.f[1] = ctl_1;         ///< X-Plane Engine 2
            p.f[2] = ctl_2;         ///< X-Plane Engine 3
            p.f[3] = ctl_3;         ///< X-Plane Engine 4
            p.f[4] = ctl_4;         ///< X-Plane Engine 5
            p.f[5] = ctl_5;         ///< X-Plane Engine 6
            p.f[6] = ctl_6;         ///< X-Plane Engine 7
            p.f[7] = ctl_7;         ///< X-Plane Engine 8

            /* Direct throttle control */
            p.index = 25;
            writeBytesSafe((const char*)&p, sizeof(p));
            break;
        }
        case MAV_TYPE_VTOL_RESERVED2:
        {
            /**
             * Tailsitter with four control flaps and eight motors.
             */

            /* Throttle channels */
            p.f[0] = ctl_0;
            p.f[1] = ctl_1;
            p.f[2] = ctl_2;
            p.f[3] = ctl_3;
            p.f[4] = ctl_4;
            p.f[5] = ctl_5;
            p.f[6] = ctl_6;
            p.f[7] = ctl_7;
            p.index = 25;
            writeBytesSafe((const char*)&p, sizeof(p));

            /* Control individual actuators */
            float max_surface_deflection = 30.0f; // Degrees
            sendDataRef("sim/flightmodel/controls/wing1l_ail1def", ctl_8 * max_surface_deflection);
            sendDataRef("sim/flightmodel/controls/wing1r_ail1def", ctl_9 * max_surface_deflection);
            sendDataRef("sim/flightmodel/controls/wing2l_ail1def", ctl_10 * max_surface_deflection);
            sendDataRef("sim/flightmodel/controls/wing2r_ail1def", ctl_11 * max_surface_deflection);
            sendDataRef("sim/flightmodel/controls/wing1l_ail2def", ctl_12 * max_surface_deflection);
            sendDataRef("sim/flightmodel/controls/wing1r_ail2def", ctl_13 * max_surface_deflection);
            sendDataRef("sim/flightmodel/controls/wing2l_ail2def", ctl_14 * max_surface_deflection);
            sendDataRef("sim/flightmodel/controls/wing2r_ail2def", ctl_15 * max_surface_deflection);

            break;
        }
        default:
        {
            /* direct pass-through, normal fixed-wing. */
            p.f[0] = -ctl_1;        ///< X-Plane Elevator
            p.f[1] = ctl_0;         ///< X-Plane Aileron
            p.f[2] = ctl_2;         ///< X-Plane Rudder

            /* Send to group 8, which equals manual controls */
            p.index = 8;
            writeBytesSafe((const char*)&p, sizeof(p));

            /* Send throttle to all eight motors */
            p.index = 25;
            p.f[0] = ctl_3;
            p.f[1] = ctl_3;
            p.f[2] = ctl_3;
            p.f[3] = ctl_3;
            p.f[4] = ctl_3;
            p.f[5] = ctl_3;
            p.f[6] = ctl_3;
            p.f[7] = ctl_3;
            writeBytesSafe((const char*)&p, sizeof(p));
            break;
        }

    }

}

Eigen::Matrix3f euler_to_wRo(double yaw, double pitch, double roll) {
  double c__ = cos(yaw);
  double _c_ = cos(pitch);
  double __c = cos(roll);
  double s__ = sin(yaw);
  double _s_ = sin(pitch);
  double __s = sin(roll);
  double cc_ = c__ * _c_;
  double cs_ = c__ * _s_;
  double sc_ = s__ * _c_;
  double ss_ = s__ * _s_;
  double c_c = c__ * __c;
  double c_s = c__ * __s;
  double s_c = s__ * __c;
  double s_s = s__ * __s;
  double _cc = _c_ * __c;
  double _cs = _c_ * __s;
  double csc = cs_ * __c;
  double css = cs_ * __s;
  double ssc = ss_ * __c;
  double sss = ss_ * __s;
  Eigen::Matrix3f wRo;
  wRo <<
    cc_  , css-s_c,  csc+s_s,
    sc_  , sss+c_c,  ssc-c_s,
    -_s_  ,     _cs,      _cc;
  return wRo;
}

void QGCXPlaneLink::_writeBytes(const QByteArray data)
{
    if (data.isEmpty()) return;

    // If socket exists and is connected, transmit the data
    if (socket && connectState)
    {
        socket->writeDatagram(data, remoteHost, remotePort);
    }
}

/**
 * @brief Read all pending packets from the interface.
 **/
void QGCXPlaneLink::readBytes()
{
    // Only emit updates on attitude message
    bool emitUpdate = false;
    quint16 fields_changed = 0;

    const qint64 maxLength = 65536;
    char data[maxLength];
    QHostAddress sender;
    quint16 senderPort;

    unsigned int s = socket->pendingDatagramSize();
    if (s > maxLength) std::cerr << __FILE__ << __LINE__ << " UDP datagram overflow, allowed to read less bytes than datagram size: " << s << std::endl;
    socket->readDatagram(data, maxLength, &sender, &senderPort);
    if (s > maxLength) {
    	std::string headStr = std::string(data, data+5);
    	std::cerr << __FILE__ << __LINE__ << " UDP datagram header: " << headStr << std::endl;
    }

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

        if (oldConnectionState != xPlaneConnected) {
            simUpdateFirst = QGC::groundTimeMilliseconds();
        }

        for (unsigned i = 0; i < nsegs; i++)
        {
            // Get index
            unsigned ioff = (5+i*36);;
            memcpy(&(p), data+ioff, sizeof(p));

            if (p.index == 3)
            {
                ind_airspeed = p.f[5] * 0.44704f;
                true_airspeed = p.f[6] * 0.44704f;
                groundspeed = p.f[7] * 0.44704;

                //qDebug() << "SPEEDS:" << "airspeed" << airspeed << "m/s, groundspeed" << groundspeed << "m/s";
            }
            if (p.index == 4)
            {
				// WORKAROUND: IF ground speed <<1m/s and altitude-above-ground <1m, do NOT use the X-Plane data, because X-Plane (tested 
				// with v10.3 and earlier) delivers yacc=0 and zacc=0 when the ground speed is very low, which gives e.g. wrong readings 
				// before launch when waiting on the runway. This might pose a problem for initial state estimation/calibration. 
				// Instead, we calculate our own accelerations.
				if (fabsf(groundspeed)<0.1f && alt_agl<1.0) 
				{
					// TODO: Add centrip. acceleration to the current static acceleration implementation.
                    Eigen::Vector3f g(0, 0, -9.80665f);
					Eigen::Matrix3f R = euler_to_wRo(yaw, pitch, roll);
					Eigen::Vector3f gr = R.transpose().eval() * g;

					xacc = gr[0];
					yacc = gr[1];
					zacc = gr[2];

					//qDebug() << "Calculated values" << gr[0] << gr[1] << gr[2];
				}
				else
				{
					// Accelerometer readings, directly from X-Plane and including centripetal forces. 
					const float one_g = 9.80665f;
					xacc = p.f[5] * one_g;
					yacc = p.f[6] * one_g;
					zacc = -p.f[4] * one_g;

					//qDebug() << "X-Plane values" << xacc << yacc << zacc;
				}

				fields_changed |= (1 << 0) | (1 << 1) | (1 << 2);
                emitUpdate = true;
            }
            // atmospheric pressure aircraft for XPlane 9 and 10
            else if (p.index == 6)
            {
                // inHg to hPa (hecto Pascal / millibar)
                abs_pressure = p.f[0] * 33.863886666718317f;
                temperature = p.f[1];
                fields_changed |= (1 << 9) | (1 << 12);
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
            else if ((xPlaneVersion == 10 && p.index == 16) || (xPlaneVersion == 9 && p.index == 17))
            {
                // Cross checked with XPlane flight
                pitchspeed = p.f[0];
                rollspeed = p.f[1];
                yawspeed = p.f[2];
                fields_changed |= (1 << 3) | (1 << 4) | (1 << 5);

                emitUpdate = true;
            }
            else if ((xPlaneVersion == 10 && p.index == 17) || (xPlaneVersion == 9 && p.index == 18))
            {
                //qDebug() << "HDNG" << "pitch" << p.f[0] << "roll" << p.f[1] << "hding true" << p.f[2] << "hding mag" << p.f[3];
                pitch = p.f[0] / 180.0f * M_PI;
                roll = p.f[1] / 180.0f * M_PI;
                yaw = p.f[2] / 180.0f * M_PI;

                // X-Plane expresses yaw as 0..2 PI
                if (yaw > M_PI) {
                    yaw -= 2.0f * static_cast<float>(M_PI);
                }
                if (yaw < -M_PI) {
                    yaw += 2.0f * static_cast<float>(M_PI);
                }

                float yawmag = p.f[3] / 180.0f * M_PI;

                if (yawmag > M_PI) {
                    yawmag -= 2.0f * static_cast<float>(M_PI);
                }
                if (yawmag < -M_PI) {
                    yawmag += 2.0f * static_cast<float>(M_PI);
                }

                // Normal rotation matrix, but since we rotate the
                // vector [0.25 0 0.45]', we end up with these relevant
                // matrix parts.

                xmag = cos(-yawmag) * 0.25f;
                ymag = sin(-yawmag) * 0.25f;
                zmag = 0.45f;
                fields_changed |= (1 << 6) | (1 << 7) | (1 << 8);

                double cosPhi = cos(roll);
                double sinPhi = sin(roll);
                double cosThe = cos(pitch);
                double sinThe = sin(pitch);
                double cosPsi = cos(0.0);
                double sinPsi = sin(0.0);

                float dcm[3][3];

                dcm[0][0] = cosThe * cosPsi;
                dcm[0][1] = -cosPhi * sinPsi + sinPhi * sinThe * cosPsi;
                dcm[0][2] = sinPhi * sinPsi + cosPhi * sinThe * cosPsi;

                dcm[1][0] = cosThe * sinPsi;
                dcm[1][1] = cosPhi * cosPsi + sinPhi * sinThe * sinPsi;
                dcm[1][2] = -sinPhi * cosPsi + cosPhi * sinThe * sinPsi;

                dcm[2][0] = -sinThe;
                dcm[2][1] = sinPhi * cosThe;
                dcm[2][2] = cosPhi * cosThe;

                Eigen::Matrix3f m = Eigen::Map<Eigen::Matrix3f>((float*)dcm).eval();

                Eigen::Vector3f mag(xmag, ymag, zmag);

                Eigen::Vector3f magbody = m * mag;

//                qDebug() << "yaw mag:" << p.f[2] << "x" << xmag << "y" << ymag;
//                qDebug() << "yaw mag in body:" << magbody(0) << magbody(1) << magbody(2);

                xmag = magbody(0);
                ymag = magbody(1);
                zmag = magbody(2);

                // Rotate the measurement vector into the body frame using roll and pitch


                emitUpdate = true;
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
				alt_agl = p.f[3] * 0.3048f; //convert feet (AGL) to meters
            }
            else if (p.index == 21)
            {
                vy = p.f[3];
                vx = -p.f[5];
                // moving 'up' in XPlane is positive, but its negative in NED
                // for us.
                vz = -p.f[4];
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

    // Wait for 0.5s before actually using the data, so that all fields are filled
    if (QGC::groundTimeMilliseconds() - simUpdateFirst < 500) {
        return;
    }

    // Send updated state
    if (emitUpdate && (QGC::groundTimeMilliseconds() - simUpdateLast) > 2)
    {
        simUpdateHz = simUpdateHz * 0.9f + 0.1f * (1000.0f / (QGC::groundTimeMilliseconds() - simUpdateLast));
        if (QGC::groundTimeMilliseconds() - simUpdateLastText > 2000) {
            emit statusMessage(tr("Receiving from XPlane at %1 Hz").arg(static_cast<int>(simUpdateHz)));
            // Reset lowpass with current value
            simUpdateHz = (1000.0f / (QGC::groundTimeMilliseconds() - simUpdateLast));
            // Set state
            simUpdateLastText = QGC::groundTimeMilliseconds();
        }

        simUpdateLast = QGC::groundTimeMilliseconds();

        if (_sensorHilEnabled)
        {
            diff_pressure = (ind_airspeed * ind_airspeed * 1.225f) / 2.0f;

            /* tropospheric properties (0-11km) for standard atmosphere */
            const double T1 = 15.0 + 273.15;	/* temperature at base height in Kelvin */
            const double a  = -6.5 / 1000;	/* temperature gradient in degrees per metre */
            const double g  = 9.80665;	/* gravity constant in m/s/s */
            const double R  = 287.05;	/* ideal gas constant in J/kg/K */

            /* current pressure at MSL in kPa */
            double p1 = 1013.25 / 10.0;

            /* measured pressure in hPa, plus offset to simulate weather effects / offsets */
            double p = abs_pressure / 10.0 + barometerOffsetkPa;

            /*
             * Solve:
             *
             *     /        -(aR / g)     \
             *    | (p / p1)          . T1 | - T1
             *     \                      /
             * h = -------------------------------  + h1
             *                   a
             */
            pressure_alt = (((pow((p / p1), (-(a * R) / g))) * T1) - T1) / a;

            // set pressure alt to changed
            fields_changed |= (1 << 11);

            emit sensorHilRawImuChanged(QGC::groundTimeUsecs(), xacc, yacc, zacc, rollspeed, pitchspeed, yawspeed,
                                        xmag, ymag, zmag, abs_pressure, diff_pressure / 100.0, pressure_alt, temperature, fields_changed);

            // XXX make these GUI-configurable and add randomness
            int gps_fix_type = 3;
            float eph = 0.3f;
            float epv = 0.6f;
            float vel = sqrt(vx*vx + vy*vy + vz*vz);
            float cog = atan2(vy, vx);
            int satellites = 8;

            emit sensorHilGpsChanged(QGC::groundTimeUsecs(), lat, lon, alt, gps_fix_type, eph, epv, vel, vx, vy, vz, cog, satellites);
        } else {
            emit hilStateChanged(QGC::groundTimeUsecs(), roll, pitch, yaw, rollspeed,
                                 pitchspeed, yawspeed, lat, lon, alt,
                                 vx, vy, vz, ind_airspeed, true_airspeed, xacc, yacc, zacc);
        }

        // Limit ground truth to 25 Hz
        if (QGC::groundTimeMilliseconds() - simUpdateLastGroundTruth > 40) {
            emit hilGroundTruthChanged(QGC::groundTimeUsecs(), roll, pitch, yaw, rollspeed,
                                       pitchspeed, yawspeed, lat, lon, alt,
                                       vx, vy, vz, ind_airspeed, true_airspeed, xacc, yacc, zacc);

            simUpdateLastGroundTruth = QGC::groundTimeMilliseconds();
        }
    }

    if (!oldConnectionState && xPlaneConnected)
    {
        emit statusMessage(tr("Receiving from XPlane."));
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
    if (connectState)
    {
        _should_exit = true;
    } else {
        emit simulationDisconnected();
        emit simulationConnected(false);
    }

    return !connectState;
}

void QGCXPlaneLink::selectAirframe(const QString& plane)
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

    writeBytesSafe((const char*)&pos, sizeof(pos));

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

//    writeBytesSafe((const char*)&pos, sizeof(pos));
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

    if (_vehicle->altitudeAMSL()->rawValue().toDouble() + offAlt < 0)
    {
        offAlt *= -1.0;
    }

    setPositionAttitude(_vehicle->latitude() + offLat,
                        _vehicle->longitude() + offLon,
                        _vehicle->altitudeAMSL()->rawValue().toDouble() + offAlt,
                        _vehicle->roll()->rawValue().toDouble(),
                        _vehicle->pitch()->rawValue().toDouble(),
                        _vehicle->uas()->getYaw());
}

void QGCXPlaneLink::setRandomAttitude()
{
    // Initialize generator
    srand(0);

    double roll = rand() / static_cast<double>(RAND_MAX) * 2.0 - 1.0;
    double pitch = rand() / static_cast<double>(RAND_MAX) * 2.0 - 1.0;
    double yaw = rand() / static_cast<double>(RAND_MAX) * 2.0 - 1.0;

    setPositionAttitude(_vehicle->latitude(),
                        _vehicle->longitude(),
                        _vehicle->altitudeAMSL()->rawValue().toDouble(),
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
    if (connectState) {
        qDebug() << "Simulation already active";
    } else {
        qDebug() << "STARTING X-PLANE LINK, CONNECTING TO" << remoteHost << ":" << remotePort;
        // XXX Hack
        storeSettings();

        start(HighPriority);
    }

    return true;
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

void QGCXPlaneLink::sendDataRef(QString ref, float value)
{
    #pragma pack(push, 1)
    struct payload {
        char b[5];
        float value;
        char name[500];
    } dref;
    #pragma pack(pop)

    dref.b[0] = 'D';
    dref.b[1] = 'R';
    dref.b[2] = 'E';
    dref.b[3] = 'F';
    dref.b[4] = '0';

    /* Set value */
    dref.value = value;

    /* Fill name with zeroes */
    memset(dref.name, 0, sizeof(dref.name));

    /* Set dref name */

    /* Send command */
    QByteArray ba = ref.toUtf8();
    if (ba.length() > 500) {
        return;
    }

    for (int i = 0; i < ba.length(); i++) {
        dref.name[i] = ba.at(i);
    }
    writeBytesSafe((const char*)&dref, sizeof(dref));
}
