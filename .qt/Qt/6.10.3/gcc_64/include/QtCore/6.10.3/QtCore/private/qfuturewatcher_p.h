// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QFUTUREWATCHER_P_H
#define QFUTUREWATCHER_P_H

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

#include <QtCore/qfuturewatcher.h>

#include "qfutureinterface_p.h"
#include <qlist.h>

#include <private/qobject_p.h>

QT_REQUIRE_CONFIG(future);

QT_BEGIN_NAMESPACE

class QFutureWatcherBasePrivate : public QObjectPrivate,
                                  public QFutureCallOutInterface
{
    Q_DECLARE_PUBLIC(QFutureWatcherBase)

public:
    QFutureWatcherBasePrivate();

    void postCallOutEvent(const QFutureCallOutEvent &callOutEvent) override;
    void callOutInterfaceDisconnected() override;

    void sendCallOutEvent(QFutureCallOutEvent *event);

    QAtomicInt pendingResultsReady;
    int maximumPendingResultsReady;

    QAtomicInt resultAtConnected;
};

QT_END_NAMESPACE

#endif
