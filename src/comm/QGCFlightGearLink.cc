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

// FlightGear process start and connection is quite fragile. Uncomment the define below to get higher level of debug output
// for tracking down problems.
#define DEBUG_FLIGHTGEAR_CONNECT

QGCFlightGearLink::QGCFlightGearLink(UASInterface* mav, QString startupArguments, QString remoteHost, QHostAddress host, quint16 port) :
    socket(NULL),
    process(NULL),
    flightGearVersion(3),
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
    this->name = tr("FlightGear 3.0+ Link (port:%1)").arg(port);
    setRemoteHost(remoteHost);
}

QGCFlightGearLink::~QGCFlightGearLink()
{   //do not disconnect unless it is connected.
    //disconnectSimulation will delete the memory that was allocated for proces, terraSync and socket
    if(connectState){
       disconnectSimulation();
    }
}

/// @brief Runs the simulation thread. We do setup work here which needs to happen in the seperate thread.
void QGCFlightGearLink::run()
{
    Q_ASSERT(mav);
    Q_ASSERT(!_fgProcessName.isEmpty());
        
    // We communicate with FlightGear over a UDP socket
    socket = new QUdpSocket(this);
    Q_CHECK_PTR(socket);
    socket->moveToThread(this);
    // FIXME: How do we deal with a failed bind. Signal?
    socket->bind(host, port);
    QObject::connect(socket, SIGNAL(readyRead()), this, SLOT(readBytes()));
    
    
    // Connect to the various HIL signals that we use to then send information across the UDP protocol to FlightGear.
    connect(mav, SIGNAL(hilControlsChanged(quint64, float, float, float, float, quint8, quint8)),
            this, SLOT(updateControls(quint64,float,float,float,float,quint8,quint8)));
    connect(this, SIGNAL(hilStateChanged(quint64, float, float, float, float,float, float, double, double, double, float, float, float, float, float, float, float, float)),
            mav, SLOT(sendHilState(quint64, float, float, float, float,float, float, double, double, double, float, float, float, float, float, float, float, float)));
    connect(this, SIGNAL(sensorHilGpsChanged(quint64, double, double, double, int, float, float, float, float, float, float, float, int)),
            mav, SLOT(sendHilGps(quint64, double, double, double, int, float, float, float, float, float, float, float, int)));
    connect(this, SIGNAL(sensorHilRawImuChanged(quint64,float,float,float,float,float,float,float,float,float,float,float,float,float,quint32)),
            mav, SLOT(sendHilSensors(quint64,float,float,float,float,float,float,float,float,float,float,float,float,float,quint32)));
    
    // Start a new process to run FlightGear in
    process = new QProcess(this);
    Q_CHECK_PTR(process);
    process->moveToThread(this);
    
    // Catch process error
    // FIXME: What happens if you quit FG from app side? Shouldn't that be the norm, instead of exiting process?
    connect(process, SIGNAL(error(QProcess::ProcessError)), this, SLOT(processError(QProcess::ProcessError)));
#ifdef DEBUG_FLIGHTGEAR_CONNECT
    connect(process, SIGNAL(readyReadStandardOutput()), this, SLOT(printFgfsOutput()));
    connect(process, SIGNAL(readyReadStandardError()), this, SLOT(printFgfsError()));
#endif
    
    if (!_fgProcessWorkingDirPath.isEmpty()) {
        process->setWorkingDirectory(_fgProcessWorkingDirPath);
		qDebug() << "Working directory" << process->workingDirectory();
    }
    
#ifdef Q_OS_WIN32
	// On Windows we need to full qualify the location of the excecutable. The call to setWorkingDirectory only
	// sets the process context, not the QProcess::start context. For some strange reason this is not the case on
	// OSX.
	QDir fgProcessFullyQualified(_fgProcessWorkingDirPath);
	_fgProcessName = fgProcessFullyQualified.absoluteFilePath(_fgProcessName);
#endif
    
    // FIXME: Need to clean up this debug arg list stuff
	QStringList debugArgList;
    debugArgList << "--log-level=debug";
    //debugArgList += "--fg-scenery=" + fgSceneryPath + "";
    //debugArgList += "--fg-root=" + fgRootPath + "";
    
	debugArgList += "--fg-root=\"c:\\Flight Gear\\data\"";
	debugArgList += "--fg-scenery=\"c:\\Flight Gear\\data\\Scenery;c:\\Flight Gear\\scenery;C:\\Flight Gear\\terrasync\"";
#ifdef DEBUG_FLIGHTGEAR_CONNECT
    qDebug() << "Starting FlightGear" << _fgProcessWorkingDirPath << _fgProcessName << _fgArgList;
	qDebug() << "Debug args" << debugArgList;
#endif
    
	process->start(_fgProcessName, /*debugArgList*/ _fgArgList);
    connectState = true;
    
    emit simulationConnected(connectState);
    emit simulationConnected();

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
        //qDebug() << "Updated controls" << rollAilerons << pitchElevator << yawRudder << throttle;
        //qDebug() << "Updated controls" << state;
    }
    else
    {
        qDebug() << "HIL: Got NaN values from the hardware: isnan output: roll: " << isnan(rollAilerons) << ", pitch: " << isnan(pitchElevator) << ", yaw: " << isnan(yawRudder) << ", throttle: " << isnan(throttle);
    }
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
    if (socket)
    {
        socket->close();
        delete socket;
        socket = NULL;
    }

    connectState = false;

    emit simulationDisconnected();
    emit simulationConnected(false);

    // Exit the thread
    quit();

    return !connectState;
}

