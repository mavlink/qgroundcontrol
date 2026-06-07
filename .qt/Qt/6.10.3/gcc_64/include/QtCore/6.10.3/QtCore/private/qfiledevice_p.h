// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QFILEDEVICE_P_H
#define QFILEDEVICE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "private/qiodevice_p.h"
#include "qfiledevice.h"

#include <memory>
#if defined(Q_OS_UNIX)
#  include <sys/types.h> // for mode_t
#  include <sys/stat.h> // for mode_t constants
#elif defined(Q_OS_WINDOWS)
#  include <qt_windows.h>
#  include <winnt.h> // for SECURITY_DESCRIPTOR
#  include <optional>
#  if defined(QT_BOOTSTRAPPED)
#    define QT_FEATURE_fslibs -1
#  else
#    define QT_FEATURE_fslibs 1
#  endif // QT_BOOTSTRAPPED
#endif

QT_BEGIN_NAMESPACE

class QAbstractFileEngine;
class QFSFileEngine;

class QFileDevicePrivate : public QIODevicePrivate
{
    Q_DECLARE_PUBLIC(QFileDevice)
protected:
    QFileDevicePrivate();
    ~QFileDevicePrivate();

public:
    virtual QAbstractFileEngine *engine() const;

protected:
    inline bool ensureFlushed() const;

    bool putCharHelper(char c) override;

    void setError(QFileDevice::FileError err);
    void setError(QFileDevice::FileError err, const QString &errorString);
    void setError(QFileDevice::FileError err, int errNum);

    mutable std::unique_ptr<QAbstractFileEngine> fileEngine;
    mutable qint64 cachedSize;

    QFileDevice::FileHandleFlags handleFlags;
    QFileDevice::FileError error;

    bool lastWasWrite;
};

inline bool QFileDevicePrivate::ensureFlushed() const
{
    // This function ensures that the write buffer has been flushed (const
    // because certain const functions need to call it.
    if (lastWasWrite) {
        const_cast<QFileDevicePrivate *>(this)->lastWasWrite = false;
        if (!const_cast<QFileDevice *>(q_func())->flush())
            return false;
    }
    return true;
}

#ifdef Q_OS_UNIX
namespace QtPrivate {

constexpr mode_t toMode_t(QFileDevice::Permissions permissions)
{
    mode_t mode = 0;
    if (permissions & (QFileDevice::ReadOwner | QFileDevice::ReadUser))
        mode |= S_IRUSR;
    if (permissions & (QFileDevice::WriteOwner | QFileDevice::WriteUser))
        mode |= S_IWUSR;
    if (permissions & (QFileDevice::ExeOwner | QFileDevice::ExeUser))
        mode |= S_IXUSR;
    if (permissions & QFileDevice::ReadGroup)
        mode |= S_IRGRP;
    if (permissions & QFileDevice::WriteGroup)
        mode |= S_IWGRP;
    if (permissions & QFileDevice::ExeGroup)
        mode |= S_IXGRP;
    if (permissions & QFileDevice::ReadOther)
        mode |= S_IROTH;
    if (permissions & QFileDevice::WriteOther)
        mode |= S_IWOTH;
    if (permissions & QFileDevice::ExeOther)
        mode |= S_IXOTH;
    return mode;
}

} // namespace QtPrivate
#elif defined(Q_OS_WINDOWS)

class QNativeFilePermissions
{
public:
    QNativeFilePermissions(std::optional<QFileDevice::Permissions> perms, bool isDir);

    SECURITY_ATTRIBUTES *securityAttributes();
    bool isOk() const { return ok; }

private:
    bool ok = false;
    bool isNull = true;

    // At most 1 allow + 1 deny ACEs for user and group, 1 allow ACE for others
    static constexpr auto MaxNumACEs = 5;

    static constexpr auto MaxACLSize =
            sizeof(ACL) + (sizeof(ACCESS_ALLOWED_ACE) + SECURITY_MAX_SID_SIZE) * MaxNumACEs;

    SECURITY_ATTRIBUTES sa;
#if QT_CONFIG(fslibs)
    SECURITY_DESCRIPTOR sd;
    alignas(DWORD) char aclStorage[MaxACLSize];
#endif
};

#endif // Q_OS_UNIX

QT_END_NAMESPACE

#endif // QFILEDEVICE_P_H
