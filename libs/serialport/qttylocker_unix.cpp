/****************************************************************************
**
** Copyright (C) 2012 Denis Shienkov <denis.shienkov@gmail.com>
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtSerialPort module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qttylocker_unix_p.h"

#ifdef HAVE_BAUDBOY_H
#  include <baudboy.h>
#  include <cstdlib>
#elif defined (HAVE_LOCKDEV_H)
#  include <lockdev.h>
#  include <unistd.h>
#else
#  include <signal.h>
#  include <errno.h>
#  include <fcntl.h>
#  include <sys/stat.h>
#  include <unistd.h>
#  include <QtCore/qfile.h>
#  include <QtCore/qdir.h>
#  include <QtCore/qstringlist.h>
#endif // defined (HAVE_BAUDBOY_H)

QT_BEGIN_NAMESPACE

#if !(defined (HAVE_BAUDBOY_H) || defined (HAVE_LOCKDEV_H))

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
#  define QStringLiteral QLatin1String
#endif

static inline const QStringList& lockDirectoryList()
{
    static const QStringList lockDirectoryEntries = QStringList()
        << QStringLiteral("/var/lock")
        << QStringLiteral("/etc/locks")
        << QStringLiteral("/var/spool/locks")
        << QStringLiteral("/var/spool/uucp")
        << QStringLiteral("/tmp");

    return lockDirectoryEntries;
}

// Returns the full path first found in the directory where you can create a lock file
// (ie a directory with access to the read/write).
// Verification of directories is of the order in accordance with the order
// of records in the variable lockDirList.
static
QString lookupFirstSharedLockDir()
{
    QStringList directoryList = lockDirectoryList();

    foreach (const QString &lockDirectory, directoryList) {
        if (::access(lockDirectory.toLocal8Bit().constData(), R_OK | W_OK) == 0)
            return lockDirectory;
    }
    return QString();
}

// Returns the name of the lock file which is tied to the
// device name, eg "LCK..ttyS0", etc.
static
QString generateLockFileNameAsNamedForm(const char *portName)
{
    QString result(lookupFirstSharedLockDir());
    if (!result.isEmpty()) {
        result.append(QLatin1String("/LCK.."));
        result.append(QString::fromLatin1(portName).replace(QLatin1Char('/'), QLatin1Char('_')));
    }
    return result;
}

#endif //!(defined (HAVE_BAUDBOY_H) || defined (HAVE_LOCKDEV_H))

// Try lock serial device. However, other processes can not access it.
bool QTtyLocker::lock(const char *portName)
{
#ifdef HAVE_BAUDBOY_H
    if (::ttylock(portName)
        ::ttywait(portName);
    return ::ttylock(portName) != -1;
#elif defined (HAVE_LOCKDEV_H)
    return ::dev_lock(portName) != -1;
#else
    QFile f(generateLockFileNameAsNamedForm(portName));
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QString content(QLatin1String("     %1 %2\x0A"));
        content = content.arg(::getpid()).arg(::getuid());
        if (f.write(content.toLocal8Bit()) > 0) {
            f.close();
            return true;
        }
        f.close();
    }
    return false;
#endif
}

// Try unlock serial device. However, other processes can access it.
bool QTtyLocker::unlock(const char *portName)
{
#ifdef HAVE_BAUDBOY_H
    return ::ttyunlock(portName != -1;
#elif defined (HAVE_LOCKDEV_H)
    return ::dev_unlock(portName, ::getpid()) != -1;
#else
    QFile f(generateLockFileNameAsNamedForm(portName));
    return f.remove();
#endif
}

// Verifies the device is locked or not.
// If returned currentPid = true - this means that the device is locked the current process.
bool QTtyLocker::isLocked(const char *portName, bool *currentPid)
{
    if (!currentPid)
        return true;

    *currentPid = false;

#ifdef HAVE_BAUDBOY_H
    return ::ttylocked(portName) != -1;
#elif defined (HAVE_LOCKDEV_H)
    return ::dev_testlock(portName) != -1;
#else

    QFile f(generateLockFileNameAsNamedForm(portName));
    if (!f.exists())
        return false;
    if (!f.open(QIODevice::ReadOnly))
        return true;

    QString content(QLatin1String(f.readAll()));
    f.close();

    const pid_t pid = content.section(' ', 0, 0, QString::SectionSkipEmpty).toInt();

    if (::kill(pid, 0) == -1) {
        if (errno == ESRCH) // Process does not exists
            return false;
    } else {
        if (::getpid() == pid) // Process exists and it is "their", i.e current
            *currentPid = true;
    }

    return true;

#endif
}

QT_END_NAMESPACE