/// @brief Splits a space seperated set of command line arguments into a QStringList.
///         Quoted strings are allowed and handled correctly.
/// @param uiArgs Arguments to parse
/// @param argList Returned argument list
/// @return Returns false if the argument list has mistmatced quotes within in.

bool QGCFlightGearLink::parseUIArguments(QString uiArgs, QStringList& argList)
{

	// This is not as easy as it seams since some options can be quoted to preserve spaces within things like
    // directories. There is likely some crazed regular expression which can do this. But after trying that
    // route I gave up and instead here is the code which does it the hard way. Another thing to be aware of
    // is that the QStringList passed to QProces::start is similar to what you would get in argv after the
    // command line is processed. This means that quoted strings have the quotes removed before making it to argv.
    
	bool inQuotedString = false;
	bool previousSpace = false;
	QString currentArg;
	for (int i=0; i<uiArgs.size(); i++) {
		const QChar chr = uiArgs[i];
        
		if (chr == ' ') {
			if (inQuotedString) {
				// Space is inside quoted string leave it in
				currentArg += chr;
				continue;
			} else {
				if (previousSpace) {
					// Disregard multiple spaces
					continue;
				} else {
					// We have a space that is finishing an argument
					previousSpace = true;
					if (inQuotedString) {
						MainWindow::instance()->showCriticalMessage(tr("FlightGear Failed to Start"), tr("Mismatched quotes in specified command line options"));
						return false;
					}
					if (!currentArg.isEmpty()) {
						argList += currentArg;
						currentArg.clear();
					}
				}
			}
		} else if (chr == '\"') {
			// Flip the state of being in a quoted string. Note that we specifically do not add the
			// quote to the string. This replicates standards command line parsing behaviour.
			if (chr == '\"') {
				inQuotedString = !inQuotedString;
			}
			previousSpace = false;
		} else {
			// Flip the state of being in a quoted string
			if (chr == '\"') {
				inQuotedString = !inQuotedString;
			}
			previousSpace = false;
			currentArg += chr;
		}
	}
	// We should never end parsing on an unterminated quote
	if (inQuotedString) {
		return false;
	}
    
	// Finish up last arg
	if (!currentArg.isEmpty()) {
		argList += currentArg;
		currentArg.clear();
	}
    
    return true;
}

/**
 * @brief Connect the connection.
 *
 * @return True if connection has been established, false if connection couldn't be established.
 **/
