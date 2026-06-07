// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

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

#ifndef QQMLPROPERTYUTILS_P_H
#define QQMLPROPERTYUTILS_P_H

#include <QtCore/qglobal.h>

QT_BEGIN_NAMESPACE

namespace QQml {
namespace PropertyUtils {
enum class State {
    ImplicitlySet,
    ExplicitlySet
};

inline bool isExplicitlySet(State propertyState)
{
    return propertyState == State::ExplicitlySet;
}
} // namespace PropertyUtils
} // namespace QQml

QT_END_NAMESPACE

#endif
