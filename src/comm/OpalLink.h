#ifndef OPALLINK_H
#define OPALLINK_H
/**
  Connection to OpalRT.  This class receives MAVLink packets as if it is a true link, but it
  interprets the packets internally, and calls the appropriate api functions.

  \author Bryan Godbolt <godbolt@ualberta.ca>
*/

#include <QMutex>

#include "LinkInterface.h"
#include "LinkManager.h"
#include "MG.h"

#include "errno.h"
#include "OpalApi.h"

class OpalLink : public LinkInterface
{
    Q_OBJECT
    /* Connection management */

    int getId();
    QString getName();
    bool isConnected();

    /* Connection characteristics */


    qint64 getNominalDataRate();
    bool isFullDuplex();
    int getLinkQuality();
    qint64 getTotalUpstream();
    qint64 getTotalDownstream();
    qint64 getCurrentUpstream();
    qint64 getMaxUpstream();
    qint64 getBitsSent();
    qint64 getBitsReceived();


    bool connect();


    bool disconnect();


    qint64 bytesAvailable();

public slots:


    void writeBytes(const char *bytes, qint64 length);


    void readBytes(char *bytes, qint64 maxLength);


public:
    OpalLink();

protected:
    QString name;
    int id;
    bool connectState;

    quint64 bitsSentTotal;
    quint64 bitsSentCurrent;
    quint64 bitsSentMax;
    quint64 bitsReceivedTotal;
    quint64 bitsReceivedCurrent;
    quint64 bitsReceivedMax;
    quint64 connectionStartTime;

    QMutex statisticsMutex;

    void setName(QString name);
};

#endif // OPALLINK_H
