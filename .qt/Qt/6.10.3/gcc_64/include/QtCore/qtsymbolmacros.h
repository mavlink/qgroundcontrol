// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTSYMBOLMACROS_H
#define QTSYMBOLMACROS_H

#if 0
#  pragma qt_sync_stop_processing
#endif

// For GHS symbol keeping.
#include <QtCore/qcompilerdetection.h>
#include <QtCore/qtpreprocessorsupport.h>

// For handling namespaced resources.
#ifdef QT_NAMESPACE
#  define QT_RCC_MANGLE_NAMESPACE0(x) x
#  define QT_RCC_MANGLE_NAMESPACE1(a, b) a##_##b
#  define QT_RCC_MANGLE_NAMESPACE2(a, b) QT_RCC_MANGLE_NAMESPACE1(a,b)
#  define QT_RCC_MANGLE_NAMESPACE(name) QT_RCC_MANGLE_NAMESPACE2( \
        QT_RCC_MANGLE_NAMESPACE0(name), QT_RCC_MANGLE_NAMESPACE0(QT_NAMESPACE))
#else
#   define QT_RCC_MANGLE_NAMESPACE(name) name
#endif

// GHS needs special handling to keep a symbol around.
#if defined(Q_CC_GHS)
#  define Q_GHS_KEEP_REFERENCE(S) QT_DO_PRAGMA(ghs reference S ##__Fv)
#else
#  define Q_GHS_KEEP_REFERENCE(S)
#endif

// Macros to ensure a symbol is not dropped by the linker even if it's not used.
#define QT_DECLARE_EXTERN_SYMBOL(NAME, RETURN_TYPE) \
    extern RETURN_TYPE NAME(); \
    Q_GHS_KEEP_REFERENCE(NAME)

#define QT_DECLARE_EXTERN_SYMBOL_INT(NAME) \
    QT_DECLARE_EXTERN_SYMBOL(NAME, int)

#define QT_DECLARE_EXTERN_SYMBOL_VOID(NAME) \
    QT_DECLARE_EXTERN_SYMBOL(NAME, void)

#define QT_KEEP_SYMBOL_VAR_NAME(NAME) NAME ## _keep

#define QT_KEEP_SYMBOL_HELPER(NAME, VAR_NAME) \
    volatile auto VAR_NAME = &NAME; \
    Q_UNUSED(VAR_NAME)

#define QT_KEEP_SYMBOL(NAME) \
    QT_KEEP_SYMBOL_HELPER(NAME, QT_KEEP_SYMBOL_VAR_NAME(NAME))


// Similar to the ones above, but for rcc resource symbols specifically.
#define QT_GET_RESOURCE_INIT_SYMBOL(NAME) \
    QT_RCC_MANGLE_NAMESPACE(qInitResources_ ## NAME)

#define QT_DECLARE_EXTERN_RESOURCE(NAME) \
    QT_DECLARE_EXTERN_SYMBOL_INT(QT_GET_RESOURCE_INIT_SYMBOL(NAME))

#define QT_KEEP_RESOURCE(NAME) \
    QT_KEEP_SYMBOL(QT_GET_RESOURCE_INIT_SYMBOL(NAME))

#endif // QTSYMBOLMACROS_H

