#pragma once

#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QtMessageHandler>

/// Captures warnings and debug output of the GPS.Driver.Protocols categories while alive, so protocol diagnostics
/// never reach the strict test log; every other message goes to the handler that was installed before. Captures nest.
class GPSProtocolLogCapture
{
public:
    struct Message
    {
        QString category;
        QtMsgType type = QtDebugMsg;
        QString text;
    };

    GPSProtocolLogCapture()
        : _outer(s_active)
    {
        s_active = this;
        _previous = qInstallMessageHandler(&GPSProtocolLogCapture::_handle);
    }

    ~GPSProtocolLogCapture()
    {
        qInstallMessageHandler(_previous);
        s_active = _outer;
    }

    GPSProtocolLogCapture(const GPSProtocolLogCapture&) = delete;
    GPSProtocolLogCapture& operator=(const GPSProtocolLogCapture&) = delete;

    const QList<Message>& messages() const { return _messages; }

    QStringList warnings() const
    {
        QStringList result;
        for (const auto& message : _messages) {
            if (message.type == QtWarningMsg) {
                result.append(message.text);
            }
        }
        return result;
    }

    QStringList categories() const
    {
        QStringList result;
        for (const auto& message : _messages) {
            result.append(message.category);
        }
        return result;
    }

private:
    static void _handle(QtMsgType type, const QMessageLogContext& context, const QString& text)
    {
        const QString category = QString::fromLatin1(context.category ? context.category : "");
        if (s_active && type != QtCriticalMsg && type != QtFatalMsg &&
            category.startsWith(QLatin1StringView("GPS.Driver.Protocols"))) {
            s_active->_messages.append({category, type, text});
            return;
        }
        // A nested capture's previous handler is this one; pass the message to the handler outside all captures.
        for (const auto* capture = s_active; capture; capture = capture->_outer) {
            if (capture->_previous != &GPSProtocolLogCapture::_handle) {
                if (capture->_previous) {
                    capture->_previous(type, context, text);
                }
                return;
            }
        }
    }

    static inline GPSProtocolLogCapture* s_active = nullptr;
    GPSProtocolLogCapture* _outer = nullptr;
    QtMessageHandler _previous = nullptr;
    QList<Message> _messages;
};
