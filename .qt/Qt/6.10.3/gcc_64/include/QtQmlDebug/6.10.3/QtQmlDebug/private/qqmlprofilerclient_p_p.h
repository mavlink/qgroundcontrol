// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLPROFILERCLIENT_P_P_H
#define QQMLPROFILERCLIENT_P_P_H

#include "qqmldebugclient_p_p.h"
#include "qqmldebugmessageclient_p.h"
#include "qqmlenginecontrolclient_p.h"
#include "qqmlprofilerclient_p.h"
#include "qqmlprofilertypedevent_p.h"
#include "qqmlprofilerclientdefinitions_p.h"

#include <QtCore/qqueue.h>
#include <QtCore/qstack.h>

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

class QQmlProfilerClientPrivate : public QQmlDebugClientPrivate {
    Q_DECLARE_PUBLIC(QQmlProfilerClient)
public:
    QQmlProfilerClientPrivate(QQmlDebugConnection *connection,
                              QQmlProfilerEventReceiver *eventReceiver)
        : QQmlDebugClientPrivate(QLatin1String("CanvasFrameRate"), connection)
        , eventReceiver(eventReceiver)
        , engineControl(new QQmlEngineControlClient(connection))
        , maximumTime(0)
        , recording(false)
        , requestedFeatures(0)
        , recordedFeatures(0)
        , flushInterval(0)
    {
    }

    ~QQmlProfilerClientPrivate() override;

    void sendRecordingStatus(int engineId);
    bool updateFeatures(ProfileFeature feature);
    int resolveType(const QQmlProfilerTypedEvent &type);
    int resolveStackTop();
    void forwardEvents(const QQmlProfilerEvent &last);
    void forwardDebugMessages(qint64 untilTimestamp);
    void processCurrentEvent();
    void finalize();

    QQmlProfilerEventReceiver *eventReceiver;
    QScopedPointer<QQmlEngineControlClient> engineControl;
    QScopedPointer<QQmlDebugMessageClient> messageClient;
    qint64 maximumTime;
    bool recording;
    quint64 requestedFeatures;
    quint64 recordedFeatures;
    quint32 flushInterval;

    // Reuse the same event, so that we don't have to constantly reallocate all the data.
    QQmlProfilerTypedEvent currentEvent;
    QHash<QQmlProfilerEventType, int> eventTypeIds;
    QHash<qint64, int> serverTypeIds;
    QStack<QQmlProfilerTypedEvent> rangesInProgress;
    QQueue<QQmlProfilerEvent> pendingMessages;
    QQueue<QQmlProfilerEvent> pendingDebugMessages;

    QList<int> trackedEngines;
};

QT_END_NAMESPACE

#endif // QQMLPROFILERCLIENT_P_P_H
