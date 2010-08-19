/*=====================================================================

PIXHAWK Micro Air Vehicle Flying Robotics Toolkit

(c) 2009 PIXHAWK PROJECT  <http://pixhawk.ethz.ch>

This file is part of the PIXHAWK project

    PIXHAWK is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    PIXHAWK is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with PIXHAWK. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

/**
 * @file
 *   @brief Brief Description
 *
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <SerialSimulationLink.h>
#include <QTime>
#include <QFile>
#include <QDebug>
#include <MG.h>

/**
 * Create a simulated link. This link is connected to an input and output file.
 * The link sends one line at a time at the specified sendrate. The timing of
 * the sendrate is free of drift, which means it is stable on the long run.
 * However, small deviations are mixed in which vary the sendrate slightly
 * in order to simulate disturbances on a real communication link.
 *
 * @param readFile The file with the messages to read (must be in ASCII format, line breaks can be Unix or Windows style)
 * @param writeFile The received messages are written to that file
 * @param sendrate The rate at which the messages are sent (in intervals of milliseconds)
 **/
SerialSimulationLink::SerialSimulationLink(QFile* readFile, QFile* writeFile, int sendrate)
{
    // If a non-empty portname is supplied, the serial simulation link should attempt loopback simulation
    loopBack = NULL;

    /* Comments on the variables can be found in the header file */

    lineBuffer = QByteArray();
    lineBuffer.clear();
    readyBuffer = QByteArray();
    readyBuffer.clear();
    simulationFile = readFile;
    receiveFile = writeFile;
    lastSent = MG::TIME::getGroundTimeNow();

    /* Initialize the pseudo-random number generator */
    srand(QTime::currentTime().msec());
    maxTimeNoise = 0;

    timer = new QTimer(this);
    QObject::connect(timer, SIGNAL(timeout()), this, SLOT(readLine()));
    _isConnected = false;
    rate = sendrate;
}

SerialSimulationLink::~SerialSimulationLink()
{
    //TODO Check destructor
    fileStream->flush();
    outStream->flush();
}

void SerialSimulationLink::run()
{
    /*
        forever {
                quint64 currentTime = OG::TIME::getGroundTimeNow();
                if(currentTime - lastSent >= rate) {
                        lastSent = currentTime;
                        readLine();
                }

                msleep(rate);
        }*/
}

void SerialSimulationLink::enableLoopBackMode(SerialLink* loop)
{
    // Lock the data
    readyBufferMutex.lock();
    // Disconnect this link
    disconnect();

    // Delete previous loopback link if exists
    if(loopBack != NULL)
    {
        delete loopBack;
        loopBack = NULL;
    }

    // Set new loopback link
    loopBack = loop;
    // Connect signals
    QObject::connect(loopBack, SIGNAL(connected()), this, SIGNAL(connected()));
    QObject::connect(loopBack, SIGNAL(disconnected()), this, SIGNAL(disconnected()));
    QObject::connect(loopBack, SIGNAL(connected(bool)), this, SIGNAL(connected(bool)));
    QObject::connect(loopBack, SIGNAL(bytesReady(LinkInterface*)), this, SIGNAL(bytesReady(LinkInterface*)));
    readyBufferMutex.unlock();

}


qint64 SerialSimulationLink::bytesAvailable()
{
    readyBufferMutex.lock();
    qint64 size = 0;
    if(loopBack == 0)
    {
        size = readyBuffer.size();
    }
    else
    {
        size = loopBack->bytesAvailable();
    }
    readyBufferMutex.unlock();

    return size;
}

void SerialSimulationLink::writeBytes(char* data, qint64 length)
{
    /* Write bytes to one line */
    for(qint64 i = 0; i < length; i++)
    {
        outStream->operator <<(data[i]);
        outStream->flush();
    }

}


