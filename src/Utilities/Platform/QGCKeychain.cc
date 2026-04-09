#include "QGCKeychain.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QEventLoop>
#include <QtCore/QLatin1StringView>
#include <QtCore/QSettings>
#include <atomic>
#include <mutex>
#include <qtkeychain/keychain.h>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(QGCKeychainLog, "Utilities.Platform.QGCKeychain")

namespace {

constexpr QLatin1StringView kSettingsGroup{"KeychainFallback"};

QString serviceName()
{
    return QCoreApplication::applicationName();
}

QSettings settingsStore()
{
    return QSettings(QSettings::IniFormat, QSettings::UserScope, QCoreApplication::organizationName(),
                     QCoreApplication::applicationName());
}

bool fallbackWrite(const QString& key, const QByteArray& data)
{
    QSettings s = settingsStore();
    s.beginGroup(kSettingsGroup);
    s.setValue(key, data);
    s.endGroup();
    s.sync();
    return s.status() == QSettings::NoError;
}

QByteArray fallbackRead(const QString& key)
{
    QSettings s = settingsStore();
    s.beginGroup(kSettingsGroup);
    const QByteArray result = s.value(key).toByteArray();
    s.endGroup();
    return result;
}

bool fallbackRemove(const QString& key)
{
    QSettings s = settingsStore();
    s.beginGroup(kSettingsGroup);
    s.remove(key);
    s.endGroup();
    s.sync();
    return s.status() == QSettings::NoError;
}

// Once the backend proves unavailable, skip all future probes for the session.
std::atomic<bool> g_qtkeychainUnavailable{false};

// One-time probe: avoids per-call DBus round-trip and KDE unlock prompt on a doomed session.
void probeBackendOnce()
{
    static std::once_flag flag;
    std::call_once(flag, [] {
        if (!QKeychain::isAvailable()) {
            qCInfo(QGCKeychainLog) << "qtkeychain reports no available backend; using QSettings fallback";
            g_qtkeychainUnavailable.store(true, std::memory_order_relaxed);
        }
    });
}

QKeychain::Error runJob(QKeychain::Job* job)
{
    if (!QCoreApplication::instance()) {
        qCWarning(QGCKeychainLog) << "No QCoreApplication — skipping keychain operation";
        return QKeychain::OtherError;
    }
    QEventLoop loop;
    QObject::connect(job, &QKeychain::Job::finished, &loop, &QEventLoop::quit);
    job->start();
    loop.exec();
    return job->error();
}

bool indicatesMissingBackend(QKeychain::Error err)
{
    // NoBackendAvailable = headless; AccessDenied = dismissed prompt — both treated as missing.
    return err == QKeychain::NoBackendAvailable || err == QKeychain::AccessDenied;
}

// Linux libsecret backend reports DBus ServiceUnknown as OtherError — recognize it as "no backend".
bool isMissingSecretService(QKeychain::Error err, const QString& errorString)
{
    if (err != QKeychain::OtherError) {
        return false;
    }
    return errorString.contains(QLatin1String("org.freedesktop.secrets")) ||
           errorString.contains(QLatin1String("ServiceUnknown"));
}

}  // namespace

bool QGCKeychain::writeBinary(const QString& key, const QByteArray& data)
{
    probeBackendOnce();
    if (!g_qtkeychainUnavailable.load(std::memory_order_relaxed)) {
        QKeychain::WritePasswordJob job(serviceName());
        job.setInsecureFallback(false);  // we manage our own fallback to keep one canonical store
        job.setKey(key);
        job.setBinaryData(data);
        const auto err = runJob(&job);
        if (err == QKeychain::NoError) {
            return true;
        }
        if (indicatesMissingBackend(err) || isMissingSecretService(err, job.errorString())) {
            qCInfo(QGCKeychainLog) << "qtkeychain unavailable, using QSettings fallback:" << job.errorString();
            g_qtkeychainUnavailable.store(true, std::memory_order_relaxed);
        } else {
            qCWarning(QGCKeychainLog) << "Keychain write error:" << job.errorString();
            return false;
        }
    }
    return fallbackWrite(key, data);
}

QByteArray QGCKeychain::readBinary(const QString& key)
{
    probeBackendOnce();
    if (!g_qtkeychainUnavailable.load(std::memory_order_relaxed)) {
        QKeychain::ReadPasswordJob job(serviceName());
        job.setInsecureFallback(false);
        job.setKey(key);
        const auto err = runJob(&job);
        if (err == QKeychain::NoError) {
            return job.binaryData();
        }
        if (indicatesMissingBackend(err) || isMissingSecretService(err, job.errorString())) {
            qCInfo(QGCKeychainLog) << "qtkeychain unavailable, using QSettings fallback:" << job.errorString();
            g_qtkeychainUnavailable.store(true, std::memory_order_relaxed);
            // fall through to QSettings
        } else if (err == QKeychain::EntryNotFound) {
            // Entry may have been written by a prior session with no backend — try fallback store.
        } else {
            qCWarning(QGCKeychainLog) << "Keychain read error:" << job.errorString();
            return {};
        }
    }
    return fallbackRead(key);
}

bool QGCKeychain::remove(const QString& key)
{
    probeBackendOnce();
    bool keychainOk = false;
    if (!g_qtkeychainUnavailable.load(std::memory_order_relaxed)) {
        QKeychain::DeletePasswordJob job(serviceName());
        job.setInsecureFallback(false);
        job.setKey(key);
        const auto err = runJob(&job);
        if (err == QKeychain::NoError || err == QKeychain::EntryNotFound) {
            keychainOk = true;
        } else if (indicatesMissingBackend(err) || isMissingSecretService(err, job.errorString())) {
            qCInfo(QGCKeychainLog) << "qtkeychain unavailable, using QSettings fallback:" << job.errorString();
            g_qtkeychainUnavailable.store(true, std::memory_order_relaxed);
        } else {
            qCWarning(QGCKeychainLog) << "Keychain remove error:" << job.errorString();
        }
    }
    // Always clear the QSettings fallback too — entry may live there from a prior session.
    const bool fallbackOk = fallbackRemove(key);
    return keychainOk || fallbackOk;
}
