#pragma once

#include <atomic>
#include <functional>

#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtCore/QStringList>
#include <QtNetwork/QTcpSocket>

#include "NTRIPConfiguration.h"
#include "QGCLoggingCategoryManager.h"

namespace NTRIPTestSupport {

class DebugCapture
{
public:
    explicit DebugCapture(const char* category)
        : _category(category)
    {
        _active = this;
        _previousHandler =
            qInstallMessageHandler([](QtMsgType type, const QMessageLogContext& context, const QString& message) {
                auto* capture = _active.load();
                if (capture && type == QtDebugMsg && qstrcmp(context.category, capture->_category) == 0) {
                    {
                        QMutexLocker lock(&capture->_mutex);
                        capture->_messages.append(message);
                    }
                    if (const auto handler = _previousHandler.load()) {
                        handler(type, context, message);
                    }
                    if (capture->onMessage) {
                        capture->onMessage();
                    }
                } else if (const auto handler = _previousHandler.load()) {
                    handler(type, context, message);
                }
            });
        auto* logging = QGCLoggingCategoryManager::instance();
        _wasEnabled = logging->isCategoryEnabled(QString::fromLatin1(_category));
        if (!_wasEnabled) {
            logging->setCategoryEnabled(QString::fromLatin1(_category), true);
        }
    }

    ~DebugCapture()
    {
        if (!_wasEnabled) {
            QGCLoggingCategoryManager::instance()->setCategoryEnabled(QString::fromLatin1(_category), false);
        }
        qInstallMessageHandler(_previousHandler.load());
        _active = nullptr;
    }

    QStringList messages() const
    {
        QMutexLocker lock(&_mutex);
        return _messages;
    }

    std::function<void()> onMessage;

private:
    const char* _category;
    mutable QMutex _mutex;
    QStringList _messages;
    static inline std::atomic<DebugCapture*> _active{nullptr};
    static inline std::atomic<QtMessageHandler> _previousHandler{nullptr};
    bool _wasEnabled = false;
};

class WriteSocket : public QTcpSocket
{
public:
    explicit WriteSocket(QObject* parent)
        : QTcpSocket(parent)
    {
        open(QIODevice::WriteOnly);
    }

    std::function<qint64(qint64)> admit;

protected:
    qint64 writeData(const char*, qint64 size) override { return admit ? admit(size) : size; }
};

inline NTRIPConnectionConfig config()
{
    NTRIPConnectionConfig result;
    result.host = QStringLiteral("127.0.0.1");
    result.mountpoint = QStringLiteral("TEST");
    return result;
}

}  // namespace NTRIPTestSupport
