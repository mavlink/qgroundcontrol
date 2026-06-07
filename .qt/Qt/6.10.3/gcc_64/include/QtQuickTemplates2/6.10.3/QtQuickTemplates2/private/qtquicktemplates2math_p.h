// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QTQUICKTEMPLATES2MATH_P_H
#define QTQUICKTEMPLATES2MATH_P_H

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

#include <cmath>

QT_BEGIN_NAMESPACE

template<typename ...Real>
static bool areRepresentableAsInteger(Real... numbers) {
    auto check = [](qreal number) -> bool { return std::nearbyint(number) == number; };
    return (... && check(numbers));
}

QT_END_NAMESPACE

#endif // QTQUICKTEMPLATES2MATH_P_H
