#ifndef MAVLINKDECODER_H
#define MAVLINKDECODER_H

#include <QObject>
#include "MAVLinkProtocol.h"

class MAVLinkDecoder : public QObject
{
    Q_OBJECT
public:
    MAVLinkDecoder(MAVLinkProtocol* protocol, QObject *parent = 0);

signals:
    void textMessageReceived(int uasid, int componentid, int severity, const QString& text);
    void valueChanged(const int uasId, const QString& name, const QString& unit, const double value, const quint64 msec);
    void valueChanged(const int uasId, const QString& name, const QString& unit, const int value, const quint64 msec);
    void valueChanged(const int uasId, const QString& name, const QString& unit, const unsigned int value, const quint64 msec);
    void valueChanged(const int uasId, const QString& name, const QString& unit, const quint64 value, const quint64 msec);
    void valueChanged(const int uasId, const QString& name, const QString& unit, const qint64 value, const quint64 msec);


public slots:
    /** @brief Receive one message from the protocol and decode it */
    void receiveMessage(LinkInterface* link,mavlink_message_t message);
protected:
    /** @brief Emit the value of one message field */
    void emitFieldValue(mavlink_message_t* msg, int fieldid, quint64 time);
    /** @brief Shift a timestamp in Unix time if necessary */
    quint64 getUnixTimeFromMs(int systemID, quint64 time);

    mavlink_message_t receivedMessages[256]; ///< Available / known messages
    mavlink_message_info_t messageInfo[256]; ///< Message information
    QMap<uint16_t, bool> messageFilter;               ///< Message/field names not to emit
    QMap<uint16_t, bool> textMessageFilter;           ///< Message/field names not to emit in text mode
    int componentID[256];                             ///< Multi component detection
    bool componentMulti[256];                         ///< Multi components detected
    quint64 onboardTimeOffset[256];                   ///< Offset of onboard time from Unix epoch (of the receiving GCS)
    qint64 onboardToGCSUnixTimeOffsetAndDelay[256];   ///< Offset of onboard time and GCS Unix time
    quint64 firstOnboardTime[256];                    ///< First seen onboard time

};

#endif // MAVLINKDECODER_H
