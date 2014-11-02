


#ifndef PORTLISTENER_H_
#define PORTLISTENER_H_

#include <QObject>
#include "qextserialport.h"

class PortListener : public QObject
{
Q_OBJECT
public:
    PortListener(const QString &portName);

private:
    QextSerialPort *port;

private slots:
    void onReadyRead();
    void onDsrChanged(bool status);

};


#endif /*PORTLISTENER_H_*/
