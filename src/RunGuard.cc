/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "RunGuard.h"

#include <QtCore/QCryptographicHash>
#include <QtCore/QDir>
#include <QtCore/QStandardPaths>

RunGuard::RunGuard(const QString &key)
    : _key(key)
    , _lockFilePath(lockDir() + QLatin1String("/qgc-") + generateKeyHash(key, QLatin1String("_lock")) + QLatin1String(".lock"))
    , _lockFile(_lockFilePath)
{
    // Recover instantly from stale locks after crashes.
    _lockFile.setStaleLockTime(0);
}

RunGuard::~RunGuard()
{
    release();
}

bool RunGuard::isAnotherRunning()
{
    if (_lockFile.isLocked()) {
        return false;
    }

    if (_lockFile.tryLock(0)) {
        _lockFile.unlock();
        return false;
    }

    switch (_lockFile.error()) {
    case QLockFile::NoError:
        return false;
    case QLockFile::LockFailedError:
        return true;
    case QLockFile::PermissionError:
        qWarning() << "QLockFile PermissionError: Unable to access lock file at" << _lockFile.fileName();
        break;
    case QLockFile::UnknownError:
        qWarning() << "QLockFile UnknownError: An unknown error occurred with lock file at" << _lockFile.fileName();
        break;
    default:
        break;
    }

    return true;
}

bool RunGuard::tryToRun()
{
    return (_lockFile.isLocked() ? true : _lockFile.tryLock(0));
}

void RunGuard::release()
{
    if (_lockFile.isLocked()) {
        _lockFile.unlock();
    }
}

QString RunGuard::generateKeyHash(const QString &key, const QString &salt)
{
    QByteArray data;
    (void) data.append(key.toUtf8());
    (void) data.append(salt.toUtf8());
    const QByteArray hex = QCryptographicHash::hash(data, QCryptographicHash::Sha1).toHex();
    return QString::fromLatin1(hex);
}

QString RunGuard::lockDir()
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    if (dir.isEmpty()) {
        dir = QDir::tempPath();
    }

    if (!QDir().mkpath(dir)) {
        qWarning() << "RunGuard: Failed to create lock directory:" << dir;
        return QString();
    }

    return dir;
}