bool QGCFlightGearLink::connectSimulation()
{
    // We setup all the information we need to start FlightGear here such that if something goes
    // wrong we can return false out of here. All of this happens on the main UI thread. Once we
    // have that information setup we start the thread which will call run, which will in turn
    // start the various FG processes on the seperate thread.
    
    // FixMe: Does returning false out of here leave in inconsistent state?
    
    qDebug() << "STARTING FLIGHTGEAR LINK";
    
    // FIXME: !mav is failure isn't it?
    if (!mav) {
        return false;
    }
    
    // FIXME: Pull previous information from settings
    
    QString fgAppName;
    QString fgRootPath;						// FlightGear root data directory as specified by --fg-root
	bool	fgRootDirOverride = false;		// true: User has specified --fg-root from ui options
	QString fgSceneryPath;					// FlightGear scenery path as specified by --fg-scenery
	bool	fgSceneryDirOverride = false;	// true: User has specified --fg-scenery from ui options
    QDir    fgAppDir;						// Location of main FlightGear application
    
#if defined Q_OS_MACX
    // Mac installs will default to the /Applications folder 99% of the time. Anything other than
    // that is pretty non-standard so we don't try to get fancy beyond hardcoding that path.
    fgAppDir.setPath("/Applications");
    fgAppName = "FlightGear.app";
    _fgProcessName = "./fgfs.sh";
    _fgProcessWorkingDirPath = "/Applications/FlightGear.app/Contents/Resources/";
    fgRootPath = "/Applications/FlightGear.app/Contents/Resources/data/";
#elif defined Q_OS_WIN32
    _fgProcessName = "fgfs.exe";
    //fgProcessWorkingDir = "C:\\Program Files (x86)\\FlightGear\\bin\\Win32";
    
    // Windows installs are not as easy to determine. Default installation is to
    // C:\Program Files\FlightGear, but that can be easily changed. That also doesn't
    // tell us whether the user is running 32 or 64 bit which will both be installed there.
    // The preferences for the fgrun app will tell us which version they are using
    // and where it is. That is stored in the $APPDATA\fliggear.org\fgrun.prefs file. This
    // looks to be a more stable location and way to determine app location so we use that.
    fgAppName = "fgfs.exe";
    const char* appdataEnv = "APPDATA";
    if (!qgetenv(appdataEnv).isEmpty()) {
        QString fgrunPrefsFile = QDir(qgetenv(appdataEnv).constData()).absoluteFilePath("flightgear.org/fgrun.prefs");
        qDebug() << fgrunPrefsFile;
        QFile file(fgrunPrefsFile);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
			QString lookahead;	// lookahead for continuation lines
            while (!in.atEnd() || !lookahead.isEmpty()) {
                QString line;
				QRegExp regExp;
                
				// Prefs file has strange format where a line prepended with "+" is a continuation of the previous line.
				// So do a lookahead to determine if we have a continuation or not.
                
				if (!lookahead.isEmpty()) {
					line = lookahead;
					lookahead.clear();
				} else {
					line = in.readLine();
				}
                
				if (!in.atEnd()) {
					lookahead = in.readLine();
					regExp.setPattern("^\\+(.*)");
					if (regExp.indexIn(lookahead) == 0) {
						Q_ASSERT(regExp.captureCount() == 1);
						line += regExp.cap(1);
						lookahead.clear();
					}
				}
                
                regExp.setPattern("^fg_exe:(.*)");
                if (regExp.indexIn(line) == 0 && regExp.captureCount() == 1) {
                    QString fgExeLocationFullQualified = regExp.cap(1);
                    qDebug() << fgExeLocationFullQualified;
                    regExp.setPattern("(.*)\\\\fgfs.exe");
                    if (regExp.indexIn(fgExeLocationFullQualified) == 0 && regExp.captureCount() == 1) {
                        fgAppDir.setPath(regExp.cap(1));
                        _fgProcessWorkingDirPath = fgAppDir.absolutePath();
                        qDebug() << "fg_exe" << fgAppDir.absolutePath();
                    }
                    continue;
                }
                
                regExp.setPattern("^fg_root:(.*)");
                if (regExp.indexIn(line) == 0 && regExp.captureCount() == 1) {
                    fgRootPath = QDir(regExp.cap(1)).absolutePath();
                    qDebug() << "fg_root" << fgRootPath;
                    continue;
                }
                
                regExp.setPattern("^fg_scenery:(.*)");
                if (regExp.indexIn(line) == 0 && regExp.captureCount() == 1) {
					// Scenery can contain multiple paths seperated by ';' so don't do QDir::absolutePath on it
                    fgSceneryPath = regExp.cap(1);
                    qDebug() << "fg_scenery" << fgSceneryPath;
                    continue;
                }
            }
        }
    }
