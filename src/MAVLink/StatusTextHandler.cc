#include "StatusTextHandler.h"
#include "MAVLinkLib.h"
#include <QGCLoggingCategory.h>

#include <QtCore/QTimer>
#include <QtCore/QDateTime>

QGC_LOGGING_CATEGORY(StatusTextHandlerLog, "MAVLink.StatusTextHandler")

StatusText::StatusText(MAV_COMPONENT componentid, MAV_SEVERITY severity, const QString &text)
    : m_compId(componentid)
    , m_severity(severity)
    , m_text(text)
{
    // qCDebug(StatusTextHandlerLog) << Q_FUNC_INFO << this;
}

bool StatusText::severityIsError() const
{
    switch (m_severity) {
        case MAV_SEVERITY_EMERGENCY:
        case MAV_SEVERITY_ALERT:
        case MAV_SEVERITY_CRITICAL:
        case MAV_SEVERITY_ERROR:
            return true;

        default:
            return false;
    }
}

StatusTextHandler::StatusTextHandler(QObject *parent)
    : QObject(parent)
    , m_chunkedStatusTextTimer(new QTimer(this))
{
    // qCDebug(StatusTextHandlerLog) << Q_FUNC_INFO << this;

    m_chunkedStatusTextTimer->setSingleShot(true);
    m_chunkedStatusTextTimer->setInterval(1000);
    (void) connect(m_chunkedStatusTextTimer, &QTimer::timeout, this, &StatusTextHandler::_chunkedStatusTextTimeout);
}

StatusTextHandler::~StatusTextHandler()
{
    clearMessages();

    // qCDebug(StatusTextHandlerLog) << Q_FUNC_INFO << this;
}

QString StatusTextHandler::getMessageText(const mavlink_message_t &message)
{
    // Warning: There is a bug in mavlink which causes mavlink_msg_statustext_get_text to work incorrect.
    // It ends up copying crap off the end of the buffer, so don't use it for now.

    mavlink_statustext_t statusText;
    mavlink_msg_statustext_decode(&message, &statusText);

    char buffer[MAVLINK_MSG_STATUSTEXT_FIELD_TEXT_LEN + 1];
    memcpy(buffer, statusText.text, MAVLINK_MSG_STATUSTEXT_FIELD_TEXT_LEN);
    buffer[MAVLINK_MSG_STATUSTEXT_FIELD_TEXT_LEN] = '\0';

    return QString(buffer);
}

QString StatusTextHandler::formattedMessages() const
{
    QString result;
    for (const StatusText *message: messages()) {
        (void) result.prepend(message->getFormattedText());
    }

    return result;
}

void StatusTextHandler::clearMessages()
{
    qDeleteAll(m_messages);
    m_messages.clear();

    if (m_criticalMessageCount != 0) {
        m_criticalMessageCount = 0;
        emit criticalMessageCountChanged();
    }
}

void StatusTextHandler::handleHTMLEscapedTextMessage(MAV_COMPONENT compId, MAV_SEVERITY severity, const QString &text, const QString &description)
{
    QString htmlText(text);

    (void) htmlText.replace("\n", "<br/>");

    // TODO: handle text + description separately in the UI
    if (!description.isEmpty()) {
        QString htmlDescription(description);
        (void) htmlDescription.replace("\n", "<br/>");
        (void) htmlText.append(QStringLiteral("<br/><small><small>"));
        (void) htmlText.append(htmlDescription);
        (void) htmlText.append(QStringLiteral("</small></small>"));
    }

    if (m_activeComponent == MAV_COMPONENT::MAV_COMPONENT_ENUM_END) {
        m_activeComponent = compId;
    }

    if (compId != m_activeComponent) {
        m_multiComp = true;
    }

    // Color the output depending on the message severity. We have 3 distinct cases:
    // 1: If we have an ERROR or worse, make it bigger, bolder, and highlight it red.
    // 2: If we have a warning or notice, just make it bold and color it orange.
    // 3: Otherwise color it the standard color, white.
    QString style;
    switch (severity) {
        case MAV_SEVERITY_EMERGENCY:
        case MAV_SEVERITY_ALERT:
        case MAV_SEVERITY_CRITICAL:
        case MAV_SEVERITY_ERROR:
            style = QStringLiteral("<#E>");
            break;

        case MAV_SEVERITY_NOTICE:
        case MAV_SEVERITY_WARNING:
            style = QStringLiteral("<#I>");
            break;

        default:
            style = QStringLiteral("<#N>");
            break;
    }

    QString severityText;
    switch (severity) {
        case MAV_SEVERITY_EMERGENCY:
            severityText = tr("EMERGENCY");
            break;

        case MAV_SEVERITY_ALERT:
            severityText = tr("ALERT");
            break;

        case MAV_SEVERITY_CRITICAL:
            severityText = tr("Critical");
            break;

        case MAV_SEVERITY_ERROR:
            severityText = tr("Error");
            break;

        case MAV_SEVERITY_WARNING:
            severityText = tr("Warning");
            break;

        case MAV_SEVERITY_NOTICE:
            severityText = tr("Notice");
            break;

        case MAV_SEVERITY_INFO:
            severityText = tr("Info");
            break;

        case MAV_SEVERITY_DEBUG:
            severityText = tr("Debug");
            break;

        default:
            qCWarning(StatusTextHandlerLog) << Q_FUNC_INFO << "Invalid MAV_SEVERITY";
            break;
    }

    QString compString;
    if (m_multiComp) {
        compString = QString("COMP:%1").arg(compId);
    }

    const QString dateString = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");

    const QString formatText = QString("<font style=\"%1\">[%2 %3] %4: %5</font><br/>").arg(style, dateString, compString, severityText, htmlText);

    StatusText* const message = new StatusText(compId, severity, text);
    message->setFormatedText(formatText);

    emit newFormattedMessage(formatText);

    (void) m_messages.append(message);

    if (message->severityIsError()) {
        m_criticalMessageCount++;
        emit criticalMessageCountChanged();
        emit newErrorMessage(message->getText());
    }
}

