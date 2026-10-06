#pragma once

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

public:
    explicit StatusTextHandler(QObject *parent = nullptr);
    ~StatusTextHandler();

    void mavlinkMessageReceived(const mavlink_message_t &message);
    void handleHTMLEscapedTextMessage(MAV_COMPONENT componentid, MAV_SEVERITY severity, const QString &text, const QString &description);

    void clearMessages();

    const QList<StatusText*>& messages() const { return m_messages; }
    QString formattedMessages() const;

    uint32_t criticalMessageCount() const { return m_criticalMessageCount; }

    static QString getMessageText(const mavlink_message_t &message);

signals:
    void newFormattedMessage(QString message);
    void textMessageReceived(MAV_COMPONENT componentid, MAV_SEVERITY severity, QString text, QString description);
    void criticalMessageCountChanged();
    void newErrorMessage(QString message);

private slots:
    void _chunkedStatusTextTimeout();

private:
    void _handleStatusText(const mavlink_message_t &message);
    void _chunkedStatusTextCompleted(MAV_COMPONENT compId);

    QTimer *m_chunkedStatusTextTimer = nullptr;

    bool m_multiComp = false;
    MAV_COMPONENT m_activeComponent = MAV_COMPONENT::MAV_COMPONENT_ENUM_END;
    uint32_t m_criticalMessageCount = 0;

    QVector<StatusText*> m_messages;

    typedef struct __ChunkedStatusTextInfo {
        uint16_t chunkId;
        MAV_SEVERITY severity;
        QStringList rgMessageChunks;
    } ChunkedStatusTextInfo_t;

    QMap<MAV_COMPONENT, ChunkedStatusTextInfo_t> m_chunkedStatusTextInfoMap;
};
