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

    mavlink_message_t receivedMessages[256]; ///< Available / known messages
    mavlink_message_info_t messageInfo[256]; ///< Message information
    QMap<uint16_t, bool> messageFilter;               ///< Message/field names not to emit

};

#endif // MAVLINKDECODER_H
