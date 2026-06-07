// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QMLLSMAIN_P_H
#define QMLLSMAIN_P_H

#include <QtCore/qtconfigmacros.h>

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

QT_BEGIN_NAMESPACE

namespace QmlLsp {
int qmllsMain(int argv, char *argc[]);
} // namespace QmlLsp

QT_END_NAMESPACE
#endif // QMLLSMAIN_P_H
