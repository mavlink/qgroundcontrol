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
#include <QHostInfo>

#include <iostream>

#include "QGCJSBSimLink.h"
#include "QGC.h"
#include "QGCMessageBox.h"

QGCJSBSimLink::QGCJSBSimLink(UASInterface* mav, QString startupArguments, QString remoteHost, QHostAddress host, quint16 port) :
    socket(NULL),
    process(NULL),
    startupArguments(startupArguments)
{
    // We're doing it wrong - because the Qt folks got the API wrong:
    // http://blog.qt.digia.com/blog/2010/06/17/youre-doing-it-wrong/
    moveToThread(this);

    this->host = host;
    this->port = port+mav->getUASID();
    this->connectState = false;
    this->currentPort = 49000+mav->getUASID();
    this->mav = mav;
    this->name = tr("JSBSim Link (port:%1)").arg(port);
    setRemoteHost(remoteHost);
}

QGCJSBSimLink::~QGCJSBSimLink()
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
void QGCJSBSimLink::run()
{
    qDebug() << "STARTING FLIGHTGEAR LINK";

    if (!mav) return;
    socket = new QUdpSocket(this);
    socket->moveToThread(this);
    connectState = socket->bind(host, port);

    QObject::connect(socket, SIGNAL(readyRead()), this, SLOT(readBytes()));

    process = new QProcess(this);

    connect(mav, SIGNAL(hilControlsChanged(quint64, float, float, float, float, quint8, quint8)), this, SLOT(updateControls(quint64,float,float,float,float,quint8,quint8)));
    connect(this, SIGNAL(hilStateChanged(quint64,float,float,float,float,float,float,double,double,double,float,float,float,float,float,float,float,float)), mav, SLOT(sendHilState(quint64,float,float,float,float,float,float,double,double,double,float,float,float,float,float,float,float,float)));


    UAS* uas = dynamic_cast<UAS*>(mav);
    if (uas)
    {
        uas->startHil();
    }

    //connect(&refreshTimer, SIGNAL(timeout()), this, SLOT(sendUAVUpdate()));
    // Catch process error
    QObject::connect( process, SIGNAL(error(QProcess::ProcessError)),
                      this, SLOT(processError(QProcess::ProcessError)));

    // Start Flightgear
    QStringList arguments;
    QString processJSB;
    QString rootJSB;

#ifdef Q_OS_MACX
    processJSB = "/usr/local/bin/JSBSim";
    rootJSB = "/Applications/FlightGear.app/Contents/Resources/data";
#endif

#ifdef Q_OS_WIN32
    processJSB = "C:\\Program Files (x86)\\FlightGear\\bin\\Win32\\fgfs";
    rootJSB = "C:\\Program Files (x86)\\FlightGear\\data";
#endif

#ifdef Q_OS_LINUX
    processJSB = "/usr/games/fgfs";
    rootJSB = "/usr/share/games/flightgear";
#endif

    // Sanity checks
    bool sane = true;
    QFileInfo executable(processJSB);
    if (!executable.isExecutable())
    {
        QGCMessageBox::critical("JSBSim", tr("JSBSim failed to start. JSBSim was not found at %1").arg(processJSB));
        sane = false;
    }

    QFileInfo root(rootJSB);
    if (!root.isDir())
    {
        QGCMessageBox::critical("JSBSim", tr("JSBSim failed to start. JSBSim data directory was not found at %1").arg(rootJSB));
        sane = false;
    }

    if (!sane) return;

    /*Prepare JSBSim Arguments */

    if (mav->getSystemType() == MAV_TYPE_QUADROTOR)
    {
        arguments << QString("--realtime --suspend --nice --simulation-rate=1000 --logdirectivefile=%s/flightgear.xml --script=%s/%s").arg(rootJSB).arg(rootJSB).arg(script);
    }
    else
    {
        arguments << QString("JSBSim --realtime --suspend --nice --simulation-rate=1000 --logdirectivefile=%s/flightgear.xml --script=%s/%s").arg(rootJSB).arg(rootJSB).arg(script);
    }

    process->start(processJSB, arguments);

    emit simulationConnected(connectState);
    if (connectState) {
        emit simulationConnected();
        connectionStartTime = QGC::groundTimeUsecs()/1000;
    }
    qDebug() << "STARTING SIM";

    exec();
}

void QGCJSBSimLink::setPort(int port)
{
    this->port = port;
    disconnectSimulation();
    connectSimulation();
}

