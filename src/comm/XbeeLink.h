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
	QString getPortName() const;
	void requestReset() { }
	int getBaudRate() const;
    
    // These are left unimplemented in order to cause linker errors which indicate incorrect usage of
    // connect/disconnect on link directly. All connect/disconnect calls should be made through LinkManager.
    bool connect(void);
    bool disconnect(void);

public slots: // virtual functions from XbeeLinkInterface
	bool setPortName(QString portName);
	bool setBaudRate(int rate);
	bool setRemoteAddressHigh(quint32 high);
	bool setRemoteAddressLow(quint32 low);

public:
    // virtual functions from LinkInterface
    QString getName() const;
    bool isConnected() const;

    // Extensive statistics for scientific purposes
    qint64 getConnectionSpeed() const;
    qint64 getCurrentOutDataRate() const;
    qint64 getCurrentInDataRate() const;

private slots: // virtual functions from LinkInterface
	void _writeBytes(const QByteArray bytes);

protected slots: // virtual functions from LinkInterface
	void readBytes();

public:
	void run(); // initiating the thread

protected:
	xbee_con *m_xbeeCon;
	char *m_portName;
	unsigned int m_portNameLength;
	int m_baudRate;
	bool m_connected;
	QString m_name;
	quint32 m_addrHigh;
	quint32 m_addrLow;

private:
    // From LinkInterface
    virtual bool _connect(void);
    virtual void _disconnect(void);

    bool hardwareConnect();
	//void CALLTYPE portCallback(xbee_con *XbeeCon, xbee_pkt *XbeePkt);
};


#endif // _XBEELINK_H_
