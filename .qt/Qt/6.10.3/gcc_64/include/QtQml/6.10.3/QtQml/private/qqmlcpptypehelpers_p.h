// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLCPPTYPEHELPERS_P_H
#define QQMLCPPTYPEHELPERS_P_H

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

#include <type_traits>

/*! \internal
    Used by Qmltc to decide when value types should be passed by value or reference.
 */
template<typename T>
using passByConstRefOrValue =
        std::conditional_t<((sizeof(T) > 3 * sizeof(void *)) || !(std::is_trivially_copyable_v<T> && std::is_trivially_default_constructible_v<T>)),
                           const T &,
                           T>;

#endif // QQMLCPPTYPEHELPERS_P_H