void QGCJSBSimLink::processError(QProcess::ProcessError err)
{
    QString msg;
    
    switch(err) {
        case QProcess::FailedToStart:
            msg = tr("JSBSim Failed to start. Please check if the path and command is correct");
            break;
            
        case QProcess::Crashed:
            msg = tr("JSBSim crashed. This is a JSBSim-related problem, check for JSBSim upgrade.");
            break;
            
        case QProcess::Timedout:
            msg = tr("JSBSim start timed out. Please check if the path and command is correct");
            break;
            
        case QProcess::ReadError:
        case QProcess::WriteError:
            msg = tr("Could not communicate with JSBSim. Please check if the path and command are correct");
            break;
            
        case QProcess::UnknownError:
        default:
            msg = tr("JSBSim error occurred. Please check if the path and command is correct.");
            break;
    }
    
    QGCMessageBox::critical("JSBSim HIL", msg);
}

/**
 * @param host Hostname in standard formatting, e.g. localhost:14551 or 192.168.1.1:14551
 */
void QGCJSBSimLink::setRemoteHost(const QString& host)
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

void QGCJSBSimLink::updateActuators(quint64 time, float act1, float act2, float act3, float act4, float act5, float act6, float act7, float act8)
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

void QGCJSBSimLink::updateControls(quint64 time, float rollAilerons, float pitchElevator, float yawRudder, float throttle, quint8 systemMode, quint8 navMode)
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
        writeBytes(state.toLatin1().constData(), state.length());
    }
    else
    {
        qDebug() << "HIL: Got NaN values from the hardware: isnan output: roll: " << isnan(rollAilerons) << ", pitch: " << isnan(pitchElevator) << ", yaw: " << isnan(yawRudder) << ", throttle: " << isnan(throttle);
    }
    //qDebug() << "Updated controls" << state;
}

void QGCJSBSimLink::writeBytes(const char* data, qint64 size)
{
    //#define QGCJSBSimLink_DEBUG
#ifdef QGCJSBSimLink_DEBUG
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
void QGCJSBSimLink::readBytes()
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
//    QString state(b);

//    // Parse string
//    float roll, pitch, yaw, rollspeed, pitchspeed, yawspeed;
//    double lat, lon, alt;
//    double vx, vy, vz, xacc, yacc, zacc;

//    // Send updated state
//    emit hilStateChanged(QGC::groundTimeUsecs(), roll, pitch, yaw, rollspeed,
//                         pitchspeed, yawspeed, lat, lon, alt,
//                         vx, vy, vz, xacc, yacc, zacc);




        // Echo data for debugging purposes
        std::cerr << __FILE__ << __LINE__ << "Received datagram:" << std::endl;
        for (unsigned int i=0; i<s; i++)
        {
            unsigned int v=data[i];
            fprintf(stderr,"%02x ", v);
        }
        std::cerr << std::endl;
}


/**
 * @brief Get the number of bytes to read.
 *
 * @return The number of bytes to read
 **/
qint64 QGCJSBSimLink::bytesAvailable()
{
    return socket->pendingDatagramSize();
}

/**
 * @brief Disconnect the connection.
 *
 * @return True if connection has been disconnected, false if connection couldn't be disconnected.
 **/
bool QGCJSBSimLink::disconnectSimulation()
{
    disconnect(process, SIGNAL(error(QProcess::ProcessError)),
               this, SLOT(processError(QProcess::ProcessError)));
    disconnect(mav, SIGNAL(hilControlsChanged(quint64, float, float, float, float, quint8, quint8)), this, SLOT(updateControls(quint64,float,float,float,float,quint8,quint8)));
    disconnect(this, SIGNAL(hilStateChanged(quint64,float,float,float,float,float,float,double,double,double,float,float,float,float,float,float,float,float)), mav, SLOT(sendHilState(quint64,float,float,float,float,float,float,double,double,double,float,float,float,float,float,float,float,float)));

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
    return !connectState;
}

/**
 * @brief Connect the connection.
 *
 * @return True if connection has been established, false if connection couldn't be established.
 **/
bool QGCJSBSimLink::connectSimulation()
{
    start(HighPriority);
    return true;
}

/**
 * @brief Set the startup arguments used to start flightgear
 *
 **/
void QGCJSBSimLink::setStartupArguments(QString startupArguments)
{
    this->startupArguments = startupArguments;
}

/**
 * @brief Check if connection is active.
 *
 * @return True if link is connected, false otherwise.
 **/
bool QGCJSBSimLink::isConnected()
{
    return connectState;
}

QString QGCJSBSimLink::getName()
{
    return name;
}

QString QGCJSBSimLink::getRemoteHost()
{
    return QString("%1:%2").arg(currentHost.toString(), currentPort);
}

void QGCJSBSimLink::setName(QString name)
{
    this->name = name;
}
