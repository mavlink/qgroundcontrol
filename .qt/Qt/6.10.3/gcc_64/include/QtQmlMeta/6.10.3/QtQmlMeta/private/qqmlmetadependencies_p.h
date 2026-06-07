// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLMETADEPENDENCIES_P_H
#define QQMLMETADEPENDENCIES_P_H

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

#include <QtQmlMeta/qtqmlmetaexports.h>

QT_BEGIN_NAMESPACE

struct QQmlMetaDependencies
{
    // Export the method so that the linker cannot remove it.
    static Q_QMLMETA_EXPORT bool collect();
};

QT_END_NAMESPACE

#endif // QQMLMETADEPENDENCIES_P_H
