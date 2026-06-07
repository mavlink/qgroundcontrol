// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKTEMPLATESUTILS_P_H
#define QQUICKTEMPLATESUTILS_P_H

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

#include <QtQuick/qquickitem.h>

QT_BEGIN_NAMESPACE

namespace QQuickTemplatesUtils {
bool isInteractiveControlType(const QQuickItem *item);
}

QT_END_NAMESPACE

#endif // QQUICKTEMPLATESUTILS_P_H
