// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLMODULEREGISTRATION_H
#define QQMLMODULEREGISTRATION_H

#include <QtQml/qtqmlglobal.h>

QT_BEGIN_NAMESPACE

struct QQmlModuleRegistrationPrivate;
class Q_QML_EXPORT QQmlModuleRegistration
{
    Q_DISABLE_COPY_MOVE(QQmlModuleRegistration)
public:
    QQmlModuleRegistration(const char *uri, void (*registerFunction)());
    ~QQmlModuleRegistration();

#if QT_DEPRECATED_SINCE(6, 0)
    QT_DEPRECATED_X("Use registration without major version")
    QQmlModuleRegistration(const char *uri, int majorVersion, void (*registerFunction)());
#endif

private:
    QQmlModuleRegistrationPrivate *d = nullptr;
};

QT_END_NAMESPACE

#endif // QQMLMODULEREGISTRATION_H
