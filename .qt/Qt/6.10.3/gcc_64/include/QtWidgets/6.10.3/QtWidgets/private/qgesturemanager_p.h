// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QGESTUREMANAGER_P_H
#define QGESTUREMANAGER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include "qobject.h"
#include "qbasictimer.h"
#include "private/qwidget_p.h"
#include "qgesturerecognizer.h"

#include <QtCore/qpointer.h>

#ifndef QT_NO_GESTURES

#include <functional>

QT_BEGIN_NAMESPACE

class QBasicTimer;
class QGraphicsObject;
class QGestureManager : public QObject
{
    Q_OBJECT
public:
    QGestureManager(QObject *parent);
    ~QGestureManager();

    Qt::GestureType registerGestureRecognizer(QGestureRecognizer *recognizer);
    void unregisterGestureRecognizer(Qt::GestureType type);

    bool filterEvent(QWidget *receiver, QEvent *event);
    bool filterEvent(QObject *receiver, QEvent *event);
#if QT_CONFIG(graphicsview)
    bool filterEvent(QGraphicsObject *receiver, QEvent *event);
#endif // QT_CONFIG(graphicsview)

    enum InstanceCreation { ForceCreation, DontForceCreation };

    static QGestureManager *instance(InstanceCreation ic = ForceCreation); // implemented in qapplication.cpp
    static bool gesturePending(QObject *o);

    void cleanupCachedGestures(QObject *target, Qt::GestureType type);

    void recycle(QGesture *gesture);

protected:
    bool filterEventThroughContexts(const QMultiMap<QObject *, Qt::GestureType> &contexts,
                                    QEvent *event);

private:
    QMultiMap<Qt::GestureType, QGestureRecognizer *> m_recognizers;

    QSet<QGesture *> m_activeGestures;
    QSet<QGesture *> m_maybeGestures;

    struct ObjectGesture
    {
        QObject* object;
        Qt::GestureType gesture;

        ObjectGesture(QObject *o, const Qt::GestureType &g) : object(o), gesture(g) { }
        inline bool operator<(const ObjectGesture &rhs) const
        {
            if (std::less<QObject *>{}(object, rhs.object))
                return true;
            if (object == rhs.object)
                return gesture < rhs.gesture;
            return false;
        }
    };

    QMap<ObjectGesture, QList<QGesture *> > m_objectGestures;
    QHash<QGesture *, QGestureRecognizer *> m_gestureToRecognizer;
    QHash<QGesture *, QObject *> m_gestureOwners;

    QHash<QGesture *, QPointer<QWidget> > m_gestureTargets;

    int m_lastCustomGestureId;

    QHash<QGestureRecognizer *, QSet<QGesture *> > m_obsoleteGestures;
    QHash<QGesture *, QGestureRecognizer *> m_deletedRecognizers;
    QSet<QGesture *> m_gesturesToDelete;
    void cleanupGesturesForRemovedRecognizer(QGesture *gesture);

    QGesture *getState(QObject *widget, QGestureRecognizer *recognizer,
                       Qt::GestureType gesture);
    void deliverEvents(const QSet<QGesture *> &gestures,
                       QSet<QGesture *> *undeliveredGestures);
    void getGestureTargets(const QSet<QGesture*> &gestures,
                           QHash<QWidget *, QList<QGesture *> > *conflicts,
                           QHash<QWidget *, QList<QGesture *> > *normal);

    void cancelGesturesForChildren(QGesture *originatingGesture);
};

QT_END_NAMESPACE

#endif // QT_NO_GESTURES

#endif // QGESTUREMANAGER_P_H
