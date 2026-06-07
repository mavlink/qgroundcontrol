// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTQUICKVECTORIMAGEGLOBAL_P_H
#define QTQUICKVECTORIMAGEGLOBAL_P_H

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
#include <QtQuickVectorImageGenerator/qtquickvectorimagegeneratorexports.h>

QT_BEGIN_NAMESPACE

namespace QQuickVectorImageGenerator
{
    enum PathSelector {
        FillPath = 0x1,
        StrokePath = 0x2,
        FillAndStroke = 0x3
    };

    enum GeneratorFlag {
        OptimizePaths = 0x01,
        CurveRenderer = 0x02,
        OutlineStrokeMode = 0x04,
        AssumeTrustedSource = 0x08,
    };

    Q_DECLARE_FLAGS(GeneratorFlags, GeneratorFlag);
    Q_DECLARE_OPERATORS_FOR_FLAGS(GeneratorFlags);
}

QT_END_NAMESPACE

#endif //QTQUICKVECTORIMAGEGLOBAL_P_H
