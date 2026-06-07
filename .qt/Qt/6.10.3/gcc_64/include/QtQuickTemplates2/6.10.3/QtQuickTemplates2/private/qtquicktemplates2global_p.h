// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QTQUICKTEMPLATES2GLOBAL_P_H
#define QTQUICKTEMPLATES2GLOBAL_P_H

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

#include <QtCore/qglobal.h>
#include <QtQml/private/qqmlglobal_p.h>
#include <QtQuickTemplates2/private/qtquicktemplates2-config_p.h>
#include <QtQuickTemplates2/qtquicktemplates2exports.h>

QT_BEGIN_NAMESPACE

Q_QUICKTEMPLATES2_EXPORT void QQuickTemplates_initializeModule();
Q_QUICKTEMPLATES2_EXPORT void qml_register_types_QtQuick_Templates();

[[maybe_unused]] static inline QString backgroundName() { return QStringLiteral("background"); }
[[maybe_unused]] static inline QString handleName() { return QStringLiteral("handle"); }
[[maybe_unused]] static inline QString indicatorName() { return QStringLiteral("indicator"); }

QT_END_NAMESPACE

#endif // QTQUICKTEMPLATES2GLOBAL_P_H
