// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTQUICKGLOBAL_P_H
#define QTQUICKGLOBAL_P_H

#include <QtQml/private/qtqmlglobal_p.h>
#include <QtGui/private/qtguiglobal_p.h>
#include <QtQuick/private/qtquick-config_p.h>

#include <QtCore/qloggingcategory.h>

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

#include "qtquickglobal.h"
#include <QtQuick/qtquickexports.h>

QT_BEGIN_NAMESPACE

void Q_QUICK_EXPORT qml_register_types_QtQuick();

void Q_QUICK_EXPORT QQuick_initializeModule();

Q_DECLARE_LOGGING_CATEGORY(lcTouch)
Q_DECLARE_LOGGING_CATEGORY(lcMouse)
Q_DECLARE_LOGGING_CATEGORY(lcFocus)

/*
    This is needed for QuickTestUtils. Q_AUTOTEST_EXPORT checks QT_BUILDING_QT
    (amongst others) to see if it should export symbols. Until QuickTestUtils
    was introduced, this was enough, as there weren't any intermediate test
    helper libraries that used a Qt library and were in turn used by tests.

    Taking QQuickItemViewPrivate as an example: previously it was using
    Q_AUTOTEST_EXPORT. Since QuickTestUtils is a Qt library (albeit a private
    one), QT_BUILDING_QT was true and so Q_AUTOTEST_EXPORT evaluated to an
    export. However, QQuickItemViewPrivate was already exported by the Quick
    library, so we would get errors like this:

    Qt6Quickd.lib(Qt6Quickd.dll) : error LNK2005: "public: static class
    QQuickItemViewPrivate * __cdecl QQuickItemViewPrivate::get(class QQuickItemView *)"
    (?get@QQuickItemViewPrivate@@SAPEAV1@PEAVQQuickItemView@@@Z) already defined
    in Qt6QuickTestUtilsd.lib(viewtestutils.cpp.obj)

    So, to account for the special case of QuickTestUtils, we need to be more
    specific about which part of Qt we're building; instead of checking if we're
    building any Qt library at all, check if we're building the Quick library,
    and only then export.
*/
#if defined(QT_BUILD_INTERNAL) && defined(QT_BUILD_QUICK_LIB) && defined(QT_SHARED)
#    define Q_QUICK_AUTOTEST_EXPORT Q_DECL_EXPORT
#elif defined(QT_BUILD_INTERNAL) && defined(QT_SHARED)
#    define Q_QUICK_AUTOTEST_EXPORT Q_DECL_IMPORT
#else
#    define Q_QUICK_AUTOTEST_EXPORT
#endif

QT_END_NAMESPACE

#endif // QTQUICKGLOBAL_P_H
