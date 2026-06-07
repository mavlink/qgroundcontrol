// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKITEMVIEWTRANSITION_P_P_H
#define QQUICKITEMVIEWTRANSITION_P_P_H

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

#include <QtQuick/private/qtquickglobal_p.h>

QT_REQUIRE_CONFIG(quick_viewtransitions);

#include <QtCore/qobject.h>
#include <QtCore/qpoint.h>
#include <QtQml/qqml.h>
#include <private/qqmlguard_p.h>
#include <private/qquicktransition_p.h>
#include <private/qanimationjobutil_p.h>

#include <QtCore/qpointer.h>

QT_BEGIN_NAMESPACE

class QQuickItem;
class QQuickTransition;
class QQuickItemViewTransitionableItem;
class QQuickItemViewTransitionJob;


class Q_QUICK_EXPORT QQuickItemViewTransitionChangeListener
{
public:
    QQuickItemViewTransitionChangeListener() {}
    virtual ~QQuickItemViewTransitionChangeListener() {}

    virtual void viewItemTransitionFinished(QQuickItemViewTransitionableItem *item) = 0;
};


class Q_QUICK_EXPORT QQuickItemViewTransitioner
{
public:
    enum TransitionType {
        NoTransition,
        PopulateTransition,
        AddTransition,
        MoveTransition,
        RemoveTransition
    };

    QQuickItemViewTransitioner();
    virtual ~QQuickItemViewTransitioner();

    bool canTransition(QQuickItemViewTransitioner::TransitionType type, bool asTarget) const;
    void transitionNextReposition(QQuickItemViewTransitionableItem *item, QQuickItemViewTransitioner::TransitionType type, bool isTarget);

    void addToTargetLists(QQuickItemViewTransitioner::TransitionType type, QQuickItemViewTransitionableItem *item, int index);
    void resetTargetLists();

    QQuickTransition *transitionObject(QQuickItemViewTransitioner::TransitionType type, bool asTarget) const;
    const QList<int> &targetIndexes(QQuickItemViewTransitioner::TransitionType type) const;
    const QList<QObject *> &targetItems(QQuickItemViewTransitioner::TransitionType type) const;

    inline void setPopulateTransitionEnabled(bool b) { usePopulateTransition = b; }
    inline bool populateTransitionEnabled() const { return usePopulateTransition; }

    inline void setChangeListener(QQuickItemViewTransitionChangeListener *obj) { changeListener = obj; }

    QSet<QQuickItemViewTransitionJob *> runningJobs;

    QList<int> addTransitionIndexes;
    QList<int> moveTransitionIndexes;
    QList<int> removeTransitionIndexes;
    QList<QObject *> addTransitionTargets;
    QList<QObject *> moveTransitionTargets;
    QList<QObject *> removeTransitionTargets;

    QQmlGuard<QQuickTransition> populateTransition;
    QQmlGuard<QQuickTransition> addTransition;
    QQmlGuard<QQuickTransition> addDisplacedTransition;
    QQmlGuard<QQuickTransition> moveTransition;
    QQmlGuard<QQuickTransition> moveDisplacedTransition;
    QQmlGuard<QQuickTransition> removeTransition;
    QQmlGuard<QQuickTransition> removeDisplacedTransition;
    QQmlGuard<QQuickTransition> displacedTransition;

private:
    friend class QQuickItemViewTransitionJob;

    QQuickItemViewTransitionChangeListener *changeListener;
    bool usePopulateTransition;

    void finishedTransition(QQuickItemViewTransitionJob *job, QQuickItemViewTransitionableItem *item);
};


/*
  An item that can be transitioned using QQuickViewTransitionJob.
  */
class Q_QUICK_EXPORT QQuickItemViewTransitionableItem
{
public:
    QQuickItemViewTransitionableItem(QQuickItem *i);
    virtual ~QQuickItemViewTransitionableItem();

    qreal itemX() const;
    qreal itemY() const;

    void moveTo(const QPointF &pos, bool immediate = false);

    bool transitionScheduledOrRunning() const;
    bool transitionRunning() const;
    bool isPendingRemoval() const;

    bool prepareTransition(QQuickItemViewTransitioner *transitioner, int index, const QRectF &viewBounds);
    void startTransition(QQuickItemViewTransitioner *transitioner, int index);
    void completeTransition(QQuickTransition *quickTransition);

    SelfDeletable m_selfDeletable;
    QPointF nextTransitionTo;
    QPointF lastMovedTo;
    QPointF nextTransitionFrom;
    QQuickItem *item;
    QQuickItemViewTransitionJob *transition;
    QQuickItemViewTransitioner::TransitionType nextTransitionType;
    bool isTransitionTarget : 1;
    bool nextTransitionToSet : 1;
    bool nextTransitionFromSet : 1;
    bool lastMovedToSet : 1;
    bool prepared : 1;

private:
    friend class QQuickItemViewTransitioner;
    friend class QQuickItemViewTransitionJob;
    void setNextTransition(QQuickItemViewTransitioner::TransitionType, bool isTargetItem);
    bool transitionWillChangePosition() const;
    void finishedTransition();
    void resetNextTransitionPos();
    void clearCurrentScheduledTransition();
    void stopTransition();
};


class QQuickViewTransitionAttached : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int index READ index NOTIFY indexChanged FINAL)
    Q_PROPERTY(QQuickItem* item READ item NOTIFY itemChanged FINAL)
    Q_PROPERTY(QPointF destination READ destination NOTIFY destinationChanged FINAL)

    Q_PROPERTY(QList<int> targetIndexes READ targetIndexes NOTIFY targetIndexesChanged FINAL)
    Q_PROPERTY(QQmlListProperty<QObject> targetItems READ targetItems NOTIFY targetItemsChanged FINAL)

    QML_NAMED_ELEMENT(ViewTransition)
    QML_ADDED_IN_VERSION(2, 0)
    QML_UNCREATABLE("ViewTransition is only available via attached properties.")
    QML_ATTACHED(QQuickViewTransitionAttached)

public:
    QQuickViewTransitionAttached(QObject *parent);

    int index() const { return m_index; }
    QQuickItem *item() const { return m_item; }
    QPointF destination() const { return m_destination; }

    QList<int> targetIndexes() const { return m_targetIndexes; }
    QQmlListProperty<QObject> targetItems();

    static QQuickViewTransitionAttached *qmlAttachedProperties(QObject *);

Q_SIGNALS:
    void indexChanged();
    void itemChanged();
    void destinationChanged();

    void targetIndexesChanged();
    void targetItemsChanged();

private:
    friend class QQuickItemViewTransitionJob;
    QPointF m_destination;
    QList<int> m_targetIndexes;
    QList<QObject *> m_targetItems;

    QPointer<QQuickItem> m_item;
    int m_index;
};

QT_END_NAMESPACE

#endif // QQUICKITEMVIEWTRANSITION_P_P_H