void SerialSimulationLink::readBytes()
{
    const qint64 maxLength = 2048;
    char data[maxLength];
    /* Lock concurrent resource readyBuffer */
    readyBufferMutex.lock();
    if(loopBack == NULL)
    {
        // FIXME Maxlength has no meaning here
        /* copy leftmost maxLength bytes and remove them from buffer */
        qstrncpy(data, readyBuffer.left(maxLength).data(), maxLength);
        readyBuffer.remove(0, maxLength);
    }
    else
    {
        //loopBack->readBytes(data, maxLength);
    }
    readyBufferMutex.unlock();
}

/**
 * @brief Reads a line at a time of the simulation data file and sends it.
 *
 * The data is read binary, which means that the file doesn't have to contain
 * only ASCII characters. The line break (independent of operating system) is
 * NOT read. The line gets sent as a whole. Because the next line is buffered,
 * the line gets sent instantly when the function is called.
 *
 * @bug The time noise addition is commented out because it adds some delay
 *      which leads to a drift in the timer. This can be fixed by multithreading.
 **/
void SerialSimulationLink::readLine()
{

    if(_isConnected)
    {
        /* The order of operations in this method is arranged to
                 * minimize the impact of slow file read operations on the
                 * message emit timing. The functions should be kept in this order
                 */

        /* (1) Add noise for next iteration (noise is always 0 for maxTimeNoise = 0) */
        addTimeNoise();

        /* (2) Save content of line buffer in readyBuffer (has to be lock for thread safety)*/
        readyBufferMutex.lock();
        if(loopBack == NULL)
        {
            readyBuffer.append(lineBuffer);
            //qDebug() << "readLine readyBuffer: " << readyBuffer;
        }
        else
        {
            loopBack->writeBytes(lineBuffer.data(), lineBuffer.size());
        }
        readyBufferMutex.unlock();

        if(loopBack == NULL)
        {
            readBytes();
        }

        /* (4) Read one line and save it in line buffer */
        lineBuffer.clear();

        // Remove whitespaces, tabs and line breaks
        QString readString = fileStream->readLine().trimmed();
        readString.remove(" ");
        readString.remove("\t");
        readString.remove("\n");
        readString.remove("\v");
        readString.remove("\r");
        lineBuffer.append(readString.toAscii());

        //qDebug() << "SerialSimulationLink::readLine()" << readString.size() << readString;

        /* Check if end of file has been reached, start from the beginning if necessary
                 * This has to be done after the last read, otherwise the timer is out of sync */
        if (fileStream->atEnd()) {
            simulationFile->reset();
        }

    }

}

/**
 * Set the maximum time deviation noise. This amount (in milliseconds) is
 * the maximum time offset (+/-) from the specified message send rate.
 *
 * @param milliseconds The maximum time offset (in milliseconds)
 *
 * @bug The current implementation might induce one milliseconds additional
 * 		 discrepancy, this will be fixed by multithreading
 **/
void SerialSimulationLink::setMaximumTimeNoise(int milliseconds)
{
    maxTimeNoise = milliseconds;
}


/**
 * Add or subtract a pseudo random time offset. The maximum time offset is
 * defined by setMaximumTimeNoise().
 *
 * @see setMaximumTimeNoise()
 **/
void SerialSimulationLink::addTimeNoise()
{
    /* Calculate the time deviation */
    if(maxTimeNoise == 0) {
        /* Don't do expensive calculations if no noise is desired */
        timer->setInterval(rate);
    } else {
        /* Calculate random time noise (gauss distribution):
                 *
                 * (1) (2 * rand()) / RAND_MAX: Number between 0 and 2
                 * (induces numerical noise through floating point representation,
                 * ignored here)
                 *
                 * (2) ((2 * rand()) / RAND_MAX) - 1: Number between -1 and 1
                 *
                 * (3) Complete term: Number between -maxTimeNoise and +maxTimeNoise
                 */
        double timeDeviation = (((2 * rand()) / RAND_MAX) - 1) * maxTimeNoise;
        timer->setInterval(static_cast<int>(rate + floor(timeDeviation)));
    }

}

/**
 * Disconnect the connection.
 *
 * @return True if connection has been disconnected, false if connection
 * couldn't be disconnected.
 **/