void StatusTextHandler::mavlinkMessageReceived(const mavlink_message_t &message)
{
    if (message.msgid != MAVLINK_MSG_ID_STATUSTEXT) {
        return;
    }

    _handleStatusText(message);
}

void StatusTextHandler::_handleStatusText(const mavlink_message_t &message)
{
    mavlink_statustext_t statustext;
    mavlink_msg_statustext_decode(&message, &statustext);

    const QString messageText = getMessageText(message);
    const bool includesNullTerminator = messageText.length() < MAVLINK_MSG_STATUSTEXT_FIELD_TEXT_LEN;

    const MAV_COMPONENT compId = static_cast<MAV_COMPONENT>(message.compid);
    if (m_chunkedStatusTextInfoMap.contains(compId) && (m_chunkedStatusTextInfoMap.value(compId).chunkId != statustext.id)) {
        // We have an incomplete chunked status still pending
        (void) m_chunkedStatusTextInfoMap.value(compId).rgMessageChunks.append(QString());
        _chunkedStatusTextCompleted(compId);
    }

    if (statustext.id == 0) {
        // Non-chunked status text. We still use common chunked text output mechanism.
        ChunkedStatusTextInfo_t chunkedInfo;
        chunkedInfo.chunkId = 0;
        chunkedInfo.severity = static_cast<MAV_SEVERITY>(statustext.severity);
        (void) chunkedInfo.rgMessageChunks.append(messageText);
        (void) m_chunkedStatusTextInfoMap.insert(compId, chunkedInfo);
    } else {
        if (m_chunkedStatusTextInfoMap.contains(compId)) {
            // A chunk sequence is in progress
            QStringList& chunks = m_chunkedStatusTextInfoMap[compId].rgMessageChunks;
            if (statustext.chunk_seq > chunks.size()) {
                // We are missing some chunks in between, fill them in as missing
                for (size_t i = chunks.size(); i < statustext.chunk_seq; i++) {
                    (void) chunks.append(QString());
                }
            }

            (void) chunks.append(messageText);
        } else {
            // Starting a new chunk sequence
            ChunkedStatusTextInfo_t chunkedInfo;
            chunkedInfo.chunkId = statustext.id;
            chunkedInfo.severity = static_cast<MAV_SEVERITY>(statustext.severity);
            (void) chunkedInfo.rgMessageChunks.append(messageText);
            (void) m_chunkedStatusTextInfoMap.insert(compId, chunkedInfo);
        }

        m_chunkedStatusTextTimer->start();
    }

    if ((statustext.id == 0) || includesNullTerminator) {
        m_chunkedStatusTextTimer->stop();
        _chunkedStatusTextCompleted(compId);
    }
}

void StatusTextHandler::_chunkedStatusTextTimeout()
{
    for (auto compId : m_chunkedStatusTextInfoMap.keys()) {
        auto& chunkedInfo = m_chunkedStatusTextInfoMap[compId];
        (void) chunkedInfo.rgMessageChunks.append(QString());
        _chunkedStatusTextCompleted(compId);
    }
}

void StatusTextHandler::_chunkedStatusTextCompleted(MAV_COMPONENT compId)
{
    const ChunkedStatusTextInfo_t& chunkedInfo = m_chunkedStatusTextInfoMap.value(compId);
    const MAV_SEVERITY severity = chunkedInfo.severity;

    QString messageText;
    for (const QString& chunk : std::as_const(chunkedInfo.rgMessageChunks)) {
        if (chunk.isEmpty()) {
            (void) messageText.append(tr(" ... ", "Indicates missing chunk from chunked STATUS_TEXT"));
        } else {
            (void) messageText.append(chunk);
        }
    }

    (void) m_chunkedStatusTextInfoMap.remove(compId);

    emit textMessageReceived(compId, severity, messageText, "");
}
