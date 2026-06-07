// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:insignificant

#ifndef QTQMLGLOBAL_P_H
#define QTQMLGLOBAL_P_H

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
#include <QtQml/qtqmlglobal.h>
#include <QtQml/private/qtqml-config_p.h>
#include <QtQml/qtqmlexports.h>

#define Q_QML_AUTOTEST_EXPORT Q_AUTOTEST_EXPORT

#ifdef QT_NO_DEBUG
#define QML_NEARLY_ALWAYS_INLINE Q_ALWAYS_INLINE
#else
#define QML_NEARLY_ALWAYS_INLINE inline
#endif

#endif // QTQMLGLOBAL_P_H
