#include "UomRegistrationCredentials.h"

#include <QtConcurrent/QtConcurrentRun>
#include <QtCore/QFutureWatcher>
#include <QtCore/QTimer>

#ifdef QGC_UOM_LIBSECRET
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

#pragma push_macro("signals")
#undef signals
#include <libsecret/secret.h>
#pragma pop_macro("signals")
#endif

namespace {
struct SecretResult
{
    QString value{};
    bool success = false;
    bool persistent = false;
};

#ifdef QGC_UOM_LIBSECRET
SecretResult keyringOperation(const QString& reference, const QString& value, int operation)
{
    static const SecretSchema schema = []() {
        SecretSchema schemaValue{};
        schemaValue.name = "org.mavlink.qgroundcontrol.uom";
        schemaValue.flags = SECRET_SCHEMA_NONE;
        schemaValue.attributes[0] = {"reference", SECRET_SCHEMA_ATTRIBUTE_STRING};
        return schemaValue;
    }();
    GCancellable* cancellable = g_cancellable_new();
    std::mutex mutex;
    std::condition_variable condition;
    bool done = false;
    std::thread deadline([&]() {
        std::unique_lock lock(mutex);
        if (!condition.wait_for(lock, std::chrono::seconds(10), [&]() { return done; })) {
            g_cancellable_cancel(cancellable);
        }
    });
    GError* error = nullptr;
    SecretResult result;
    const QByteArray name = reference.toUtf8();
    if (operation == 0) {
        const QByteArray secret = value.toUtf8();
        result.success =
            secret_password_store_sync(&schema, SECRET_COLLECTION_DEFAULT, "QGroundControl UOM", secret.constData(),
                                       cancellable, &error, "reference", name.constData(), nullptr);
        result.value = value;
        result.persistent = result.success;
    } else if (operation == 1) {
        gchar* secret =
            secret_password_lookup_sync(&schema, cancellable, &error, "reference", name.constData(), nullptr);
        if (secret) {
            result.value = QString::fromUtf8(secret);
            result.success = result.persistent = true;
            secret_password_free(secret);
        }
    } else {
        secret_password_clear_sync(&schema, cancellable, &error, "reference", name.constData(), nullptr);
        result.success = error == nullptr;
    }
    if (error) {
        g_error_free(error);
    }
    {
        std::lock_guard lock(mutex);
        done = true;
    }
    condition.notify_one();
    deadline.join();
    g_object_unref(cancellable);
    return result;
}
#endif
}  // namespace

UomRegistrationCredentials::UomRegistrationCredentials(QObject* parent, bool memoryOnly)
    : QObject(parent)
    , _memoryOnly(memoryOnly)
{
    _worker.setMaxThreadCount(1);
#ifndef QGC_UOM_LIBSECRET
    _memoryOnly = true;
#endif
}

UomRegistrationCredentials::~UomRegistrationCredentials()
{
    _worker.clear();
    _worker.waitForDone();
}

void UomRegistrationCredentials::put(const QString& reference, const QString& value)
{
    if (value.isEmpty()) {
        remove(reference);
        return;
    }
    _values.insert(reference, value);
    _persistent.remove(reference);
    _run(reference, value, 0);
}

void UomRegistrationCredentials::get(const QString& reference)
{
    if (_values.contains(reference)) {
        QTimer::singleShot(0, this, [this, reference]() {
            emit completed(reference, _values.contains(reference), persistent(reference),
                           persistent(reference) ? tr("Stored in the system keyring")
                                                 : tr("Stored in memory for this session only"));
        });
        return;
    }
    _run(reference, {}, 1);
}

void UomRegistrationCredentials::remove(const QString& reference)
{
    _values.remove(reference);
    _persistent.remove(reference);
    _run(reference, {}, 2);
}

void UomRegistrationCredentials::_run(const QString& reference, const QString& value, int operation)
{
    const quint64 generation = ++_generations[reference];
    const auto finish = [this, reference, generation, operation, value](const SecretResult& result) {
        if (_generations.value(reference) != generation) {
            return;
        }
        if (result.persistent) {
            _values.insert(reference, result.value);
            _persistent.insert(reference);
        }
        const bool success = operation == 0 ? !value.isEmpty() : result.success;
        emit completed(
            reference, success, result.persistent,
            result.persistent
                ? tr("Stored in the system keyring")
                : (operation == 2 ? tr("Credential removed from memory")
                                  : tr("System keyring unavailable; credentials are kept only for this session")));
    };
    if (_memoryOnly) {
        QTimer::singleShot(0, this, [finish, operation]() { finish(SecretResult{.success = operation != 1}); });
        return;
    }
#ifdef QGC_UOM_LIBSECRET
    auto* watcher = new QFutureWatcher<SecretResult>(this);
    connect(watcher, &QFutureWatcher<SecretResult>::finished, this, [watcher, finish]() {
        finish(watcher->result());
        watcher->deleteLater();
    });
    watcher->setFuture(QtConcurrent::run(
        &_worker, [reference, value, operation]() { return keyringOperation(reference, value, operation); }));
#endif
}
