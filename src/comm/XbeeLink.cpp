#include <qdebug.h>
#include <QThread>
#include <QMutex>
#include <MG.h>
#include <configuration.h>
#include<string>
#include "XbeeLink.h"

XbeeLink::XbeeLink(QString portName, int baudRate) : 
	m_xbeeCon(NULL), m_portName(NULL), m_portNameLength(0), m_baudRate(baudRate), m_connected(false), m_id(-1),
	m_addrHigh(0), m_addrLow(0)
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
	this->disconnect();
}

QString XbeeLink::getPortName()
{
	QString portName;
	for(unsigned int i = 0;i<this->m_portNameLength;i++)
	{
		portName.append(this->m_portName[i]);
	}
	return portName;
}

int XbeeLink::getBaudRate()
{
	return this->m_baudRate;
}

bool XbeeLink::setPortName(QString portName)
{
	bool reconnect(false);
	if(this->m_connected)
	{
		this->disconnect();
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
			this->m_portName[i]=list[0][i].toAscii();
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
		retVal = this->connect();
	}

	return retVal;
}

bool XbeeLink::setBaudRate(int rate)
{
	bool reconnect(false);
	if(this->m_connected)
	{
		this->disconnect();
		reconnect = true;
	}
	bool retVal(true);
	this->m_baudRate = rate;
	if(reconnect)
	{
		retVal = this->connect();
	}
	return retVal;
}

int XbeeLink::getId()
{
	return this->m_id;
}

QString XbeeLink::getName()
{
	return this->m_name;
}

bool XbeeLink::isConnected()
{
	return this->m_connected;
}

qint64 XbeeLink::getNominalDataRate()
{
	return this->m_baudRate;
}

bool XbeeLink::isFullDuplex()
{
	return false;
}

int XbeeLink::getLinkQuality()
{
	return -1; // TO DO:
}

qint64 XbeeLink::getTotalUpstream()
{
	return 0; // TO DO:
}

qint64 XbeeLink::getCurrentUpstream()
{
	return 0; // TO DO:
}

qint64 XbeeLink::getMaxUpstream()
{
	return 0; // TO DO:
}

qint64 XbeeLink::getBitsSent()
{
	return 0; // TO DO:
}

qint64 XbeeLink::getBitsReceived()
{
	return 0; // TO DO:
}

bool XbeeLink::hardwareConnect()
{
	emit tryConnectBegin(true);
	if(this->isConnected())
	{
		this->disconnect();
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
	emit connected(true);
	return true;
}

bool XbeeLink::connect()
{
	if (this->isRunning()) this->disconnect();
    this->start(LowPriority);
    return true;
}

bool XbeeLink::disconnect()
{
	if(this->isRunning()) this->terminate(); //stop running the thread, restart it upon connect

	if(this->m_xbeeCon)
	{
		xbee_end();
		this->m_xbeeCon = NULL;
	}
	this->m_connected = false;

	emit disconnected();
	emit connected(false);
	return true;
}

qint64 XbeeLink::bytesAvailable()
{
	return 0;
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
	}
	else
	{
		this->disconnect();
		emit communicationError(this->getName(), tr("Could not send data - link %1 is disconnected!").arg(this->getName()));
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
		qDebug() << data;
		emit bytesReceived(this,data);
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