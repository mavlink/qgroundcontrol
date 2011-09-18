#ifndef _XBEELINK_H_
#define _XBEELINK_H_

#include <QObject>
#include <QString>
#include <qdebug.h>
#include <qmutex.h>
#include <qbytearray.h>
#include <list>
#include "XbeeLinkInterface.h"
#include <xbee.h>
#include "CallConv.h"

class XbeeLink : public XbeeLinkInterface 
{
	Q_OBJECT

public:
	XbeeLink(QString portName = "", int baudRate=57600);
	~XbeeLink();
	
public: // virtual functions from XbeeLinkInterface
	QString getPortName();
	int getBaudRate();

public slots: // virtual functions from XbeeLinkInterface
	bool setPortName(QString portName);
	bool setBaudRate(int rate);
	bool setRemoteAddressHigh(quint32 high);
	bool setRemoteAddressLow(quint32 low);

public: // virtual functions from LinkInterface
	int getId();
	QString getName();
	bool isConnected();
	qint64 getNominalDataRate();
	bool isFullDuplex();
	int getLinkQuality();
	qint64 getTotalUpstream();
	qint64 getCurrentUpstream();
	qint64 getMaxUpstream();
	qint64 getBitsSent();
	qint64 getBitsReceived();
	bool connect();
	bool disconnect();
	qint64 bytesAvailable();

public slots: // virtual functions from LinkInterface
	void writeBytes(const char *bytes, qint64 length);

protected slots: // virtual functions from LinkInterface
	void readBytes();

public:
	void run(); // initiating the thread

protected:
	xbee_con *m_xbeeCon;
	int m_id;
	char *m_portName;
	unsigned int m_portNameLength;
	int m_baudRate;
	bool m_connected;
	QString m_name;
	quint32 m_addrHigh;
	quint32 m_addrLow;

private:
	bool hardwareConnect();
	//void CALLTYPE portCallback(xbee_con *XbeeCon, xbee_pkt *XbeePkt);
};


#endif // _XBEELINK_H_