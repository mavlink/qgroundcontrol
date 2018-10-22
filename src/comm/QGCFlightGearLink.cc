/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


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
#include <QHostInfo>
#include <QMessageBox>
#include <QClipboard>

#include <iostream>
#include <Eigen/Eigen>

#include "QGCFlightGearLink.h"
#include "QGC.h"
#include "QGCQFileDialog.h"
#include "QGCMessageBox.h"
#include "QGCApplication.h"
#include "Vehicle.h"
#include "UAS.h"
#include "QGroundControlQmlGlobal.h"

// FlightGear _fgProcess start and connection is quite fragile. Uncomment the define below to get higher level of debug output
// for tracking down problems.
//#define DEBUG_FLIGHTGEAR_CONNECT

QGCFlightGearLink::QGCFlightGearLink(Vehicle* vehicle, QString startupArguments, QString remoteHost, QHostAddress host, quint16 port)
    : _vehicle(vehicle)
    , _udpCommSocket(NULL)
    , _fgProcess(NULL)
    , flightGearVersion(3)
    , startupArguments(startupArguments)
    , _sensorHilEnabled(true)
    , barometerOffsetkPa(0.0f)
{
    // We're doing it wrong - because the Qt folks got the API wrong:
    // http://blog.qt.digia.com/blog/2010/06/17/youre-doing-it-wrong/
    moveToThread(this);

    this->host = host;
    this->port = port + _vehicle->id();
    this->connectState = false;
    this->currentPort = 49000 + _vehicle->id();
    this->name = tr("FlightGear 3.0+ Link (port:%1)").arg(port);
    setRemoteHost(remoteHost);

    // We need a mechanism so show error message from our FGLink thread on the UI thread. This signal connection will do that for us.
    connect(this, &QGCFlightGearLink::showCriticalMessageFromThread, qgcApp(), &QGCApplication::criticalMessageBoxOnMainThread);
    connect(this, &QGCFlightGearLink::disconnectSim, this, &QGCFlightGearLink::disconnectSimulation);
}

QGCFlightGearLink::~QGCFlightGearLink()
{   //do not disconnect unless it is connected.
    //disconnectSimulation will delete the memory that was allocated for process, terraSync and _udpCommSocket
    if(connectState){
       disconnectSimulation();
    }
}

