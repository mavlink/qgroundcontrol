#pragma once

// Thread-safe token→port map for the JNI boundary; fences port lifetime against in-flight callbacks (see LookupGuard).

#include <QtCore/QReadLocker>
#include <QtCore/QtTypes>

class AndroidSerialPort;

namespace PortRegistry {

// Token type matches the JNI jlong width (both 64-bit signed); kept jni-free so the registry builds and unit-tests on the host.
using Token = qint64;

// Strictly-increasing, never reused — JNI tokens go stale on close() rather than being recycled.
Token allocateToken();

void registerPort(Token token, AndroidSerialPort* port);
void unregisterPort(Token token);

// Drop all token->port entries; called from JNI_OnUnload so a same-process native reload starts clean.
void clear();

// RAII guard for JNI callback entry: its read lock fences port lifetime so the port can't be destroyed mid-callback (UAF). QPointer won't do — it nulls after destruction, not before.
class LookupGuard
{
public:
    explicit LookupGuard(Token token);

    LookupGuard(const LookupGuard&) = delete;
    LookupGuard& operator=(const LookupGuard&) = delete;

    AndroidSerialPort* port() const { return _port; }

    explicit operator bool() const { return _port != nullptr; }

private:
    QReadLocker _locker;
    AndroidSerialPort* _port;
};

}  // namespace PortRegistry
