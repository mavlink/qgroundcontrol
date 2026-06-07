// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLPROPERTYCACHEMETODARGUMENTS_P_H
#define QQMLPROPERTYCACHEMETODARGUMENTS_P_H

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

#include <QtCore/qlist.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qtaggedpointer.h>
#include <QtCore/qmetatype.h>
#include <QtCore/private/qglobal_p.h>

QT_BEGIN_NAMESPACE

class QString;
class QQmlPropertyCacheMethodArguments
{
public:
    QQmlPropertyCacheMethodArguments *next;
    QList<QByteArray> *names;
    QMetaType types[1]; // First one is return type
};

QT_END_NAMESPACE

#endif // QQMLPROPERTYCACHEMETODARGUMENTS_P_H
