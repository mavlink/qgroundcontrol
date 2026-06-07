// Copyright (C) 2022 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QTIPCCOMMON_P_H
#define QTIPCCOMMON_P_H

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

#include "qtipccommon.h"
#include <private/qglobal_p.h>
#include <private/qtcore-config_p.h>

#if QT_CONFIG(sharedmemory) || QT_CONFIG(systemsemaphore)

#if defined(Q_OS_UNIX)
#  include <qfile.h>
#  include <private/qcore_unix_p.h>
#endif

QT_BEGIN_NAMESPACE

class QNativeIpcKeyPrivate
{
public:
    QString legacyKey_;

    static QString legacyKey(const QNativeIpcKey &key)
    {
        if (key.isSlowPath())
            return key.d->legacyKey_;
        return QString();
    }
    static void setLegacyKey(QNativeIpcKey &key, const QString &legacyKey)
    {
        QNativeIpcKeyPrivate::makeExtended(key)->legacyKey_ = legacyKey;
    }
    static void setNativeAndLegacyKeys(QNativeIpcKey &key, const QString &nativeKey,
                                       const QString &legacyKey)
    {
        key.setNativeKey(nativeKey);
        setLegacyKey(key, legacyKey);
    }

private:
    static QNativeIpcKeyPrivate *makeExtended(QNativeIpcKey &key)
    {
        if (!key.isSlowPath())
            key.d = new QNativeIpcKeyPrivate;
        return key.d;
    }
};

namespace QtIpcCommon {
enum class IpcType {
    SharedMemory,
    SystemSemaphore
};

constexpr bool isIpcSupported(IpcType ipcType, QNativeIpcKey::Type type)
{
    switch (type) {
    case QNativeIpcKey::Type::SystemV:
        break;

    case QNativeIpcKey::Type::PosixRealtime:
        if (ipcType == IpcType::SharedMemory)
            return QT_CONFIG(posix_shm);
        return QT_CONFIG(posix_sem);

    case QNativeIpcKey::Type::Windows:
#ifdef Q_OS_WIN
        return true;
#else
        return false;
#endif
    }

    if (ipcType == IpcType::SharedMemory)
        return QT_CONFIG(sysv_shm);
    return QT_CONFIG(sysv_sem);
}

template <auto Member1, auto... Members> class IpcStorageVariant
{
    template <typename T, typename C> static C extractClass(T C::*);
    template <typename T, typename C> static T extractObject(T C::*);

    template <auto M>
    static constexpr bool IsEnabled = decltype(extractObject(M))::Enabled;

    static_assert(std::is_member_object_pointer_v<decltype(Member1)>);
    using StorageType = decltype(extractClass(Member1));
    StorageType d;

public:
    template <typename Lambda> static auto
    visit_internal(StorageType &storage, QNativeIpcKey::Type keyType, const Lambda &lambda)
    {
        if constexpr ((IsEnabled<Member1> || ... || IsEnabled<Members>)) {
            if constexpr (IsEnabled<Member1>) {
                using MemberType1 = decltype(extractObject(Member1));
                if (MemberType1::supports(keyType))
                    return lambda(&(storage.*Member1));
            }
            if constexpr ((... || IsEnabled<Members>))
                return IpcStorageVariant<Members...>::visit_internal(storage, keyType, lambda);
            Q_UNREACHABLE();
        } else {
            // no backends enabled, but we can't return void
            return false;
        }
    }

    template <typename Lambda> auto visit(QNativeIpcKey::Type keyType, const Lambda &lambda)
    {
        return visit_internal(d, keyType, lambda);
    }

    template <typename Lambda> static auto
    staticVisit(QNativeIpcKey::Type keyType, const Lambda &lambda)
    {
        if constexpr ((IsEnabled<Member1> || ... || IsEnabled<Members>)) {
            if constexpr (IsEnabled<Member1>) {
                using MemberType1 = decltype(extractObject(Member1));
                if (MemberType1::supports(keyType))
                    return lambda(static_cast<MemberType1 *>(nullptr));
            }
            if constexpr ((... || IsEnabled<Members>))
                return IpcStorageVariant<Members...>::staticVisit(keyType, lambda);
            Q_UNREACHABLE();
        } else {
            // no backends enabled, but we can't return void
            return false;
        }
    }
};

QNativeIpcKey legacyPlatformSafeKey(const QString &key, IpcType ipcType, QNativeIpcKey::Type type);
QNativeIpcKey platformSafeKey(const QString &key, IpcType ipcType, QNativeIpcKey::Type type);

#ifdef Q_OS_UNIX
// Convenience function to create the file if needed
inline int createUnixKeyFile(const QByteArray &fileName)
{
    int fd = qt_safe_open(fileName.constData(), O_EXCL | O_CREAT | O_RDWR, 0640);
    if (fd < 0) {
        if (errno == EEXIST)
            return 0;
        return -1;
    } else {
        close(fd);
    }
    return 1;

}
#endif // Q_OS_UNIX
} // namespace QtIpcCommon

QT_END_NAMESPACE

#endif // QT_CONFIG(sharedmemory) || QT_CONFIG(systemsemaphore)


#endif // QTIPCCOMMON_P_H
