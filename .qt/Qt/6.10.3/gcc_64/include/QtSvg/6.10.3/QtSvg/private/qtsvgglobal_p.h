// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTSVGGLOBAL_P_H
#define QTSVGGLOBAL_P_H

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

#include "qtsvgglobal.h"
#include <QtSvg/qtsvgexports.h>

QT_BEGIN_NAMESPACE

namespace QtSvg {

enum class UnitTypes : quint32 {
    unknown,
    objectBoundingBox,
    userSpaceOnUse
};

enum class AnimatorType : quint32 {
    Controlled,
    Automatic,
};

}

QT_END_NAMESPACE

#endif // QTSVGGLOBAL_P_H
