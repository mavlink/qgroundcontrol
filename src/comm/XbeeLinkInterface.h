#ifndef XBEELINKINTERFACE_H_
#define XBEELINKINTERFACE_H_

#include <QObject>
#include <QString>
#include <LinkInterface.h>

class XbeeLinkInterface : public LinkInterface
{
    Q_OBJECT

public:
    virtual QString getPortName() = 0;
    virtual int getBaudRate() = 0;

public slots:
    virtual bool setPortName(QString portName) = 0;
    virtual bool setBaudRate(int rate) = 0;
	virtual bool setRemoteAddressHigh(quint32 high) = 0;
	virtual bool setRemoteAddressLow(quint32 low) = 0;

signals:
	void tryConnectBegin(bool toTrue);
	void tryConnectEnd(bool toTrue);
};

#endif // XBEELINKINTERFACE_H_