#elif defined Q_OS_LINUX
    // Linux installs to a location on the path so we don't need a directory to run the executable
    fgAppName = "fgfs";
    _fgProcessName = "fgfs";
    fgRootPath = "/usr/share/games/flightgear/";   // Default Ubuntu location as best guess
#else
#error Unknown OS build flavor
#endif
    
    // Validate the FlightGear application directory location. Linux runs from path so we don't validate on that OS.
#ifndef Q_OS_LINUX
    Q_ASSERT(!fgAppName.isEmpty());
    QString fgAppFullyQualified = fgAppDir.absoluteFilePath(fgAppName);
    while (!QFileInfo(fgAppFullyQualified).exists()) {
        QMessageBox msgBox(QMessageBox::Critical,
                           tr("FlightGear application not found"),
                           tr("FlightGear application not found at: %1").arg(fgAppFullyQualified),
                           QMessageBox::Cancel,
                           MainWindow::instance());
        msgBox.setWindowModality(Qt::ApplicationModal);
        msgBox.addButton(tr("I'll specify directory"), QMessageBox::ActionRole);
        if (msgBox.exec() == QMessageBox::Cancel) {
            return false;
        }
        
        // Let the user pick the right directory
        fgAppDir.setPath(QFileDialog::getExistingDirectory(MainWindow::instance(), tr("Please select directory of FlightGear application : ") + fgAppName));
        fgAppFullyQualified = fgAppDir.absoluteFilePath(fgAppName);
    }
#endif
    
	// Split the space seperated command line arguments coming in from the ui into a QStringList since
	// that is what QProcess::start needs.
	QStringList uiArgList;
    bool mismatchedQuotes = parseUIArguments(startupArguments, uiArgList);
    if (!mismatchedQuotes) {
        MainWindow::instance()->showCriticalMessage(tr("FlightGear Failed to Start"), tr("Mismatched quotes in specified command line options"));
        return false;
    }
    
	// Add the user specified arguments to our argument list
#ifdef DEBUG_FLIGHTGEAR_CONNECT
	qDebug() << "Split arguments" << uiArgList;
#endif
    _fgArgList += uiArgList;
    
    // Add --fg-root command line arg. If --fg-root is specified from the ui we use that instead.
    // We need to know what --fg-root is set to because we are going to use it to validate
    // communication protocols.
    if (startupArguments.contains("--fg-root=", Qt::CaseInsensitive)) {
		// FIXME: Won't handle missing quotes
        const char* regExpStr = "--fg-root=(.*)";
        int index = _fgArgList.indexOf(QRegExp(regExpStr, Qt::CaseInsensitive));
        Q_ASSERT(index != -1);
        QString rootArg(_fgArgList[index]);
        QRegExp regExp(regExpStr);
        index = regExp.indexIn(rootArg);
        Q_ASSERT(index != -1);
        fgRootPath = regExp.cap(1);
        qDebug() << "--fg-root override" << fgRootPath;
        fgRootDirOverride = true;
    } else {
		_fgArgList += "--fg-root=" + fgRootPath;
	}
    
    // Add --fg-scenery command line arg. If --fg-scenery is specified from the ui we use that instead.
    // On Windows --fg-scenery is required on the command line otherwise FlightGear won't boot.
	// FIXME: Use single routine for both overrides
    if (startupArguments.contains("--fg-scenery=", Qt::CaseInsensitive)) {
		// FIXME: Won't handle missing quotes
        const char* regExpStr = "--fg-scenery=(.*)";
        int index = _fgArgList.indexOf(QRegExp(regExpStr, Qt::CaseInsensitive));
        Q_ASSERT(index != -1);
        QString rootArg(_fgArgList[index]);
        QRegExp regExp(regExpStr);
        index = regExp.indexIn(rootArg);
        Q_ASSERT(index != -1);
		Q_ASSERT(regExp.captureCount() == 1);
        fgSceneryPath = regExp.cap(1);
        qDebug() << "--fg-scenery override" << fgSceneryPath;
        fgSceneryDirOverride = true;
    } else if (!fgSceneryPath.isEmpty()) {
		_fgArgList += "--fg-scenery=" + fgSceneryPath;
	}
    
