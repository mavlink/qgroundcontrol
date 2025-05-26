/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QLoggingCategory>

#include "MAVLinkLib.h"

Q_DECLARE_LOGGING_CATEGORY(StatusTextHandlerLog)

class StatusTextHandler;
class QTimer;

class StatusText
{
public:
    StatusText(MAV_COMPONENT componentid, MAV_SEVERITY severity, const QString &text);

    bool severityIsError() const;

    MAV_COMPONENT getComponentID() const { return m_compId; }
    MAV_SEVERITY getSeverity() const { return m_severity; }
    QString getText() const { return m_text; }
    QString getFormattedText() const { return m_formatedText; }

    void setFormatedText(const QString &formatedText) { m_formatedText = formatedText; }

private:
    MAV_COMPONENT m_compId;
    MAV_SEVERITY m_severity;
    QString m_text;
    QString m_formatedText;
};

class StatusTextHandler : public QObject
{
    Q_OBJECT

    enum class MessageType {
        MessageNone,
        MessageNormal,
        MessageWarning,
        MessageError
    };

public:
    explicit StatusTextHandler(QObject *parent = nullptr);
    ~StatusTextHandler();

    void mavlinkMessageReceived(const mavlink_message_t &message);
    void handleHTMLEscapedTextMessage(MAV_COMPONENT componentid, MAV_SEVERITY severity, const QString &text, const QString &description);

    void clearMessages();
    void resetAllMessages();
    void resetErrorLevelMessages();

    const QList<StatusText*>& messages() const { return m_messages; }
    QString formattedMessages() const;

    bool messageTypeNone() const { return (m_messageType == MessageType::MessageNone); }
    bool messageTypeNormal() const { return (m_messageType == MessageType::MessageNormal); }
    bool messageTypeWarning() const { return (m_messageType == MessageType::MessageWarning); }
    bool messageTypeError() const { return (m_messageType == MessageType::MessageError); }

    uint32_t getErrorCount() const { return m_errorCount; }
    uint32_t getErrorCountTotal() const { return m_errorCountTotal; }
    uint32_t getWarningCount() const { return m_warningCount; }
    uint32_t getNormalCount() const { return m_normalCount; }
    uint32_t messageCount() const { return m_messageCount; }

    static QString getMessageText(const mavlink_message_t &message);

signals:
    void newFormattedMessage(QString message);
    void textMessageReceived(MAV_COMPONENT componentid, MAV_SEVERITY severity, QString text, QString description);
    void messageCountChanged(uint32_t newCount);
    void messageTypeChanged();
    void newErrorMessage(QString message);

private slots:
    void _chunkedStatusTextTimeout();

private:
    void _handleStatusText(const mavlink_message_t &message);
    void _handleTextMessage(uint32_t newCount, MessageType messageType = MessageType::MessageNone);
    void _chunkedStatusTextCompleted(MAV_COMPONENT compId);

    QTimer *m_chunkedStatusTextTimer = nullptr;

    bool m_multiComp = false;
    MAV_COMPONENT m_activeComponent = MAV_COMPONENT::MAV_COMPONENT_ENUM_END;
    uint32_t m_errorCount = 0;
    uint32_t m_errorCountTotal = 0;
    uint32_t m_warningCount = 0;
    uint32_t m_normalCount = 0;
    uint32_t m_messageCount = 0;

    QVector<StatusText*> m_messages;

    MessageType m_messageType = MessageType::MessageNone;

    typedef struct __ChunkedStatusTextInfo {
        uint16_t chunkId;
        MAV_SEVERITY severity;
        QStringList rgMessageChunks;
    } ChunkedStatusTextInfo_t;

    QMap<MAV_COMPONENT, ChunkedStatusTextInfo_t> m_chunkedStatusTextInfoMap;
};