/// @brief Runs the simulation thread. We do setup work here which needs to happen in the separate thread.
void QGCFlightGearLink::run()
{
    Q_ASSERT(_vehicle);
    Q_ASSERT(!_fgProcessName.isEmpty());

    // We communicate with FlightGear over a UDP _udpCommSocket
    _udpCommSocket = new QUdpSocket(this);
    Q_CHECK_PTR(_udpCommSocket);
    _udpCommSocket->moveToThread(this);
    _udpCommSocket->bind(host, port, QAbstractSocket::ReuseAddressHint);
    QObject::connect(_udpCommSocket, &QUdpSocket::readyRead, this, &QGCFlightGearLink::readBytes);


    // Connect to the various HIL signals that we use to then send information across the UDP protocol to FlightGear.
    connect(_vehicle->uas(), &UAS::hilControlsChanged, this, &QGCFlightGearLink::updateControls);

    connect(this, &QGCFlightGearLink::hilStateChanged, _vehicle->uas(), &UAS::sendHilState);
    connect(this, &QGCFlightGearLink::sensorHilGpsChanged, _vehicle->uas(), &UAS::sendHilGps);
    connect(this, &QGCFlightGearLink::sensorHilRawImuChanged, _vehicle->uas(), &UAS::sendHilSensors);
    connect(this, &QGCFlightGearLink::sensorHilOpticalFlowChanged, _vehicle->uas(), &UAS::sendHilOpticalFlow);

    // Start a new QProcess to run FlightGear in
    _fgProcess = new QProcess(this);
    Q_CHECK_PTR(_fgProcess);
    _fgProcess->moveToThread(this);

    connect(_fgProcess, static_cast<void (QProcess::*)(QProcess::ProcessError)>(&QProcess::error),
            this, &QGCFlightGearLink::processError);

//#ifdef DEBUG_FLIGHTGEAR_CONNECT
    connect(_fgProcess, &QProcess::readyReadStandardOutput, this, &QGCFlightGearLink::_printFgfsOutput);
    connect(_fgProcess, &QProcess::readyReadStandardError, this, &QGCFlightGearLink::_printFgfsError);
//#endif

    if (!_fgProcessWorkingDirPath.isEmpty()) {
        _fgProcess->setWorkingDirectory(_fgProcessWorkingDirPath);
        qDebug() << "Working directory" << _fgProcess->workingDirectory();
    }

#ifdef Q_OS_WIN32
    // On Windows we need to full qualify the location of the excecutable. The call to setWorkingDirectory only
    // sets the QProcess context, not the QProcess::start context. For some strange reason this is not the case on
    // OSX.
    QDir fgProcessFullyQualified(_fgProcessWorkingDirPath);
    _fgProcessName = fgProcessFullyQualified.absoluteFilePath(_fgProcessName);
#endif

#ifdef DEBUG_FLIGHTGEAR_CONNECT
    qDebug() << "\nStarting FlightGear" << _fgProcessWorkingDirPath << _fgProcessName << _fgArgList << "\n";
#endif

    _fgProcess->start(_fgProcessName, _fgArgList);
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
        emit showCriticalMessageFromThread(tr("FlightGear Failed to Start"), _fgProcess->errorString());
        break;
    case QProcess::Crashed:
        emit showCriticalMessageFromThread(tr("FlightGear Crashed"), tr("This is a FlightGear-related problem. Please upgrade FlightGear"));
        break;
    case QProcess::Timedout:
        emit showCriticalMessageFromThread(tr("FlightGear Start Timed Out"), tr("Please check if the path and command is correct"));
        break;
    case QProcess::WriteError:
        emit showCriticalMessageFromThread(tr("Could not Communicate with FlightGear"), tr("Please check if the path and command is correct"));
        break;
    case QProcess::ReadError:
        emit showCriticalMessageFromThread(tr("Could not Communicate with FlightGear"), tr("Please check if the path and command is correct"));
        break;
    case QProcess::UnknownError:
    default:
        emit showCriticalMessageFromThread(tr("FlightGear Error"), tr("Please check if the path and command is correct."));
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

void QGCFlightGearLink::updateControls(quint64 time, float rollAilerons, float pitchElevator, float yawRudder, float throttle, quint8 systemMode, quint8 navMode)
{
    // magnetos,aileron,elevator,rudder,throttle\n

    //float magnetos = 3.0f;
    Q_UNUSED(time);
    Q_UNUSED(systemMode);
    Q_UNUSED(navMode);

    if(!qIsNaN(rollAilerons) && !qIsNaN(pitchElevator) && !qIsNaN(yawRudder) && !qIsNaN(throttle))
    {
        QString state("%1\t%2\t%3\t%4\t%5\n");
        state = state.arg(rollAilerons).arg(pitchElevator).arg(yawRudder).arg(true).arg(throttle);
        emit _invokeWriteBytes(state.toLatin1());
        //qDebug() << "Updated controls" << rollAilerons << pitchElevator << yawRudder << throttle;
        //qDebug() << "Updated controls" << state;
    }
    else
    {
        qDebug() << "HIL: Got NaN values from the hardware: isnan output: roll: " << qIsNaN(rollAilerons) << ", pitch: " << qIsNaN(pitchElevator) << ", yaw: " << qIsNaN(yawRudder) << ", throttle: " << qIsNaN(throttle);
    }
}

void QGCFlightGearLink::_writeBytes(const QByteArray data)
{
    //#define QGCFlightGearLink_DEBUG
#ifdef QGCFlightGearLink_DEBUG
    QString bytes;
    QString ascii;
    for (int i=0, size = data.size(); i<size; i++)
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
    if (connectState && _udpCommSocket) _udpCommSocket->writeDatagram(data, currentHost, currentPort);
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

    unsigned int s = _udpCommSocket->pendingDatagramSize();
    if (s > maxLength) std::cerr << __FILE__ << __LINE__ << " UDP datagram overflow, allowed to read less bytes than datagram size" << std::endl;
    _udpCommSocket->readDatagram(data, maxLength, &sender, &senderPort);

    QByteArray b(data, s);

    // Print string
    QString state(b);
    //qDebug() << "FG LINK GOT:" << state;

    QStringList values = state.split("\t");

    // Check length
    const int nValues = 22;
    if (values.size() != nValues)
    {
        qDebug() << "RETURN LENGTH MISMATCHING EXPECTED" << nValues << "BUT GOT" << values.size();
        qDebug() << state;
        emit showCriticalMessageFromThread(tr("FlightGear HIL"),
                                           tr("Flight Gear protocol file '%1' is out of date. Quit %2. Delete the file and restart %2 to fix.").arg(_fgProtocolFileFullyQualified).arg(qgcApp()->applicationName()));
        disconnectSimulation();
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
    float alt_agl;


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

    alt_agl = values.at(21).toFloat();

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
                                    xmag_body, ymag_body, zmag_body, abs_pressure*1e-2f, diff_pressure*1e-2f, pressure_alt, temperature, fields_changed); //Pressure in hPa for _vehicle->uas()link

//        qDebug()  << "sensorHilRawImuChanged " << xacc  << yacc << zacc  << rollspeed << pitchspeed << yawspeed << xmag << ymag << zmag << abs_pressure << diff_pressure << pressure_alt << temperature;
        int gps_fix_type = 3;
        float eph = 0.3f;
        float epv = 0.6f;
        float vel = sqrt(vx*vx + vy*vy + vz*vz);
        float cog = yaw;
        int satellites = 8;

        emit sensorHilGpsChanged(QGC::groundTimeUsecs(), lat, lon, alt, gps_fix_type, eph, epv, vel, vx, vy, vz, cog, satellites);
//        qDebug()  << "sensorHilGpsChanged " << lat  << lon << alt  << vel;

        // Send Optical Flow message. For now we set the flow quality to 0 and just write the ground_distance field
        float distanceMeasurement = -1.0; // -1 means invalid value
        if (fabsf(roll) < 0.87 && fabsf(pitch) < 0.87) // return a valid value only for decent angles
        {
            distanceMeasurement = fabsf((float)(1.0/cosPhi * 1.0/cosThe * alt_agl)); // assuming planar ground
        }
        emit sensorHilOpticalFlowChanged(QGC::groundTimeUsecs(), 0, 0, 0.0f,
                                         0.0f, 0.0f, distanceMeasurement);
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
    return _udpCommSocket->pendingDatagramSize();
}

/**
 * @brief Disconnect the connection.
 *
 * @return True if connection has been disconnected, false if connection couldn't be disconnected.
 **/
bool QGCFlightGearLink::disconnectSimulation()
{
    disconnect(_fgProcess, static_cast<void (QProcess::*)(QProcess::ProcessError)>(&QProcess::error),
            this, &QGCFlightGearLink::processError);

    disconnect(_vehicle->uas(), &UAS::hilControlsChanged, this, &QGCFlightGearLink::updateControls);

    disconnect(this, &QGCFlightGearLink::hilStateChanged, _vehicle->uas(), &UAS::sendHilState);
    disconnect(this, &QGCFlightGearLink::sensorHilGpsChanged, _vehicle->uas(), &UAS::sendHilGps);
    disconnect(this, &QGCFlightGearLink::sensorHilRawImuChanged, _vehicle->uas(), &UAS::sendHilSensors);
    disconnect(this, &QGCFlightGearLink::sensorHilOpticalFlowChanged, _vehicle->uas(), &UAS::sendHilOpticalFlow);

    if (_fgProcess)
    {
        _fgProcess->close();
        _fgProcess->deleteLater();
        _fgProcess = NULL;
    }
    if (_udpCommSocket)
    {
        _udpCommSocket->close();
        _udpCommSocket->deleteLater();
        _udpCommSocket = NULL;
    }

    connectState = false;

    emit simulationDisconnected();
    emit simulationConnected(false);

    // Exit the thread
    quit();

    return !connectState;
}

/// @brief Splits a space separated set of command line arguments into a QStringList.
///         Quoted strings are allowed and handled correctly.
///     @param uiArgs Arguments to parse
///     @param argList Returned argument list
/// @return Returns false if the argument list has mistmatced quotes within in.

bool QGCFlightGearLink::parseUIArguments(QString uiArgs, QStringList& argList)
{
    // FYI: The only reason this routine is public is so that we can reference it from a unit test.

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
                        QGCMessageBox::critical(tr("FlightGear HIL"), tr("FlightGear failed to start. There are mismatched quotes in specified command line options"));
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
            inQuotedString = !inQuotedString;
            previousSpace = false;
        } else {
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

/// @brief Locates the specified argument in the argument list, returning the value for it.
///     @param uiArgList Argument list to search through
///     @param argLabel Argument label to search for
///     @param argValue Returned argument value if found
/// @return Returns true if argument found and argValue returned

bool QGCFlightGearLink::_findUIArgument(const QStringList& uiArgList, const QString& argLabel, QString& argValue)
{
    QString regExpStr = argLabel + "=(.*)";
    int index = uiArgList.indexOf(QRegExp(regExpStr, Qt::CaseInsensitive));
    if (index != -1) {
        QRegExp regExp(regExpStr);
        index = regExp.indexIn(uiArgList[index]);
        Q_ASSERT(index != -1);
        argValue = regExp.cap(1);
        return true;
    } else {
        return false;
    }
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
    // start the various FG processes on the separate thread.

    if (!_vehicle->uas()) {
        return false;
    }

    QString     fgAppName;
    QString     fgRootPath;						// FlightGear root data directory as specified by --fg-root
    QStringList fgRootPathProposedList;         // Directories we will attempt to search for --fg-root
    bool        fgRootDirOverride = false;		// true: User has specified --fg-root from ui options
    QString     fgSceneryPath;					// FlightGear scenery path as specified by --fg-scenery
    bool        fgSceneryDirOverride = false;	// true: User has specified --fg-scenery from ui options
    QDir        fgAppDir;						// Location of main FlightGear application

    // Reset the list of arguments which will be provided to FG to the arguments set by the user via the UI
    // First split the space separated command line arguments coming in from the ui into a QStringList since
    // that is what QProcess::start needs.
    QStringList uiArgList;
    bool mismatchedQuotes = parseUIArguments(startupArguments, uiArgList);
    if (!mismatchedQuotes) {
        QGCMessageBox::critical(tr("FlightGear HIL"), tr("FlightGear failed to start. There are mismatched quotes in specified command line options"));
        return false;
    }
#ifdef DEBUG_FLIGHTGEAR_CONNECT
    qDebug() << "\nSplit arguments" << uiArgList << "\n";
#endif
    // Now set the FG arguments to the arguments from the UI
    _fgArgList = uiArgList;

#if defined Q_OS_MAC
    // Mac installs will default to the /Applications folder 99% of the time. Anything other than
    // that is pretty non-standard so we don't try to get fancy beyond hardcoding that path.
    fgAppDir.setPath("/Applications");
    fgAppName = "FlightGear.app";
    // new path
    _fgProcessName = "./fgfs";
    _fgProcessWorkingDirPath = "/Applications/FlightGear.app/Contents/MacOS/";
    if(!QFileInfo(_fgProcessWorkingDirPath + _fgProcessName).exists()){
        // old path
        _fgProcessName = "./fgfs.sh";
        _fgProcessWorkingDirPath = "/Applications/FlightGear.app/Contents/Resources/";
    }
    fgRootPathProposedList += "/Applications/FlightGear.app/Contents/Resources/data/";
#elif defined Q_OS_WIN32
    _fgProcessName = "fgfs.exe";

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
                    fgRootPathProposedList += QDir(regExp.cap(1)).absolutePath();
                    qDebug() << "fg_root" << fgRootPathProposedList[0];
                    continue;
                }

                regExp.setPattern("^fg_scenery:(.*)");
                if (regExp.indexIn(line) == 0 && regExp.captureCount() == 1) {
                    // Scenery can contain multiple paths separated by ';' so don't do QDir::absolutePath on it
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
    fgRootPathProposedList += "/usr/share/flightgear/data/";    // Default Archlinux location
    fgRootPathProposedList += "/usr/share/games/flightgear/";   // Default Ubuntu location
#else
#error Unknown OS build flavor
#endif

#ifndef Q_OS_LINUX
    // Validate the FlightGear application directory location. Linux runs from path so we don't validate on that OS.
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
        QString dirPath = QGCQFileDialog::getExistingDirectory(MainWindow::instance(), tr("Please select directory of FlightGear application : ") + fgAppName);
        if (dirPath.isEmpty()) {
            return false;
        }
        fgAppDir.setPath(dirPath);
        fgAppFullyQualified = fgAppDir.absoluteFilePath(fgAppName);
    }
#endif

    // If we have an --fg-root coming in from the ui options, that setting overrides any internal searching of
    // proposed locations.
    QString argValue;
    fgRootDirOverride = _findUIArgument(_fgArgList, "--fg-root", argValue);
    if (fgRootDirOverride) {
        fgRootPathProposedList.clear();
        fgRootPathProposedList += argValue;
        qDebug() << "--fg-root override" << argValue;
    }

    // See if we can find an --fg-root directory from the proposed list.
    Q_ASSERT(fgRootPath.isEmpty());
    for (int i=0; i<fgRootPathProposedList.count(); i++) {
        fgRootPath = fgRootPathProposedList[i];
        if (QFileInfo(fgRootPath).isDir()) {
            // We found it
            break;
        } else {
            fgRootPath.clear();
        }
    }

    // Alert the user if we couldn't find an --fg-root
    if (fgRootPath.isEmpty()) {
        QString errMsg;
        if (fgRootDirOverride) {
            errMsg = tr("--fg-root directory specified from ui option not found: %1").arg(fgRootPath);
        } else if (fgRootPath.isEmpty()) {
            errMsg = tr("Unable to automatically determine --fg-root directory location. You will need to specify --fg-root=<directory> as an additional command line parameter from ui.");
        }
        QGCMessageBox::critical(tr("FlightGear HIL"), errMsg);
        return false;
    }

    if (!fgRootDirOverride) {
        _fgArgList += "--fg-root=" + fgRootPath;
    }

    // Add --fg-scenery command line arg. If --fg-scenery is specified from the ui we use that instead.
    // On Windows --fg-scenery is required on the command line otherwise FlightGear won't boot.
    fgSceneryDirOverride = _findUIArgument(_fgArgList, "--fg-scenery", argValue);
    if (fgSceneryDirOverride) {
        fgSceneryPath = argValue;
        qDebug() << "--fg-scenery override" << argValue;
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
        QGCMessageBox::critical(tr("FlightGear HIL"), errMsg);
        return false;
    }
#else
    Q_UNUSED(fgSceneryDirOverride);
#endif

    // Setup and verify directory which contains QGC provided aircraft files
    QString qgcAircraftDir(QApplication::applicationDirPath() + "/flightgear/Aircraft");
    if (!QFileInfo(qgcAircraftDir).isDir()) {
        QGCMessageBox::critical(tr("FlightGear HIL"), tr("Incorrect %1 installation. Aircraft directory is missing: '%2'.").arg(qgcApp()->applicationName()).arg(qgcAircraftDir));
        return false;
    }
    _fgArgList += "--fg-aircraft=" + qgcAircraftDir;

    // Setup protocol we will be using to communicate with FlightGear
    QString fgProtocol(_vehicle->vehicleType() == MAV_TYPE_QUADROTOR ? "qgroundcontrol-quadrotor" : "qgroundcontrol-fixed-wing");
    QString fgProtocolArg("--generic=socket,%1,300,127.0.0.1,%2,udp,%3");
    _fgArgList << fgProtocolArg.arg("out").arg(port).arg(fgProtocol);
    _fgArgList << fgProtocolArg.arg("in").arg(currentPort).arg(fgProtocol);

    // Verify directory where FlightGear stores communicaton protocols.
    QDir fgProtocolDir(fgRootPath);
    if (!fgProtocolDir.cd("Protocol")) {
        QGCMessageBox::critical(tr("FlightGear HIL"), tr("Incorrect FlightGear setup. Protocol directory is missing: '%1'. Command line parameter for --fg-root may be set incorrectly.").arg(fgProtocolDir.path()));
        return false;
    }

    // Verify directory which contains QGC provided FlightGear communication protocol files
    QDir qgcProtocolDir(QApplication::applicationDirPath() + "/flightgear/Protocol/");
    if (!qgcProtocolDir.isReadable()) {
        QGCMessageBox::critical(tr("FlightGear HIL"), tr("Incorrect installation. Protocol directory is missing (%1).").arg(qgcProtocolDir.path()));
        return false;
    }

    // Make sure we can find the communication protocol file in QGC install
    QString fgProtocolXmlFile = fgProtocol + ".xml";
    QString qgcProtocolFileFullyQualified = qgcProtocolDir.absoluteFilePath(fgProtocolXmlFile);
    if (!QFileInfo(qgcProtocolFileFullyQualified).exists()) {
        QGCMessageBox::critical(tr("FlightGear HIL"), tr("Incorrect installation. FlightGear protocol file missing: %1").arg(qgcProtocolFileFullyQualified));
        return false;
    }

    // Communication protocol must be in FlightGear protocol directory. There does not appear to be any way
    // around this by specifying something on the FlightGear command line. FG code does direct append
    // of protocol xml file to $FG_ROOT and $FG_ROOT only allows a single directory to be specified.
    _fgProtocolFileFullyQualified = fgProtocolDir.absoluteFilePath(fgProtocolXmlFile);

    if (QFileInfo(_fgProtocolFileFullyQualified).exists()) {
        // Verify that the file is current by comparing it against the one in QGC

        QFile fgFile(_fgProtocolFileFullyQualified);
        QFile qgcFile(qgcProtocolFileFullyQualified);

        if (!fgFile.open(QIODevice::ReadOnly) ||
            !qgcFile.open(QIODevice::ReadOnly)) {
            QGCMessageBox::warning(tr("FlightGear HIL"), tr("Unable to verify that protocol file %1 is current. "
                                                            "If file is out of date, you may experience problems. "
                                                            "Safest approach is to delete the file manually and allow %2 install the latest file.").arg(qgcApp()->applicationName()).arg(_fgProtocolFileFullyQualified));
        }

        QByteArray fgBytes = fgFile.readAll();
        QByteArray qgcBytes = qgcFile.readAll();

        fgFile.close();
        qgcFile.close();

        if (fgBytes != qgcBytes) {
            QGCMessageBox::warning(tr("FlightGear HIL"), tr("FlightGear protocol file %1 is out of date. It will be deleted, which will cause %2 to install the latest version of the file.").arg(_fgProtocolFileFullyQualified).arg(qgcApp()->applicationName()));
            if (!QFile::remove(_fgProtocolFileFullyQualified)) {
                QGCMessageBox::warning(tr("FlightGear HIL"), tr("Delete of protocol file failed. You will have to manually delete the file."));
                return false;
            }
        }
    }

    if (!QFileInfo(_fgProtocolFileFullyQualified).exists()) {
        QMessageBox msgBox(QMessageBox::Critical,
                           tr("FlightGear Failed to Start"),
                           tr("FlightGear Failed to Start. %1 protocol (%2) not installed to FlightGear Protocol directory (%3)").arg(qgcApp()->applicationName()).arg(fgProtocolXmlFile).arg(fgProtocolDir.path()),
                           QMessageBox::Cancel,
                           MainWindow::instance());
        msgBox.setWindowModality(Qt::ApplicationModal);
        msgBox.addButton(tr("Fix it for me"), QMessageBox::ActionRole);
        if (msgBox.exec() == QMessageBox::Cancel) {
            return false;
        }

        // Now that we made it this far, we should be able to try to copy the protocol file to FlightGear.
        bool succeeded = QFile::copy(qgcProtocolFileFullyQualified, _fgProtocolFileFullyQualified);
        if (!succeeded) {
#ifdef Q_OS_WIN32
            QString copyCmd = QString("copy \"%1\" \"%2\"").arg(qgcProtocolFileFullyQualified).arg(_fgProtocolFileFullyQualified);
            copyCmd.replace("/", "\\");
#else
            QString copyCmd = QString("sudo cp %1 %2").arg(qgcProtocolFileFullyQualified).arg(_fgProtocolFileFullyQualified);
#endif

            QMessageBox msgBox(QMessageBox::Critical,
                               tr("Copy failed"),
#ifdef Q_OS_WIN32
                               tr("Copy from (%1) to (%2) failed, possibly due to permissions issue. You will need to perform manually. Try pasting the following command into a Command Prompt which was started with Run as Administrator:\n\n").arg(qgcProtocolFileFullyQualified).arg(_fgProtocolFileFullyQualified) + copyCmd,
#else
                               tr("Copy from (%1) to (%2) failed, possibly due to permissions issue. You will need to perform manually. Try pasting the following command into a shell:\n\n").arg(qgcProtocolFileFullyQualified).arg(_fgProtocolFileFullyQualified) + copyCmd,
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
    if (_vehicle->vehicleType() == MAV_TYPE_QUADROTOR) {
        // Start all engines of the quad
        _fgArgList << "--prop:/engines/engine[0]/running=true";
        _fgArgList << "--prop:/engines/engine[1]/running=true";
        _fgArgList << "--prop:/engines/engine[2]/running=true";
        _fgArgList << "--prop:/engines/engine[3]/running=true";
    } else {
        _fgArgList << "--prop:/engines/engine/running=true";
    }

    // We start out at our home position
    QGeoCoordinate homePosition = QGroundControlQmlGlobal::flightMapPosition();
    _fgArgList << QString("--lat=%1").arg(homePosition.latitude());
    _fgArgList << QString("--lon=%1").arg(homePosition.longitude());
    // The altitude is not set because an altitude not equal to the ground altitude leads to a non-zero default throttle in flightgear
    // Without the altitude-setting the aircraft is positioned on the ground
    //_fgArgList << QString("--altitude=%1").arg(homePosition.altitude());

#ifdef DEBUG_FLIGHTGEAR_CONNECT
    // This tell FlightGear to output highest debug level of log output. Handy for debuggin failures by looking at the FG
    // log files.
    _fgArgList << "--log-level=debug";
#endif

    start(HighPriority);
    return true;
}

void QGCFlightGearLink::_printFgfsOutput(void)
{
   qDebug() << "fgfs stdout:";
   QByteArray byteArray = _fgProcess->readAllStandardOutput();
   QStringList strLines = QString(byteArray).split("\n");

   foreach (const QString &line, strLines){
    qDebug() << line;
   }
}

void QGCFlightGearLink::_printFgfsError(void)
{
   qDebug() << "fgfs stderr:";

   QByteArray byteArray = _fgProcess->readAllStandardError();
   QStringList strLines = QString(byteArray).split("\n");

   foreach (const QString &line, strLines){
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