bool SerialSimulationLink::disconnect() {

    if(isConnected()) {
        timer->stop();

        fileStream->flush();
        outStream->flush();

        simulationFile->close();
        receiveFile->close();

        _isConnected = false;

        if(loopBack == NULL) {
            emit disconnected();
        } else {
            loopBack->disconnect();
        }

        exit();
    }

    return true;
}

/**
 * Connect the link.
 *
 * @return True if connection has been established, false if connection
 * couldn't be established.
 **/
bool SerialSimulationLink::connect()
{
    /* Open files */
    //@TODO Add check if file can be read
    simulationFile->open(QIODevice::ReadOnly);

    /* Create or replace output file */
    if(receiveFile->exists()) receiveFile->remove(); //TODO Read return value if file has been removed
    receiveFile->open(QIODevice::WriteOnly);

    fileStream = new QTextStream(simulationFile);
    outStream = new QTextStream(receiveFile);

    /* Initialize line buffer */
    lineBuffer.clear();
    lineBuffer.append(fileStream->readLine().toAscii());

    _isConnected = true;

    if(loopBack == NULL) {
        emit connected();
    } else {
        loopBack->connect();
    }

    start(LowPriority);
    timer->start(rate);
    return true;
}

/**
 * Check if connection is active.
 *
 * @return True if link is connected, false otherwise.
 **/
bool SerialSimulationLink::isConnected()
{
    return _isConnected;
}

qint64 SerialSimulationLink::getNominalDataRate()
{
    /* 100 Mbit is reasonable fast and sufficient for all embedded applications */
    return 100000000;
}

qint64 SerialSimulationLink::getTotalUpstream()
{
    return 0;
    //TODO Add functionality here
    // @todo Add functionality here
}

qint64 SerialSimulationLink::getShortTermUpstream()
{
    return 0;
}

qint64 SerialSimulationLink::getCurrentUpstream()
{
    return 0;
}

qint64 SerialSimulationLink::getMaxUpstream()
{
    return 0;
}

qint64 SerialSimulationLink::getBitsSent()
{
    return 0;
}

qint64 SerialSimulationLink::getBitsReceived()
{
    return 0;
}

qint64 SerialSimulationLink::getTotalDownstream()
{
    return 0;
}

qint64 SerialSimulationLink::getShortTermDownstream()
{
    return 0;
}

qint64 SerialSimulationLink::getCurrentDownstream()
{
    return 0;
}

qint64 SerialSimulationLink::getMaxDownstream()
{
    return 0;
}

bool SerialSimulationLink::isFullDuplex()
{
    /* Full duplex is no problem when running in pure software, but this is a serial simulation */
    return false;
}

int SerialSimulationLink::getLinkQuality()
{
    /* The Link quality is always perfect when running in software */
    return 100;
}

bool SerialSimulationLink::setPortName(QString portName)
{
    Q_UNUSED(portName);
    return true;
}

bool SerialSimulationLink::setBaudRate(int rate)
{
    Q_UNUSED(rate);
    return true;
}

bool SerialSimulationLink::setFlowType(int type)
{
    Q_UNUSED(type);
    return true;
}

bool SerialSimulationLink::setParityType(int type)
{
    Q_UNUSED(type);
    return true;
}

bool SerialSimulationLink::setDataBitsType(int type)
{
    Q_UNUSED(type);
    return true;
}

bool SerialSimulationLink::setStopBitsType(int type)
{
    Q_UNUSED(type)
    return true;
}

QString SerialSimulationLink::getPortName()
{
    return tr("simulated/port");
}

int SerialSimulationLink::getBaudRate()
{
    return 115200;
}

int SerialSimulationLink::getBaudRateType()
{
    return 19;
}

int SerialSimulationLink::getFlowType()
{
    return 0;
}

int SerialSimulationLink::getParityType()
{
    return 0;
}

int SerialSimulationLink::getDataBitsType()
{
    return 8;
}

int SerialSimulationLink::getStopBitsType()
{
    return 2;
}

bool SerialSimulationLink::setBaudRateType(int rateIndex)
{
    Q_UNUSED(rateIndex);
    return true;
}
