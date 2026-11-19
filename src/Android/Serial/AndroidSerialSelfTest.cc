// QGC's boot-time proof that C++ and Java reach each other in the app as it is packaged. It uses the JNI layer as a
// port does, through AndroidSerialJni.h, and knows nothing of JNI itself.

#include <utility>

#include <QtCore/QByteArray>
#include <QtCore/QObject>
#include <QtCore/QStringList>
#include <QtCore/QThread>

#include "AndroidSerialJni.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(AndroidSerialSelfTestLog, "Android.AndroidSerial.SelfTest")

namespace AndroidSerial {

bool selfTest()
{
    // Notes what Java's callbacks deliver.
    struct Echo final : PortListener
    {
        bool newDataArrived(const char* bytes, int length) override
        {
            heard.append(QStringLiteral("data ") + QString::fromLatin1(QByteArray(bytes, length).toHex()));
            return true;
        }

        void bytesWritten(int count) override { heard.append(QStringLiteral("written %1").arg(count)); }

        void resourceErrorArrived(ResourceErrorReason reason, const QString& detail) override
        {
            heard.append(QStringLiteral("resource error %1 %2").arg(static_cast<int>(reason)).arg(detail));
        }

        QStringList heard;
    } echo;

    // One byte more than Java is told to deliver, bytes that are negative in Java, a count that needs more than
    // 16 bits and a text that is not ASCII: what a wrong signature or conversion would change.
    static constexpr char kData[] = {'\x00', '\x7f', '\x80', '\xff', '\x51'};
    constexpr int kDataLength = 4;
    constexpr int kWritten = 70000;
    const QString detail = QStringLiteral("d\u00e9tail");
    const QStringList expected = {
        QStringLiteral("data 007f80ff"),
        QStringLiteral("written 70000"),
        QStringLiteral("resource error %1 %2").arg(static_cast<int>(ResourceErrorReason::WriteFailed)).arg(detail),
        QStringLiteral("devices changed"),
    };

    DeviceWatcher* const watcher = DeviceWatcher::instance();
    // Direct, because Java calls back on this thread before echoToCallbacks returns. An Android thread emits the
    // signal as well, for a device that is attached or detached or a permission that is answered meanwhile.
    // That is no part of the echo, and the thread must not write to it.
    const Qt::HANDLE selfTestThread = QThread::currentThreadId();
    const QMetaObject::Connection watching = QObject::connect(
        watcher, &DeviceWatcher::devicesChanged, watcher,
        [&echo, selfTestThread]() {
            if (QThread::currentThreadId() == selfTestThread) {
                echo.heard.append(QStringLiteral("devices changed"));
            }
        },
        Qt::DirectConnection);
    const PortHandle handle = registerPort(&echo);

    const Result echoed = echoToCallbacks(handle, kData, static_cast<int>(sizeof(kData)), kDataLength, kWritten,
                                          ResourceErrorReason::WriteFailed, detail);

    unregisterPort(handle);
    (void) QObject::disconnect(watching);

    QStringList failures;
    // A method or field that Java lacks is only logged when the IDs are cached, and QGC runs on without it. The
    // calls here reach three of the methods and none of the fields.
    const QStringList unbound = unboundJavaNames();
    for (const QString& name : unbound) {
        failures.append(QStringLiteral("Java %1 is not bound").arg(name));
    }
    if (echoed != Result::Ok) {
        failures.append(QStringLiteral("echoToCallbacks returned %1").arg(static_cast<int>(echoed)));
    }
    if (echo.heard != expected) {
        failures.append(QStringLiteral("Java's callbacks delivered (%1), not (%2)")
                            .arg(echo.heard.join(QStringLiteral(", ")), expected.join(QStringLiteral(", "))));
    }

    // Calls that Java itself answers, with a result that a call which fails on its way cannot return.
    const int opened = open(QStringLiteral("usb:self-test"), handle);
    if (opened != static_cast<int>(Result::DeviceNotFound)) {
        failures.append(QStringLiteral("Opening a device that does not exist returned %1, not %2")
                            .arg(opened)
                            .arg(static_cast<int>(Result::DeviceNotFound)));
    }
    const int written = writeAsync(handle, kData, static_cast<int>(sizeof(kData)));
    if (written != static_cast<int>(Result::NotOpen)) {
        failures.append(QStringLiteral("Writing to a port that is not open returned %1, not %2")
                            .arg(written)
                            .arg(static_cast<int>(Result::NotOpen)));
    }

    for (const QString& failure : std::as_const(failures)) {
        qCCritical(AndroidSerialSelfTestLog).noquote() << failure;
    }
    if (failures.isEmpty()) {
        qCInfo(AndroidSerialSelfTestLog) << "USB serial JNI self-test passed";
    }

    return failures.isEmpty();
}

}  // namespace AndroidSerial