#ifdef Q_OS_WIN32
    // Windows won't start without an --fg-scenery set. We don't validate the directory in the path since
	// it can be multiple paths.
    if (fgSceneryPath.isEmpty()) {
        QString errMsg;
        if (fgSceneryDirOverride) {
            errMsg = tr("--fg-scenery directory specified from ui option not found: %1").arg(fgSceneryPath);
        } else {
            errMsg = tr("Unable to automatically determine --fg-scenery directory location. You will need to specify --fg-scenery=directory as an additional command line parameter from ui.");
        }
        MainWindow::instance()->showCriticalMessage(tr("FlightGear Failed to Start"), errMsg);
        return false;
    }
#else
    Q_UNUSED(fgSceneryDirOverride);
#endif
    
    // Check that we have a good fgRootDir set before we use it to check communication protocol files.
    if (fgRootPath.isEmpty() || !QFileInfo(fgRootPath).isDir()) {
        QString errMsg;
        if (fgRootDirOverride) {
            errMsg = tr("--fg-root directory specified from ui option not found: %1").arg(fgRootPath);
        } else if (fgRootPath.isEmpty()) {
            errMsg = tr("Unable to automatically determine --fg-root directory location. You will need to specify --fg-root=<directory> as an additional command line parameter from ui.");
        } else {
            errMsg = tr("Unable to automatically determine --fg-root directory location. Attempted directory '%1', which does not exist. You will need to specify --fg-root=<directory> as an additional command line parameter from ui.").arg(fgRootPath);
        }
        MainWindow::instance()->showCriticalMessage(tr("FlightGear Failed to Start"), errMsg);
        return false;
    }
    
    // Setup and verify directory which contains QGC provided aircraft files
    QString qgcAircraftDir(QApplication::applicationDirPath() + "/files/flightgear/Aircraft");
    if (!QFileInfo(qgcAircraftDir).isDir()) {
        MainWindow::instance()->showCriticalMessage(tr("Incorrect QGroundControl installation"), tr("Aircraft directory is missing: '%1'.").arg(qgcAircraftDir));
        return false;
    }
    _fgArgList += "--fg-aircraft=" + qgcAircraftDir;
    
    // Setup protocol we will be using to communicate with FlightGear
    QString fgProtocol(mav->getSystemType() == MAV_TYPE_QUADROTOR ? "qgroundcontrol-quadrotor" : "qgroundcontrol-fixed-wing");
    QString fgProtocolArg("--generic=socket,%1,300,127.0.0.1,%2,udp,%3");
    _fgArgList << fgProtocolArg.arg("out").arg(port).arg(fgProtocol);
    _fgArgList << fgProtocolArg.arg("in").arg(currentPort).arg(fgProtocol);
    
    // Verify directory where FlightGear stores communicaton protocols.
    QDir fgProtocolDir(fgRootPath);
    if (!fgProtocolDir.cd("Protocol")) {
        MainWindow::instance()->showCriticalMessage(tr("Incorrect FlightGear setup"), tr("Protocol directory is missing: '%1'. Command line parameter for --fg-root may be set incorrectly.").arg(fgProtocolDir.path()));
        return false;
    }
    
    // Verify directory which contains QGC provided FlightGear communication protocol files
    QDir qgcProtocolDir(QApplication::applicationDirPath() + "/files/flightgear/Protocol/");
    if (!qgcProtocolDir.isReadable()) {
        MainWindow::instance()->showCriticalMessage(tr("Incorrect QGroundControl installation"), tr("Protocol directory is missing (%1).").arg(qgcProtocolDir.path()));
        return false;
    }
    
    // Communication protocol must be in FlightGear protocol directory. There does not appear to be any way
    // around this by specifying something on the FlightGear command line. FG code does direct append
    // of protocol xml file to $FG_ROOT and $FG_ROOT only allows a single directory to be specified.
    QString fgProtocolXmlFile = fgProtocol + ".xml";
    QString fgProtocolFileFullyQualified = fgProtocolDir.absoluteFilePath(fgProtocolXmlFile);
    if (!QFileInfo(fgProtocolFileFullyQualified).exists()) {
        QMessageBox msgBox(QMessageBox::Critical,
                           tr("FlightGear Failed to Start"),
                           tr("FlightGear Failed to Start. QGroundControl protocol (%1) not installed to FlightGear Protocol directory (%2)").arg(fgProtocolXmlFile).arg(fgProtocolDir.path()),
                           QMessageBox::Cancel,
                           MainWindow::instance());
        msgBox.setWindowModality(Qt::ApplicationModal);
        msgBox.addButton(tr("Fix it for me"), QMessageBox::ActionRole);
        if (msgBox.exec() == QMessageBox::Cancel) {
            return false;
        }
        
        // Make sure we can find the communication protocol file in QGC install before we attempt to copy to FlightGear
        QString qgcProtocolFileFullyQualified = qgcProtocolDir.absoluteFilePath(fgProtocolXmlFile);
        if (!QFileInfo(qgcProtocolFileFullyQualified).exists()) {
            MainWindow::instance()->showCriticalMessage(tr("Incorrect QGroundControl installation"), tr("FlightGear protocol file missing: %1").arg(qgcProtocolFileFullyQualified));
            return false;
        }
        
        // Now that we made it this far, we should be able to try to copy the protocol file to FlightGear.
        bool succeeded = QFile::copy(qgcProtocolFileFullyQualified, fgProtocolFileFullyQualified);
        if (!succeeded) {
            MainWindow::instance()->showCriticalMessage(tr("Copy failed"), tr("Copy from (%1) to (%2) failed, possibly due to permissions issue. You will need to perform manually.").arg(qgcProtocolFileFullyQualified).arg(fgProtocolFileFullyQualified));
            
#ifdef Q_OS_WIN32
            QString copyCmd = QString("copy \"%1\" \"%2\"").arg(qgcProtocolFileFullyQualified).arg(fgProtocolFileFullyQualified);
            copyCmd.replace("/", "\\");
#else
            QString copyCmd = QString("sudo cp %1 %2").arg(qgcProtocolFileFullyQualified).arg(fgProtocolFileFullyQualified);
#endif
            
            QMessageBox msgBox(QMessageBox::Critical,
                               tr("Copy failed"),
#ifdef Q_OS_WIN32
                               tr("Try pasting the following command into a Command Prompt which was started with Run as Administrator: ") + copyCmd,
#else
                               tr("Try pasting the following command into a shell: ") + copyCmd,
#endif
                               QMessageBox::Cancel,
                               MainWindow::instance());
            msgBox.setWindowModality(Qt::ApplicationModal);
            msgBox.addButton(tr("Copy to Clipboard"), QMessageBox::ActionRole);
            if (msgBox.exec() != QMessageBox::Cancel) {
                QApplication::clipboard()->setText(copyCmd);
            }
            return false;
        }
    }
    
    // Start the engines to save a startup step
    if (mav->getSystemType() == MAV_TYPE_QUADROTOR) {
        // Start all engines of the quad
        _fgArgList << "--prop:/engines/engine[0]/running=true";
        _fgArgList << "--prop:/engines/engine[1]/running=true";
        _fgArgList << "--prop:/engines/engine[2]/running=true";
        _fgArgList << "--prop:/engines/engine[3]/running=true";
    } else {
        _fgArgList << "--prop:/engines/engine/running=true";
    }
    
    // We start out at our home position
    _fgArgList << QString("--lat=%1").arg(UASManager::instance()->getHomeLatitude());
    _fgArgList << QString("--lon=%1").arg(UASManager::instance()->getHomeLongitude());
    // The altitude is not set because an altitude not equal to the ground altitude leads to a non-zero default throttle in flightgear
    // Without the altitude-setting the aircraft is positioned on the ground
    //_fgArgList << QString("--altitude=%1").arg(UASManager::instance()->getHomeAltitude());
    
#ifdef DEBUG_FLIGHTGEAR_CONNECT
    // This tell FlightGear to output highest debug level of log output. Handy for debuggin failures by looking at the FG
    // log files.
    _fgArgList << "--log-level=debug";
#endif
    
    start(HighPriority);
    return true;
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
