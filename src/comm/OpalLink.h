#ifndef OPALLINK_H
#define OPALLINK_H
/**
  Connection to OpalRT.  This class receives MAVLink packets as if it is a true link, but it
  interprets the packets internally, and calls the appropriate api functions.

  \author Bryan Godbolt <godbolt@ualberta.ca>
*/

#include <QMutex>
#include <QDebug>
#include <QMessageBox>
#include <QTimer>
#include <QQueue>
#include <QByteArray>
#include <QObject>

#include "LinkInterface.h"
#include "LinkManager.h"
#include "MG.h"
#include "mavlink.h"
#include "mavlink_types.h"
#include "configuration.h"

#include "errno.h"




// FIXME
//#include "OpalApi.h"





#include "string.h"

/*
  Configuration info for the model
 */

#define NUM_OUTPUT_SIGNALS 6

class OpalLink : public LinkInterface
{
    Q_OBJECT

public:
    OpalLink();
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

    void run();

public slots:


    void writeBytes(const char *bytes, qint64 length);


    void readBytes();

    void heartbeat();

    void getSignals();

protected slots:

    void receiveMessage(mavlink_message_t message);



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
    QMutex receiveDataMutex;
    QString lastErrorMsg;
    void setLastErrorMsg();
    void displayErrorMsg();

    void setName(QString name);

    QTimer* heartbeatTimer;    ///< Timer to emit heartbeats
    int heartbeatRate;         ///< Heartbeat rate, controls the timer interval
    bool m_heartbeatsEnabled;  ///< Enabled/disable heartbeat emission

    QTimer* getSignalsTimer;
    int getSignalsPeriod;

    QQueue<QByteArray>* receiveBuffer;
    QByteArray* sendBuffer;

    const int systemID;
    const int componentID;


};

#endif // OPALLINK_H
