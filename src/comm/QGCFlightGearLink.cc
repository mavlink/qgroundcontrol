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
 *   @author Thomas Gubler <thomasgubler@student.ethz.ch>
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

QGCFlightGearLink::QGCFlightGearLink(UASInterface* mav, QString startupArguments, QString remoteHost, QHostAddress host, quint16 port) :
    socket(NULL),
    process(NULL),
    terraSync(NULL),
    flightGearVersion(0),
    startupArguments(startupArguments),
    _sensorHilEnabled(true),
    barometerOffsetkPa(0.0f)
{
    // We're doing it wrong - because the Qt folks got the API wrong:
    // http://blog.qt.digia.com/blog/2010/06/17/youre-doing-it-wrong/
    moveToThread(this);

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
    qDebug() << "STARTING FLIGHTGEAR LINK";

    if (!mav) return;
    socket = new QUdpSocket(this);
    socket->moveToThread(this);
    connectState = socket->bind(host, port);

    QObject::connect(socket, SIGNAL(readyRead()), this, SLOT(readBytes()));

    process = new QProcess(this);
    process->moveToThread(this);
    terraSync = new QProcess(this);
    terraSync->moveToThread(this);

    connect(mav, SIGNAL(hilControlsChanged(quint64, float, float, float, float, quint8, quint8)), this, SLOT(updateControls(quint64,float,float,float,float,quint8,quint8)));
    connect(this, SIGNAL(hilStateChanged(quint64, float, float, float, float,float, float, double, double, double, float, float, float, float, float, float, float, float)), mav, SLOT(sendHilState(quint64, float, float, float, float,float, float, double, double, double, float, float, float, float, float, float, float, float)));
    connect(this, SIGNAL(sensorHilGpsChanged(quint64, double, double, double, int, float, float, float, float, float, float, float, int)), mav, SLOT(sendHilGps(quint64, double, double, double, int, float, float, float, float, float, float, float, int)));
    connect(this, SIGNAL(sensorHilRawImuChanged(quint64,float,float,float,float,float,float,float,float,float,float,float,float,float,quint32)), mav, SLOT(sendHilSensors(quint64,float,float,float,float,float,float,float,float,float,float,float,float,float,quint32)));

    UAS* uas = dynamic_cast<UAS*>(mav);
    if (uas)
    {
        uas->startHil();
    }

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
    QString fgAircraft;

#ifdef Q_OS_MACX
    processFgfs = "/Applications/FlightGear.app/Contents/Resources/fgfs";
    processTerraSync = "/Applications/FlightGear.app/Contents/Resources/terrasync";
    //fgRoot = "/Applications/FlightGear.app/Contents/Resources/data";
    fgScenery = "/Applications/FlightGear.app/Contents/Resources/data/Scenery";
    terraSyncScenery = "/Applications/FlightGear.app/Contents/Resources/data/Scenery-TerraSync";
    //   /Applications/FlightGear.app/Contents/Resources/data/Scenery:
#endif

#ifdef Q_OS_WIN32
    processFgfs = "C:\\Program Files (x86)\\FlightGear\\bin\\Win32\\fgfs";
    //fgRoot = "C:\\Program Files (x86)\\FlightGear\\data";
    fgScenery = "C:\\Program Files (x86)\\FlightGear\\data\\Scenery";
    terraSyncScenery = "C:\\Program Files (x86)\\FlightGear\\data\\Scenery-Terrasync";
#endif

#ifdef Q_OS_LINUX
    processFgfs = "fgfs";
    //fgRoot = "/usr/share/flightgear";
    QString fgScenery1 = "/usr/share/flightgear/data/Scenery";
    QString fgScenery2 = "/usr/share/games/flightgear/Scenery"; // Ubuntu default location
    fgScenery = ""; //Flightgear can also start with fgScenery = ""
    if (QDir(fgScenery1).exists())
        fgScenery = fgScenery1;
    else if (QDir(fgScenery2).exists())
        fgScenery = fgScenery2;


    processTerraSync = "nice"; //according to http://wiki.flightgear.org/TerraSync, run with lower priority
    terraSyncScenery = QDir::homePath() + "/.terrasync/Scenery"; //according to http://wiki.flightgear.org/TerraSync a separate directory is used
#endif

    fgAircraft = QApplication::applicationDirPath() + "/files/flightgear/Aircraft";

    // Sanity checks
    bool sane = true;
//    QFileInfo executable(processFgfs);
//    if (!executable.isExecutable())
//    {
//        MainWindow::instance()->showCriticalMessage(tr("FlightGear Failed to Start"), tr("FlightGear was not found at %1").arg(processFgfs));
//        sane = false;
//    }

//    QFileInfo root(fgRoot);
//    if (!root.isDir())
//    {
//        MainWindow::instance()->showCriticalMessage(tr("FlightGear Failed to Start"), tr("FlightGear data directory was not found at %1").arg(fgRoot));
//        sane = false;
//    }

//    QFileInfo scenery(fgScenery);
//    if (!scenery.isDir())
//    {
//        MainWindow::instance()->showCriticalMessage(tr("FlightGear Failed to Start"), tr("FlightGear scenery directory was not found at %1").arg(fgScenery));
//        sane = false;
//    }

//    QFileInfo terraSyncExecutableInfo(processTerraSync);
//    if (!terraSyncExecutableInfo.isExecutable())
//    {
//        MainWindow::instance()->showCriticalMessage(tr("FlightGear Failed to Start"), tr("TerraSync was not found at %1").arg(processTerraSync));
//        sane = false;
//    }


    if (!sane) return;

    // --atlas=socket,out,1,localhost,5505,udp
    // terrasync -p 5505 -S -d /usr/local/share/TerraSync

    /*Prepare FlightGear Arguments */
    //flightGearArguments << QString("--fg-root=%1").arg(fgRoot);
    flightGearArguments << QString("--fg-scenery=%1:%2").arg(fgScenery).arg(terraSyncScenery); //according to http://wiki.flightgear.org/TerraSync a separate directory is used
    flightGearArguments << QString("--fg-aircraft=%1").arg(fgAircraft);
    if (mav->getSystemType() == MAV_TYPE_QUADROTOR)
    {
        flightGearArguments << QString("--generic=socket,out,300,127.0.0.1,%1,udp,qgroundcontrol-quadrotor").arg(port);
        flightGearArguments << QString("--generic=socket,in,300,127.0.0.1,%1,udp,qgroundcontrol-quadrotor").arg(currentPort);
    }
    else
    {
        flightGearArguments << QString("--generic=socket,out,300,127.0.0.1,%1,udp,qgroundcontrol-fixed-wing").arg(port);
        flightGearArguments << QString("--generic=socket,in,300,127.0.0.1,%1,udp,qgroundcontrol-fixed-wing").arg(currentPort);
    }
    flightGearArguments << "--atlas=socket,out,1,localhost,5505,udp";
//    flightGearArguments << "--in-air";
//    flightGearArguments << "--roll=0";
//    flightGearArguments << "--pitch=0";
//    flightGearArguments << "--vc=90";
//    flightGearArguments << "--heading=300";
//    flightGearArguments << "--timeofday=noon";
//    flightGearArguments << "--disable-hud-3d";
//    flightGearArguments << "--disable-fullscreen";
//    flightGearArguments << "--geometry=400x300";
//    flightGearArguments << "--disable-anti-alias-hud";
//    flightGearArguments << "--wind=0@0";
//    flightGearArguments << "--turbulence=0.0";
//    flightGearArguments << "--prop:/sim/frame-rate-throttle-hz=30";
//    flightGearArguments << "--control=mouse";
//    flightGearArguments << "--disable-intro-music";
//    flightGearArguments << "--disable-sound";
//    flightGearArguments << "--disable-random-objects";
//    flightGearArguments << "--disable-ai-models";
//    flightGearArguments << "--shading-flat";
//    flightGearArguments << "--fog-disable";
//    flightGearArguments << "--disable-specular-highlight";
//    //flightGearArguments << "--disable-skyblend";
//    flightGearArguments << "--disable-random-objects";
//    flightGearArguments << "--disable-panel";
//    //flightGearArguments << "--disable-horizon-effect";
//    flightGearArguments << "--disable-clouds";
//    flightGearArguments << "--fdm=jsb";
//    flightGearArguments << "--units-meters"; //XXX: check: the protocol xml has already a conversion from feet to m?
//    flightGearArguments << "--notrim";

    flightGearArguments += startupArguments.split(" ");
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
    //The altitude is not set because an altitude not equal to the ground altitude leads to a non-zero default throttle in flightgear
    //Without the altitude-setting the aircraft is positioned on the ground
    //flightGearArguments << QString("--altitude=%1").arg(UASManager::instance()->getHomeAltitude());

    // Add new argument with this: flightGearArguments << "";
    //flightGearArguments << QString("--aircraft=%2").arg(aircraft);

    /*Prepare TerraSync Arguments */
    QStringList terraSyncArguments;
#ifdef Q_OS_LINUX
    terraSyncArguments << "terrasync";
#endif
    terraSyncArguments << "-p";
    terraSyncArguments << "5505";
    terraSyncArguments << "-S";
    terraSyncArguments << "-d";
    terraSyncArguments << terraSyncScenery; //according to http://wiki.flightgear.org/TerraSync a separate directory is used

#ifdef Q_OS_LINUX
     /* Setting environment */
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    process->setProcessEnvironment(env);
    terraSync->setProcessEnvironment(env);
#endif
//    connect (terraSync, SIGNAL(readyReadStandardOutput()), this, SLOT(printTerraSyncOutput()));
//    connect (terraSync, SIGNAL(readyReadStandardError()), this, SLOT(printTerraSyncError()));
    terraSync->start(processTerraSync, terraSyncArguments);
//    qDebug() << "STARTING: " << processTerraSync << terraSyncArguments;

    process->start(processFgfs, flightGearArguments);
//    connect (process, SIGNAL(readyReadStandardOutput()), this, SLOT(printFgfsOutput()));
//    connect (process, SIGNAL(readyReadStandardError()), this, SLOT(printFgfsError()));



    emit simulationConnected(connectState);
    if (connectState) {
        emit simulationConnected();
        connectionStartTime = QGC::groundTimeUsecs()/1000;
    }
    qDebug() << "STARTING SIM";

//    qDebug() << "STARTING: " << processFgfs << flightGearArguments;

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

void QGCFlightGearLink::updateActuators(quint64 time, float act1, float act2, float act3, float act4, float act5, float act6, float act7, float act8)
{
    Q_UNUSED(time);
    Q_UNUSED(act1);
    Q_UNUSED(act2);
    Q_UNUSED(act3);
    Q_UNUSED(act4);
    Q_UNUSED(act5);
    Q_UNUSED(act6);
    Q_UNUSED(act7);
    Q_UNUSED(act8);
}

void QGCFlightGearLink::updateControls(quint64 time, float rollAilerons, float pitchElevator, float yawRudder, float throttle, quint8 systemMode, quint8 navMode)
{
    // magnetos,aileron,elevator,rudder,throttle\n

    //float magnetos = 3.0f;
    Q_UNUSED(time);
    Q_UNUSED(systemMode);
    Q_UNUSED(navMode);

    if(!isnan(rollAilerons) && !isnan(pitchElevator) && !isnan(yawRudder) && !isnan(throttle))
    {
        QString state("%1\t%2\t%3\t%4\t%5\n");
        state = state.arg(rollAilerons).arg(pitchElevator).arg(yawRudder).arg(true).arg(throttle);
        writeBytes(state.toAscii().constData(), state.length());
//        qDebug() << "updated controls" << rollAilerons << pitchElevator << yawRudder << throttle;
    }
    else
    {
        qDebug() << "HIL: Got NaN values from the hardware: isnan output: roll: " << isnan(rollAilerons) << ", pitch: " << isnan(pitchElevator) << ", yaw: " << isnan(yawRudder) << ", throttle: " << isnan(throttle);
    }
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
//    qDebug() << "FG LINK GOT:" << state;

    QStringList values = state.split("\t");

    // Check length
    const int nValues = 21;
    if (values.size() != nValues)
    {
        qDebug() << "RETURN LENGTH MISMATCHING EXPECTED" << nValues << "BUT GOT" << values.size();
        qDebug() << state;
        return;
    }

    // Parse string
    float roll, pitch, yaw, rollspeed, pitchspeed, yawspeed;
    double lat, lon, alt;
    float ind_airspeed;
    float true_airspeed;
    float vx, vy, vz, xacc, yacc, zacc;
    float diff_pressure;
    float temperature;
    float abs_pressure;
    float mag_variation, mag_dip, xmag_ned, ymag_ned, zmag_ned, xmag_body, ymag_body, zmag_body;


    lat = values.at(1).toDouble();
    lon = values.at(2).toDouble();
    alt = values.at(3).toDouble();
    roll = values.at(4).toFloat();
    pitch = values.at(5).toFloat();
    yaw = values.at(6).toFloat();
    rollspeed = values.at(7).toFloat();
    pitchspeed = values.at(8).toFloat();
    yawspeed = values.at(9).toFloat();

    xacc = values.at(10).toFloat();
    yacc = values.at(11).toFloat();
    zacc = values.at(12).toFloat();

    vx = values.at(13).toFloat();
    vy = values.at(14).toFloat();
    vz = values.at(15).toFloat();

    true_airspeed = values.at(16).toFloat();

    mag_variation = values.at(17).toFloat();
    mag_dip = values.at(18).toFloat();

    temperature = values.at(19).toFloat();
    abs_pressure = values.at(20).toFloat() * 1e2f; //convert to Pa from hPa
    abs_pressure += barometerOffsetkPa * 1e3f; //add offset, convert from kPa to Pa

    //calculate differential pressure
    const float air_gas_constant = 287.1f; // J/(kg * K)
    const float absolute_null_celsius = -273.15f; // °C
    float density = abs_pressure / (air_gas_constant * (temperature - absolute_null_celsius));
    diff_pressure = true_airspeed * true_airspeed * density / 2.0f;
    //qDebug() << "diff_pressure: " << diff_pressure << "abs_pressure: " << abs_pressure;

    /* Calculate indicated airspeed */
    const float air_density_sea_level_15C  = 1.225f; //kg/m^3
    if (diff_pressure > 0)
    {
        ind_airspeed =  sqrtf((2.0f*diff_pressure) / air_density_sea_level_15C);
    } else
    {
        ind_airspeed =  -sqrtf((2.0f*fabsf(diff_pressure)) / air_density_sea_level_15C);
    }

    //qDebug() << "ind_airspeed: " << ind_airspeed << "true_airspeed: " << true_airspeed;

    // Send updated state
    //qDebug()  << "sensorHilEnabled: " << sensorHilEnabled;
    if (_sensorHilEnabled)
    {
        quint16 fields_changed = 0xFFF; //set all 12 used bits

        float pressure_alt = alt;

        xmag_ned = cosf(mag_variation);
        ymag_ned = sinf(mag_variation);
        zmag_ned = sinf(mag_dip);
        float tempMagLength = sqrtf(xmag_ned*xmag_ned + ymag_ned*ymag_ned + zmag_ned*zmag_ned);
        xmag_ned = xmag_ned / tempMagLength;
        ymag_ned = ymag_ned / tempMagLength;
        zmag_ned = zmag_ned / tempMagLength;

        //transform magnetic measurement to body frame coordinates
        double cosPhi = cos(roll);
        double sinPhi = sin(roll);
        double cosThe = cos(pitch);
        double sinThe = sin(pitch);
        double cosPsi = cos(yaw);
        double sinPsi = sin(yaw);

        float R_B_N[3][3];

        R_B_N[0][0] = cosThe * cosPsi;
        R_B_N[0][1] = -cosPhi * sinPsi + sinPhi * sinThe * cosPsi;
        R_B_N[0][2] = sinPhi * sinPsi + cosPhi * sinThe * cosPsi;

		R_B_N[1][0] = cosThe * sinPsi;
		R_B_N[1][1] = cosPhi * cosPsi + sinPhi * sinThe * sinPsi;
		R_B_N[1][2] = -sinPhi * cosPsi + cosPhi * sinThe * sinPsi;

		R_B_N[2][0] = -sinThe;
		R_B_N[2][1] = sinPhi * cosThe;
		R_B_N[2][2] = cosPhi * cosThe;

        Eigen::Matrix3f R_B_N_M = Eigen::Map<Eigen::Matrix3f>((float*)R_B_N).eval();

        Eigen::Vector3f mag_ned(xmag_ned, ymag_ned, zmag_ned);

        Eigen::Vector3f mag_body = R_B_N_M * mag_ned;

        xmag_body = mag_body(0);
        ymag_body = mag_body(1);
        zmag_body = mag_body(2);

        emit sensorHilRawImuChanged(QGC::groundTimeUsecs(), xacc, yacc, zacc, rollspeed, pitchspeed, yawspeed,
                                    xmag_body, ymag_body, zmag_body, abs_pressure*1e-2f, diff_pressure*1e-2f, pressure_alt, temperature, fields_changed); //Pressure in hPa for mavlink

//        qDebug()  << "sensorHilRawImuChanged " << xacc  << yacc << zacc  << rollspeed << pitchspeed << yawspeed << xmag << ymag << zmag << abs_pressure << diff_pressure << pressure_alt << temperature;
        int gps_fix_type = 3;
        float eph = 0.3f;
        float epv = 0.6f;
        float vel = sqrt(vx*vx + vy*vy + vz*vz);
        float cog = yaw;
        int satellites = 8;

        emit sensorHilGpsChanged(QGC::groundTimeUsecs(), lat, lon, alt, gps_fix_type, eph, epv, vel, vx, vy, vz, cog, satellites);

//        qDebug()  << "sensorHilGpsChanged " << lat  << lon << alt  << vel;
    } else {
        emit hilStateChanged(QGC::groundTimeUsecs(), roll, pitch, yaw, rollspeed,
                         pitchspeed, yawspeed, lat, lon, alt,
                         vx, vy, vz,
                         ind_airspeed, true_airspeed,
                         xacc, yacc, zacc);
        //qDebug()  << "hilStateChanged " << (qint32)lat << (qint32)lon << (qint32)alt;
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
    disconnect(mav, SIGNAL(hilControlsChanged(quint64, float, float, float, float, quint8, quint8)), this, SLOT(updateControls(quint64,float,float,float,float,quint8,quint8)));
    disconnect(this, SIGNAL(hilStateChanged(quint64, float, float, float, float,float, float, double, double, double, float, float, float, float, float, float, float, float)), mav, SLOT(sendHilState(quint64, float, float, float, float,float, float, double, double, double, float, float, float, float, float, float, float, float)));
    disconnect(this, SIGNAL(sensorHilGpsChanged(quint64, double, double, double, int, float, float, float, float, float, float, float, int)), mav, SLOT(sendHilGps(quint64, double, double, double, int, float, float, float, float, float, float, float, int)));
    disconnect(this, SIGNAL(sensorHilRawImuChanged(quint64,float,float,float,float,float,float,float,float,float,float,float,float,float,quint32)), mav, SLOT(sendHilSensors(quint64,float,float,float,float,float,float,float,float,float,float,float,float,float,quint32)));

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

    start(HighPriority);
    return true;
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

void QGCFlightGearLink::printFgfsOutput()
{
   qDebug() << "fgfs stdout:";
   QByteArray byteArray = process->readAllStandardOutput();
   QStringList strLines = QString(byteArray).split("\n");

   foreach (QString line, strLines){
    qDebug() << line;
   }
}

void QGCFlightGearLink::printFgfsError()
{
   qDebug() << "fgfs stderr:";

   QByteArray byteArray = process->readAllStandardError();
   QStringList strLines = QString(byteArray).split("\n");

   foreach (QString line, strLines){
    qDebug() << line;
   }
}

/**
 * @brief Set the startup arguments used to start flightgear
 *
 **/
void QGCFlightGearLink::setStartupArguments(QString startupArguments)
{
    this->startupArguments = startupArguments;
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

void QGCFlightGearLink::setBarometerOffset(float barometerOffsetkPa)
{
    this->barometerOffsetkPa = barometerOffsetkPa;
}
