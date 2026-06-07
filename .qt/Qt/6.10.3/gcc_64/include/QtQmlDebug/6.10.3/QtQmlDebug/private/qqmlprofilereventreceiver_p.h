// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLPROFILEREVENTRECEIVER_P_H
#define QQMLPROFILEREVENTRECEIVER_P_H

#include "qqmlprofilerevent_p.h"
#include "qqmlprofilereventtype_p.h"

#include <QtCore/qobject.h>

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

QT_BEGIN_NAMESPACE

class QQmlProfilerEventReceiver : public QObject
{
    Q_OBJECT
public:
    explicit QQmlProfilerEventReceiver(QObject *parent = nullptr) : QObject(parent) {}
    ~QQmlProfilerEventReceiver() override;

    virtual int numLoadedEventTypes() const = 0;
    virtual void addEventType(const QQmlProfilerEventType &type) = 0;
    virtual void addEvent(const QQmlProfilerEvent &event) = 0;
};

QT_END_NAMESPACE

#endif // QQMLPROFILEREVENTRECEIVER_P_H
