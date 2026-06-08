#include "AndroidSerialPortRegistry.h"

#include <QtCore/QHash>
#include <QtCore/QReadWriteLock>
#include <atomic>

namespace PortRegistry {

namespace {

QReadWriteLock& registryLock()
{
    static QReadWriteLock lock;
    return lock;
}

QHash<Token, AndroidSerialPort*>& registry()
{
    static QHash<Token, AndroidSerialPort*> h;
    return h;
}

std::atomic<Token> g_nextToken{1};

}  // namespace

Token allocateToken()
{
    return g_nextToken.fetch_add(1, std::memory_order_relaxed);
}

void registerPort(Token token, AndroidSerialPort* port)
{
    QWriteLocker locker(&registryLock());
    registry().insert(token, port);
}

void unregisterPort(Token token)
{
    QWriteLocker locker(&registryLock());
    registry().remove(token);
}

void clear()
{
    QWriteLocker locker(&registryLock());
    registry().clear();
}

LookupGuard::LookupGuard(Token token) : _locker(&registryLock()), _port(registry().value(token, nullptr)) {}

}  // namespace PortRegistry
