#include <qdebug.h>
#include <QThread>
#include <QMutex>
#include <MG.h>
#include <configuration.h>
#include<string>
#include "XbeeLink.h"

XbeeLink::XbeeLink(QString portName, int baudRate) : 
	m_xbeeCon(NULL),
    m_id(-1),
    m_portName(NULL),
    m_portNameLength(0),
    m_baudRate(baudRate),
    m_connected(false),
	m_addrHigh(0),
    m_addrLow(0)
{

	/* setup the xbee */
	this->setPortName(portName);
	
	//this->connect();
	// Set unique ID and add link to the list of links
	this->m_id = getNextLinkId();
	// set the Name
	this->m_name = tr("xbee link") + QString::number(this->m_id);
	emit nameChanged(this->m_name);
}

XbeeLink::~XbeeLink()
{
	if(m_portName)
	{
		delete m_portName;
		m_portName = NULL;
	}
	_disconnect();
}

QString XbeeLink::getPortName() const
{
	QString portName;
	for(unsigned int i = 0;i<this->m_portNameLength;i++)
	{
		portName.append(this->m_portName[i]);
	}
	return portName;
}

int XbeeLink::getBaudRate() const
{
	return this->m_baudRate;
}

bool XbeeLink::setPortName(QString portName)
{
	bool reconnect(false);
	if(this->m_connected)
	{
		_disconnect();
		reconnect = true;
	}
	if(m_portName)
	{
		delete m_portName;
		m_portName = NULL;
	}
	QStringList list = portName.split(QRegExp("\\s+"),QString::SkipEmptyParts);
	if(list.size()>0)
	{
		this->m_portNameLength = list[0].size()+1;
		m_portName = new char[this->m_portNameLength];
		for(int i=0;i<list[0].size();i++)
		{
			this->m_portName[i]=list[0][i].toLatin1();
		}
		this->m_portName[list[0].size()] = '\0';
	}
	else
	{
		this->m_portNameLength = 1;
		m_portName = new char[this->m_portNameLength];
		this->m_portName[0] = '\0';
	}
	
	bool retVal(true);
	if(reconnect)
	{
		retVal = _connect();
	}

	return retVal;
}

bool XbeeLink::setBaudRate(int rate)
{
	bool reconnect(false);
	if(this->m_connected)
	{
		_disconnect();
		reconnect = true;
	}
	bool retVal(true);
	this->m_baudRate = rate;
	if(reconnect)
	{
		retVal = _connect();
	}
	return retVal;
}

int XbeeLink::getId() const
{
	return this->m_id;
}

QString XbeeLink::getName() const
{
	return this->m_name;
}

bool XbeeLink::isConnected() const
{
	return this->m_connected;
}

qint64 XbeeLink::getConnectionSpeed() const
{
	return this->m_baudRate;
}

qint64 XbeeLink::getCurrentInDataRate() const
{
    return 0;
}

qint64 XbeeLink::getCurrentOutDataRate() const
{
    return 0;
}

bool XbeeLink::hardwareConnect()
{
	emit tryConnectBegin(true);
	if(this->isConnected())
	{
		_disconnect();
	}
	if (*this->m_portName == '\0')
	{
		return false;
	}
	if (xbee_setupAPI(this->m_portName,this->m_baudRate,0x2B,0x3E8) == -1) 
		{
		  /* oh no... it failed */
			qDebug() <<"xbee_setup() failed...\n";
			emit tryConnectEnd(true);
			return false;
		}
	this->m_xbeeCon = xbee_newcon('A',xbee2_data,0x13A200,0x403D0935);
	emit tryConnectEnd(true);
	this->m_connected = true;
	emit connected();
	return true;
}

bool XbeeLink::_connect(void)
{
	if (this->isRunning()) _disconnect();
    this->start(LowPriority);
    return true;
}

bool XbeeLink::_disconnect(void)
{
	if(this->isRunning()) this->terminate(); //stop running the thread, restart it upon connect

	if(this->m_xbeeCon)
	{
		xbee_end();
		this->m_xbeeCon = NULL;
	}
	this->m_connected = false;

	emit disconnected();
	return true;
}

void XbeeLink::writeBytes(const char *bytes, qint64 length)  // TO DO: delete the data array
{
	char *data;
	data = new char[length];
	for(long i=0;i<length;i++)
	{
		data[i] = bytes[i];
	}
	if(!xbee_nsenddata(this->m_xbeeCon,data,length)) // return value of 0 is successful written
	{
        // Log the amount and time written out for future data rate calculations.
        QMutexLocker dataRateLocker(&dataRateMutex);
        logDataRateToBuffer(outDataWriteAmounts, outDataWriteTimes, &outDataIndex, length, QDateTime::currentMSecsSinceEpoch());
	}
	else
	{
		_disconnect();
		emit communicationError(tr("Link Error"), QString("Error on link: %1. Could not send data - link is disconnected!").arg(getName()));
	}
}

void XbeeLink::readBytes()
{
	xbee_pkt *xbeePkt;
	xbeePkt = xbee_getpacketwait(this->m_xbeeCon);
	if(!(NULL==xbeePkt))
	{
		QByteArray data;
		for(unsigned int i=0;i<=xbeePkt->datalen;i++)
		{
			data.push_back(xbeePkt->data[i]);
        }

        emit bytesReceived(this, data);

        // Log the amount and time received for future data rate calculations.
        QMutexLocker dataRateLocker(&dataRateMutex);
        logDataRateToBuffer(inDataWriteAmounts, inDataWriteTimes, &inDataIndex, data.length(), QDateTime::currentMSecsSinceEpoch());
	}
}

void XbeeLink::run()
{
    // Initialize the connection
	if(this->hardwareConnect())
	{
		// Qt way to make clear what a while(1) loop does
		forever 
		{
			this->readBytes();
	    }
	}
}

bool XbeeLink::setRemoteAddressHigh(quint32 high)
{
	this->m_addrHigh = high;
	return true;
}

bool XbeeLink::setRemoteAddressLow(quint32 low)
{
	this->m_addrLow = low;
	return true;
}

/*
void CALLTYPE XbeeLink::portCallback(xbee_con *xbeeCon, xbee_pkt *XbeePkt)
{
	QByteArray buf;
	for(quint8 i=0;i<XbeePkt->datalen;i++)
	{
		buf.push_back(XbeePkt->data[i]);
	}
	emit bytesReceived(this, buf);
}*/
