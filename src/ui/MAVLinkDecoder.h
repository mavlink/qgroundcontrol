#ifndef MAVLINKDECODER_H
#define MAVLINKDECODER_H

#include <QObject>
#include "MAVLinkProtocol.h"

class MAVLinkDecoder : public QThread
{
    Q_OBJECT
public:
    MAVLinkDecoder(MAVLinkProtocol* protocol);

    void run();

signals:
    void textMessageReceived(int uasid, int componentid, int severity, const QString& text);
    void valueChanged(const int uasId, const QString& name, const QString& unit, const QVariant& value, const quint64 msec);
    void finish(); ///< Trigger a thread safe shutdown

public slots:
    /** @brief Receive one message from the protocol and decode it */
    void receiveMessage(LinkInterface* link,mavlink_message_t message);
protected:
    /** @brief Emit the value of one message field */
    void emitFieldValue(mavlink_message_t* msg, int fieldid, quint64 time);
    /** @brief Shift a timestamp in Unix time if necessary */
    quint64 getUnixTimeFromMs(int systemID, quint64 time);

    static const size_t cMessageIds = 256;

    mavlink_message_t receivedMessages[cMessageIds];        ///< Available / known messages
    QMap<uint16_t, bool> messageFilter;                     ///< Message/field names not to emit
    QMap<uint16_t, bool> textMessageFilter;                 ///< Message/field names not to emit in text mode
    int componentID[cMessageIds];                           ///< Multi component detection
    bool componentMulti[cMessageIds];                       ///< Multi components detected
    quint64 onboardTimeOffset[cMessageIds];                 ///< Offset of onboard time from Unix epoch (of the receiving GCS)
    qint64 onboardToGCSUnixTimeOffsetAndDelay[cMessageIds]; ///< Offset of onboard time and GCS Unix time
    quint64 firstOnboardTime[cMessageIds];                  ///< First seen onboard time
    QThread* creationThread;                                ///< QThread on which the object is created
};

#endif // MAVLINKDECODER_H
