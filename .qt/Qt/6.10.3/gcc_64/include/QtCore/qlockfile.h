// Copyright (C) 2013 David Faure <faure+bluesystems@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QLOCKFILE_H
#define QLOCKFILE_H

#include <QtCore/qstring.h>
#include <QtCore/qscopedpointer.h>

#include <chrono>

QT_BEGIN_NAMESPACE

class QLockFilePrivate;

class Q_CORE_EXPORT QLockFile
{
public:
    explicit QLockFile(const QString &fileName);
    ~QLockFile();

    QString fileName() const;

    bool lock();
    QT_CORE_INLINE_SINCE(6, 10)
    bool tryLock(int timeout);
    void unlock();

    QT_CORE_INLINE_SINCE(6, 10)
    void setStaleLockTime(int);
    QT_CORE_INLINE_SINCE(6, 10)
    int staleLockTime() const;

    bool tryLock(std::chrono::milliseconds timeout = std::chrono::milliseconds::zero());

    void setStaleLockTime(std::chrono::milliseconds value);
    std::chrono::milliseconds staleLockTimeAsDuration() const;

    bool isLocked() const;
    bool getLockInfo(qint64 *pid, QString *hostname, QString *appname) const;
    bool removeStaleLockFile();

    enum LockError {
        NoError = 0,
        LockFailedError = 1,
        PermissionError = 2,
        UnknownError = 3
    };
    LockError error() const;

protected:
    QScopedPointer<QLockFilePrivate> d_ptr;

private:
    Q_DECLARE_PRIVATE(QLockFile)
    Q_DISABLE_COPY(QLockFile)
};

#if QT_CORE_INLINE_IMPL_SINCE(6, 10)
bool QLockFile::tryLock(int timeout)
{
    return tryLock(std::chrono::milliseconds{timeout});
}

void QLockFile::setStaleLockTime(int staleLockTime)
{
    setStaleLockTime(std::chrono::milliseconds{staleLockTime});
}

int QLockFile::staleLockTime() const
{
    return int(staleLockTimeAsDuration().count());
}
#endif // QT_CORE_INLINE_IMPL_SINCE(6, 10)

QT_END_NAMESPACE

#endif // QLOCKFILE_H
