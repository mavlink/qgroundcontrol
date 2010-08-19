#include "OpalLink.h"

OpalLink::OpalLink() :
        connectState(false),
        heartbeatTimer(new QTimer(this)),
        heartbeatRate(MAVLINK_HEARTBEAT_DEFAULT_RATE),
        m_heartbeatsEnabled(true),
        getSignalsTimer(new QTimer(this)),
        getSignalsPeriod(1000),
        receiveBuffer(new QQueue<QByteArray>()),
        systemID(1),
        componentID(1)
{
    start(QThread::LowPriority);

    // Set unique ID and add link to the list of links
    this->id = getNextLinkId();
    this->name = tr("OpalRT link ") + QString::number(getId());
    LinkManager::instance()->add(this);

    // Start heartbeat timer, emitting a heartbeat at the configured rate
    QObject::connect(heartbeatTimer, SIGNAL(timeout()), this, SLOT(heartbeat()));

    QObject::connect(getSignalsTimer, SIGNAL(timeout()), this, SLOT(getSignals()));
}


/*
 *
  Communication
 *
 */

qint64 OpalLink::bytesAvailable()
{
    return 0;
}

void OpalLink::writeBytes(const char *bytes, qint64 length)
{

}


void OpalLink::readBytes()
{
    receiveDataMutex.lock();
    const qint64 maxLength = 2048;
    char bytes[maxLength];
    qDebug() << "OpalLink::readBytes(): Reading a message.  size of buffer: " << receiveBuffer->count();
    QByteArray message = receiveBuffer->dequeue();
    if (maxLength < message.size())
    {
        qDebug() << "OpalLink::readBytes:: Buffer Overflow";

        memcpy(bytes, message.data(), maxLength);
    }
    else
    {
        memcpy(bytes, message.data(), message.size());
    }

    emit bytesReceived(this, message);
    receiveDataMutex.unlock();

}

void OpalLink::receiveMessage(mavlink_message_t message)
{

    // Create buffer
    char buffer[MAVLINK_MAX_PACKET_LEN];
    // Write message into buffer, prepending start sign
    int len = mavlink_msg_to_send_buffer((uint8_t*)(buffer), &message);
    // If link is connected
    if (isConnected())
    {
        receiveBuffer->enqueue(QByteArray(buffer, len));
        readBytes();
    }

}

void OpalLink::heartbeat()
{

    if (m_heartbeatsEnabled)
    {
        qDebug() << "OpalLink::heartbeat(): Generate a heartbeat";
        mavlink_message_t beat;
        mavlink_msg_heartbeat_pack(systemID, componentID,&beat, MAV_HELICOPTER, MAV_AUTOPILOT_GENERIC);
        receiveMessage(beat);
    }

}

void OpalLink::getSignals()
{
//    qDebug() <<  "OpalLink::getSignals(): Attempting to acquire signals";
//
//
//    unsigned long  timeout = 0;
//    unsigned short acqGroup = 0; //this is actually group 1 in the model
//    unsigned short allocatedSignals = NUM_OUTPUT_SIGNALS;
//    unsigned short *numSignals = new unsigned short(0);
//    double *timestep = new double(0);
//    double values[NUM_OUTPUT_SIGNALS] = {};
//    unsigned short *lastValues = new unsigned short(false);
//    unsigned short *decimation = new unsigned short(0);
//
//    int returnVal = OpalGetSignals(timeout, acqGroup, allocatedSignals, numSignals, timestep,
//                                   values, lastValues, decimation);
//
//    if (returnVal == EOK )
//    {
//        qDebug() << "OpalLink::getSignals: Timestep=" << *timestep;// << ", Last? " << (bool)(*lastValues);
//    }
//    else if (returnVal == EAGAIN)
//    {
//        qDebug() << "OpalLink::getSignals: Data was not ready";
//    }
//    // if returnVal == EAGAIN => data just wasn't ready
//    else if (returnVal != EAGAIN)
//    {
//        getSignalsTimer->stop();
//        displayErrorMsg();
//    }
//    /* deallocate used memory */
//
//    delete timestep;
//    delete lastValues;
//    delete lastValues;
//    delete decimation;

}

/*
 *
  Administrative
 *
 */
void OpalLink::run()
{
    qDebug() << "OpalLink::run():: Starting the thread";
}

int OpalLink::getId()
{
    return id;
}

QString OpalLink::getName()
{
    return name;
}

void OpalLink::setName(QString name)
{
    this->name = name;
    emit nameChanged(this->name);
}

bool OpalLink::isConnected() {
    //qDebug() << "OpalLink::isConnected:: connectState: " << connectState;
    return connectState;
}




bool OpalLink::connect()
{
    short modelState;

    /// \todo allow configuration of instid in window
//    if (OpalConnect(101, false, &modelState) == EOK)
//    {
        connectState = true;
        emit connected();
        heartbeatTimer->start(1000/heartbeatRate);
        getSignalsTimer->start(getSignalsPeriod);
//    }
//    else
//    {
//        connectState = false;
//        displayErrorMsg();
//    }

    emit connected(connectState);
    return connectState;
}

bool OpalLink::disconnect()
{
    return false;
}

void OpalLink::displayErrorMsg()
{
    setLastErrorMsg();
    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.setText(lastErrorMsg);
    msgBox.exec();
}

void OpalLink::setLastErrorMsg()
{
//    char buf[512];
//    unsigned short len;
//    OpalGetLastErrMsg(buf, sizeof(buf), &len);
//    lastErrorMsg.clear();
//    lastErrorMsg.append(buf);
}


/*
 *
  Statisctics
 *
 */

qint64 OpalLink::getNominalDataRate()
{
    return 0; //unknown
}

int OpalLink::getLinkQuality()
{
    return -1; //not supported
}

qint64 OpalLink::getTotalUpstream()
{
    statisticsMutex.lock();
    qint64 totalUpstream =  bitsSentTotal / ((MG::TIME::getGroundTimeNow() - connectionStartTime) / 1000);
    statisticsMutex.unlock();
    return totalUpstream;
}

qint64 OpalLink::getTotalDownstream() {
    statisticsMutex.lock();
    qint64 totalDownstream = bitsReceivedTotal / ((MG::TIME::getGroundTimeNow() - connectionStartTime) / 1000);
    statisticsMutex.unlock();
    return totalDownstream;
}

qint64 OpalLink::getCurrentUpstream()
{
    return 0; //unknown
}

qint64 OpalLink::getMaxUpstream()
{
    return 0; //unknown
}

qint64 OpalLink::getBitsSent() {
    return bitsSentTotal;
}

qint64 OpalLink::getBitsReceived() {
    return bitsReceivedTotal;
}


bool OpalLink::isFullDuplex()
{
    return false;
}
