// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QSYSTEMSEMAPHORE_H
#define QSYSTEMSEMAPHORE_H

#include <QtCore/qcoreapplication.h>
#include <QtCore/qtipccommon.h>
#include <QtCore/qstring.h>
#include <QtCore/qscopedpointer.h>

#include <memory>

QT_BEGIN_NAMESPACE

#if QT_CONFIG(systemsemaphore)

class QSystemSemaphorePrivate;

class Q_CORE_EXPORT QSystemSemaphore
{
    Q_GADGET
    Q_DECLARE_TR_FUNCTIONS(QSystemSemaphore)
public:
    enum AccessMode
    {
        Open,
        Create
    };
    Q_ENUM(AccessMode)

    enum SystemSemaphoreError
    {
        NoError,
        PermissionDenied,
        KeyError,
        AlreadyExists,
        NotFound,
        OutOfResources,
        UnknownError
    };

    QSystemSemaphore(const QNativeIpcKey &key, int initialValue = 0, AccessMode = Open);
    ~QSystemSemaphore();

    void setNativeKey(const QNativeIpcKey &key, int initialValue = 0, AccessMode = Open);
    void setNativeKey(const QString &key, int initialValue = 0, AccessMode mode = Open,
                      QNativeIpcKey::Type type = QNativeIpcKey::legacyDefaultTypeForOs())
    { setNativeKey({ key, type }, initialValue, mode); }
    QNativeIpcKey nativeIpcKey() const;

    QSystemSemaphore(const QString &key, int initialValue = 0, AccessMode mode = Open);
    void setKey(const QString &key, int initialValue = 0, AccessMode mode = Open);
    QString key() const;

    bool acquire();
    bool release(int n = 1);

    SystemSemaphoreError error() const;
    QString errorString() const;

    static bool isKeyTypeSupported(QNativeIpcKey::Type type) Q_DECL_CONST_FUNCTION;
    static QNativeIpcKey platformSafeKey(const QString &key,
            QNativeIpcKey::Type type = QNativeIpcKey::DefaultTypeForOs);
    static QNativeIpcKey legacyNativeKey(const QString &key,
            QNativeIpcKey::Type type = QNativeIpcKey::legacyDefaultTypeForOs());

private:
    Q_DISABLE_COPY(QSystemSemaphore)
    std::unique_ptr<QSystemSemaphorePrivate> d;
};

#endif // QT_CONFIG(systemsemaphore)

QT_END_NAMESPACE

#endif // QSYSTEMSEMAPHORE_H
