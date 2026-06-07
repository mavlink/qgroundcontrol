// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QNATIVEINTERFACE_P_H
#define QNATIVEINTERFACE_P_H

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

#include <QtCore/private/qglobal_p.h>
#include <QtCore/qloggingcategory.h>

QT_BEGIN_NAMESPACE

QT_DECLARE_EXPORTED_QT_LOGGING_CATEGORY(lcNativeInterface, Q_CORE_EXPORT)

// Provides a definition for the interface destructor
#define QT_DEFINE_NATIVE_INTERFACE_2(Namespace, InterfaceClass)                                    \
    QT_PREPEND_NAMESPACE(Namespace)::InterfaceClass::~InterfaceClass() = default

#define QT_DEFINE_NATIVE_INTERFACE(...)                                                            \
    QT_OVERLOADED_MACRO(QT_DEFINE_NATIVE_INTERFACE, QNativeInterface, __VA_ARGS__)
#define QT_DEFINE_PRIVATE_NATIVE_INTERFACE(...)                                                    \
    QT_OVERLOADED_MACRO(QT_DEFINE_NATIVE_INTERFACE, QNativeInterface::Private, __VA_ARGS__)

#define QT_NATIVE_INTERFACE_RETURN_IF(NativeInterface, baseType)                                   \
    {                                                                                              \
        using QNativeInterface::Private::TypeInfo;                                                 \
        qCDebug(lcNativeInterface, "Comparing requested interface name %s with available %s",      \
                name, TypeInfo<NativeInterface>::name());                                          \
        if (qstrcmp(name, TypeInfo<NativeInterface>::name()) == 0) {                               \
            qCDebug(lcNativeInterface,                                                             \
                    "Match for interface %s. Comparing revisions (requested %d / available %d)",   \
                    name, revision, TypeInfo<NativeInterface>::revision());                        \
            if (revision == TypeInfo<NativeInterface>::revision()) {                               \
                qCDebug(lcNativeInterface, "Full match. Returning dynamic cast of %p", baseType);  \
                return dynamic_cast<NativeInterface *>(baseType);                                  \
            } else {                                                                               \
                qCWarning(lcNativeInterface,                                                       \
                          "Native interface revision mismatch (requested %d / available %d) for "  \
                          "interface %s",                                                          \
                          revision, TypeInfo<NativeInterface>::revision(), name);                  \
                return nullptr;                                                                    \
            }                                                                                      \
        } else {                                                                                   \
            qCDebug(lcNativeInterface, "No match for requested interface name %s", name);          \
        }                                                                                          \
    }

QT_END_NAMESPACE

#endif // QNATIVEINTERFACE_P_H
