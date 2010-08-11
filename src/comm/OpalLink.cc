#include "OpalLink.h"

OpalLink::OpalLink()
{

    // Set unique ID and add link to the list of links
    this->id = getNextLinkId();
    this->name = tr("OpalRT link ") + QString::number(getId());
    LinkManager::instance()->add(this);
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
    return connectState;
}

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

bool OpalLink::connect()
{
    return false;
}

bool OpalLink::disconnect()
{
    return false;
}

qint64 OpalLink::bytesAvailable()
{
    return 0;
}
