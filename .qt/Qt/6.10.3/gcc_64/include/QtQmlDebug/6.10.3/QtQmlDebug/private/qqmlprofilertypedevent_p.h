// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLPROFILERTYPEDEVENT_P_H
#define QQMLPROFILERTYPEDEVENT_P_H

#include "qqmlprofilerevent_p.h"
#include "qqmlprofilereventtype_p.h"

#include <QtCore/qdatastream.h>

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

struct QQmlProfilerTypedEvent
{
    QQmlProfilerEvent event;
    QQmlProfilerEventType type;
    qint64 serverTypeId = 0;
};

QDataStream &operator>>(QDataStream &stream, QQmlProfilerTypedEvent &event);

Q_DECLARE_TYPEINFO(QQmlProfilerTypedEvent, Q_RELOCATABLE_TYPE);

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QQmlProfilerTypedEvent)

#endif // QQMLPROFILERTYPEDEVENT_P_H
