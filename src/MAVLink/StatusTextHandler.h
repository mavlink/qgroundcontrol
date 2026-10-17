#pragma once

#include <optional>

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QObject>

#include "MAVLinkEnums.h"
#include "MAVLinkMessageType.h"

class StatusTextHandler;
class QTimer;

class StatusText
{
public:
    StatusText(MAV_COMPONENT componentid, MAV_SEVERITY severity, const QString &text);

    bool severityIsError() const;
    bool severityIsWarning() const;

    MAV_COMPONENT getComponentID() const { return m_compId; }
    MAV_SEVERITY getSeverity() const { return m_severity; }
    QString getText() const { return m_text; }
    QString getFormattedText() const { return m_formatedText; }

    bool isRead() const { return m_read; }

    void setFormatedText(const QString& formatedText) { m_formatedText = formatedText; }

    void markRead() { m_read = true; }

private:
    MAV_COMPONENT m_compId;
    MAV_SEVERITY m_severity;
    QString m_text;
    QString m_formatedText;
    bool m_read = false;
};

class StatusTextHandler : public QObject
{
    Q_OBJECT

public:
    explicit StatusTextHandler(QObject *parent = nullptr);
    ~StatusTextHandler();

    void mavlinkMessageReceived(const mavlink_message_t &message);
    void handleHTMLEscapedTextMessage(MAV_COMPONENT componentid, MAV_SEVERITY severity, const QString &text, const QString &description);

    void clearMessages();
    void markAllRead();

    const QList<StatusText*>& messages() const { return m_messages; }

    /// Unread messages are wrapped in <b> so they render bold.
    QString formattedMessages() const;

    uint32_t unreadCriticalMessageCount() const { return m_unreadCriticalMessageCount; }

    /// Unread Warning and Notice messages.
    uint32_t unreadWarningMessageCount() const { return m_unreadWarningMessageCount; }

    bool hasUnreadMessages() const { return m_unreadMessageCount != 0; }

    /// All unread messages, including critical ones.
    uint32_t unreadMessageCount() const { return m_unreadMessageCount; }

    static QString getMessageText(const mavlink_message_t &message);

signals:
    void newFormattedMessage(QString message);
    void textMessageReceived(MAV_COMPONENT componentid, MAV_SEVERITY severity, QString text, QString description);
    void unreadCriticalMessageCountChanged();
    void unreadWarningMessageCountChanged();
    void hasUnreadMessagesChanged();
    void unreadMessageCountChanged();
    void formattedMessagesChanged();
    void newErrorMessage(QString message);

private slots:
    void _chunkedStatusTextTimeout();

private:
    void _handleStatusText(const mavlink_message_t &message);
    void _chunkedStatusTextCompleted(MAV_COMPONENT compId);
    void _resetUnreadSeverityCounts();
    static QString _displayText(const StatusText& message);

    QTimer *m_chunkedStatusTextTimer = nullptr;

    bool m_multiComp = false;
    MAV_COMPONENT m_activeComponent = MAV_COMPONENT::MAV_COMPONENT_ENUM_END;
    uint32_t m_unreadCriticalMessageCount = 0;
    uint32_t m_unreadWarningMessageCount = 0;
    uint32_t m_unreadMessageCount = 0;

    QVector<StatusText*> m_messages;

    typedef struct __ChunkedStatusTextInfo {
        uint16_t chunkId;
        MAV_SEVERITY severity;
        // nullopt marks a chunk that was never received; an empty value is a received NUL-first terminator
        QList<std::optional<QByteArray>> rgMessageChunks;
    } ChunkedStatusTextInfo_t;

    QMap<MAV_COMPONENT, ChunkedStatusTextInfo_t> m_chunkedStatusTextInfoMap;
};
