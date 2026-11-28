#include "PortableTest.h"

#include <atomic>

#include <QtCore/QLoggingCategory>
#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>

namespace {

std::atomic<QtMessageHandler> s_portableQTestHandler{nullptr};
std::atomic<bool> s_portableCaptureEnabled{false};
QMutex s_portableCaptureMutex;
QList<TestLogMessage> s_portableCapturedMessages;

/// Captures a message for strict log checking, then passes it on to QTest's handler. Called on any thread
void portableCaptureHandler(QtMsgType type, const QMessageLogContext& context, const QString& message)
{
    if (s_portableCaptureEnabled.load(std::memory_order_relaxed)) {
        const QMutexLocker locker(&s_portableCaptureMutex);
        s_portableCapturedMessages.append({.type = type,
                                           .category = QString::fromLatin1(context.category ? context.category : ""),
                                           .message = message});
    }

    const QtMessageHandler handler = s_portableQTestHandler.load(std::memory_order_acquire);
    if (handler) {
        handler(type, context, message);
    }
}

}  // namespace

void PortableTest::initTestCase()
{
    // The application names each test when it registers it, and failure reports show the name
    setObjectName(QString::fromLatin1(metaObject()->className()));

    // As in the application, where QGC's categories log debug and info messages only when asked to
    QLoggingCategory::setFilterRules(QStringLiteral("*.debug=false\n*.info=false"));
    s_portableQTestHandler.store(qInstallMessageHandler(portableCaptureHandler), std::memory_order_release);
}

void PortableTest::cleanupTestCase()
{
    const QtMessageHandler previous = s_portableQTestHandler.exchange(nullptr, std::memory_order_acq_rel);
    if (previous) {
        (void) qInstallMessageHandler(previous);
    }
}

void PortableTest::startLogCapture()
{
    const QMutexLocker locker(&s_portableCaptureMutex);
    s_portableCapturedMessages.clear();
    s_portableCaptureEnabled.store(true, std::memory_order_relaxed);
}

void PortableTest::stopLogCapture()
{
    s_portableCaptureEnabled.store(false, std::memory_order_relaxed);
}

QList<TestLogMessage> PortableTest::capturedLogMessages() const
{
    const QMutexLocker locker(&s_portableCaptureMutex);
    return s_portableCapturedMessages;
}
