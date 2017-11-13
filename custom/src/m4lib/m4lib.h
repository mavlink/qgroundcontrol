#pragma once

#include "TyphoonHCommon.h"
#include "m4serial.h"

class M4Lib : public QObject
{
    Q_OBJECT
public:

    void init();
    void deinit();

    bool write(QByteArray data, bool debug);
    void tryRead();

    M4Lib(QObject* parent = NULL);
    ~M4Lib();

signals:
    void bytesReady(QByteArray data);
private slots:
    void _bytesReady(QByteArray data);

private:
    M4SerialComm* _commPort;
};
