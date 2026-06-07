// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKITEMVIEW_P_P_H
#define QQUICKITEMVIEW_P_P_H

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

QT_REQUIRE_CONFIG(quick_itemview);

#include "qquickitemview_p.h"
#include "qquickitemviewfxitem_p_p.h"
#if QT_CONFIG(quick_viewtransitions)
#include "qquickitemviewtransition_p.h"
#endif
#include "qquickflickable_p_p.h"
#include <QtQmlModels/private/qqmlobjectmodel_p.h>
#include <QtQmlModels/private/qqmldelegatemodel_p.h>
#include <QtQmlModels/private/qqmlchangeset_p.h>

#include <QtCore/qpointer.h>

QT_BEGIN_NAMESPACE

class Q_AUTOTEST_EXPORT FxViewItem : public QQuickItemViewFxItem
{
public:
    FxViewItem(QQuickItem *, QQuickItemView *, bool own, QQuickItemViewAttached *attached);

    QQuickItemView *view;
    QQuickItemViewAttached *attached;
};


class Q_QUICK_EXPORT QQuickItemViewChangeSet
{
public:
    QQuickItemViewChangeSet();

    bool hasPendingChanges() const;
    void prepare(int currentIndex, int count);
    void reset();

    void applyChanges(const QQmlChangeSet &changeSet);

    void applyBufferedChanges(const QQuickItemViewChangeSet &other);

    int itemCount;
    int newCurrentIndex;
    QQmlChangeSet pendingChanges;
    QMultiHash<QQmlChangeSet::MoveKey, FxViewItem *> removedItems;

    bool active : 1;
    bool currentChanged : 1;
    bool currentRemoved : 1;
};


class Q_QUICK_AUTOTEST_EXPORT QQuickItemViewPrivate
        : public QQuickFlickablePrivate
#if QT_CONFIG(quick_viewtransitions)
        , public QQuickItemViewTransitionChangeListener
#endif
        , public QAnimationJobChangeListener
{
public:
    Q_DECLARE_PUBLIC(QQuickItemView)
    QQuickItemViewPrivate();
    ~QQuickItemViewPrivate();

    static inline QQuickItemViewPrivate *get(QQuickItemView *o) { return o->d_func(); }

    struct ChangeResult {
        QQmlNullableValue<qreal> visiblePos;
        bool changedFirstItem;
        qreal sizeChangesBeforeVisiblePos;
        qreal sizeChangesAfterVisiblePos;
        int countChangeBeforeVisible;
        int countChangeAfterVisibleItems;

        ChangeResult()
            : visiblePos(0), changedFirstItem(false),
              sizeChangesBeforeVisiblePos(0), sizeChangesAfterVisiblePos(0),
              countChangeBeforeVisible(0), countChangeAfterVisibleItems(0) {}

        ChangeResult(const QQmlNullableValue<qreal> &p)
            : visiblePos(p), changedFirstItem(false),
              sizeChangesBeforeVisiblePos(0), sizeChangesAfterVisiblePos(0),
              countChangeBeforeVisible(0), countChangeAfterVisibleItems(0) {}

        ChangeResult &operator+=(const ChangeResult &other) {
            if (&other == this)
                return *this;
            changedFirstItem &= other.changedFirstItem;
            sizeChangesBeforeVisiblePos += other.sizeChangesBeforeVisiblePos;
            sizeChangesAfterVisiblePos += other.sizeChangesAfterVisiblePos;
            countChangeBeforeVisible += other.countChangeBeforeVisible;
            countChangeAfterVisibleItems += other.countChangeAfterVisibleItems;
            return *this;
        }

        void reset() {
            changedFirstItem = false;
            sizeChangesBeforeVisiblePos = 0.0;
            sizeChangesAfterVisiblePos = 0.0;
            countChangeBeforeVisible = 0;
            countChangeAfterVisibleItems = 0;
        }
    };

    enum BufferMode { NoBuffer = 0x00, BufferBefore = 0x01, BufferAfter = 0x02 };

    bool isValid() const;
    qreal position() const;
    qreal size() const;
    qreal startPosition() const;
    qreal endPosition() const;
    qreal contentStartOffset() const;
    int findLastVisibleIndex(int defaultValue = -1) const;
    FxViewItem *visibleItem(int modelIndex) const;
    FxViewItem *firstItemInView() const;
    int findLastIndexInView() const;
    int mapFromModel(int modelIndex) const;

    virtual void init();
    virtual void clear(bool onDestruction=false);
    virtual void updateViewport();

    void regenerate(bool orientationChanged=false);
    void layout();
    void animationFinished(QAbstractAnimationJob *) override;
    void refill();
    void refill(qreal from, qreal to);
    void mirrorChange() override;

    FxViewItem *createItem(int modelIndex,QQmlIncubator::IncubationMode incubationMode = QQmlIncubator::AsynchronousIfNested);
    bool releaseCurrentItem(QQmlInstanceModel::ReusableFlag reusableFlag)
    {
        auto oldCurrentItem = std::exchange(currentItem, nullptr);
        return releaseItem(oldCurrentItem, reusableFlag);
    }
    virtual bool releaseItem(FxViewItem *item, QQmlInstanceModel::ReusableFlag reusableFlag);
    void itemDestroyed(QQuickItem *item) override;

    QQuickItem *createHighlightItem();
    QQuickItem *createComponentItem(QQmlComponent *component, qreal zValue, bool createDefault = false) const;
    virtual void initializeComponentItem(QQuickItem *) const;

    void updateCurrent(int modelIndex);
    void updateTrackedItem();
    void updateUnrequestedIndexes();
    void updateUnrequestedPositions();
    void updateVisibleIndex();
    void positionViewAtIndex(int index, int mode);

    qreal minExtentForAxis(const AxisData &axisData, bool forXAxis) const;
    qreal maxExtentForAxis(const AxisData &axisData, bool forXAxis) const;
    qreal calculatedMinExtent() const;
    qreal calculatedMaxExtent() const;

    void connectModel(QQuickItemView *q, QQmlDelegateModelPointer *model);
    void disconnectModel(QQuickItemView *q, QQmlDelegateModelPointer *model);

    void applyDelegateChange();
    void applyDelegateModelAccessChange()
    {
        QQmlDelegateModel::applyDelegateModelAccessChangeOnView(q_func(), this);
    }

    void applyPendingChanges();
    bool applyModelChanges(ChangeResult *insertionResult, ChangeResult *removalResult);
    bool applyRemovalChange(const QQmlChangeSet::Change &removal, ChangeResult *changeResult, int *removedCount);
    void removeItem(FxViewItem *item, const QQmlChangeSet::Change &removal, ChangeResult *removeResult);
    virtual void updateSizeChangesBeforeVisiblePos(FxViewItem *item, ChangeResult *removeResult);
    void repositionFirstItem(FxViewItem *prevVisibleItemsFirst, qreal prevVisibleItemsFirstPos,
            FxViewItem *prevFirstVisible, ChangeResult *insertionResult, ChangeResult *removalResult);

#if QT_CONFIG(quick_viewtransitions)
    void createTransitioner();
    void prepareVisibleItemTransitions();
    void prepareRemoveTransitions(QMultiHash<QQmlChangeSet::MoveKey, FxViewItem *> *removedItems);
    bool prepareNonVisibleItemTransition(FxViewItem *item, const QRectF &viewBounds);
    void viewItemTransitionFinished(QQuickItemViewTransitionableItem *item) override;
#endif

    int findMoveKeyIndex(QQmlChangeSet::MoveKey key, const QVector<QQmlChangeSet::Change> &changes) const;

    void checkVisible() const;
    void showVisibleItems() const;

    void markExtentsDirty() {
        if (layoutOrientation() == Qt::Vertical)
            vData.markExtentsDirty();
        else
            hData.markExtentsDirty();
    }

    bool hasPendingChanges() const {
        return currentChanges.hasPendingChanges()
                || bufferedChanges.hasPendingChanges()
#if QT_CONFIG(quick_viewtransitions)
                ||runDelayedRemoveTransition
#endif
                ;
    }

    void refillOrLayout() {
        if (hasPendingChanges())
            layout();
        else
            refill();
    }

    void forceLayoutPolish() {
        Q_Q(QQuickItemView);
        forceLayout = true;
        q->polish();
    }

    void releaseVisibleItems(QQmlInstanceModel::ReusableFlag reusableFlag) {
        // make a copy and clear the visibleItems first to avoid destroyed
        // items being accessed during the loop (QTBUG-61294)
        const QList<FxViewItem *> oldVisible = visibleItems;
        visibleItems.clear();
        for (FxViewItem *item : oldVisible)
            releaseItem(item, reusableFlag);
    }

    void emitCountChanged();

    virtual QQuickItemViewAttached *getAttachedObject(const QObject *) const { return nullptr; }

    QPointer<QQmlInstanceModel> model;
    int itemCount;
    int buffer;
    int bufferMode;
    int displayMarginBeginning;
    int displayMarginEnd;
    Qt::LayoutDirection layoutDirection;
    QQuickItemView::VerticalLayoutDirection verticalLayoutDirection;

    QList<FxViewItem *> visibleItems;
    qreal firstVisibleItemPosition = 0;
    void storeFirstVisibleItemPosition() {
        if (!visibleItems.isEmpty()) {
            firstVisibleItemPosition = visibleItems.constFirst()->position();
        }
    }
    int visibleIndex;
    int currentIndex;
    FxViewItem *currentItem;
    FxViewItem *trackedItem;
    QHash<QQuickItem*,int> unrequestedItems;
    int requestedIndex;
    QQuickItemViewChangeSet currentChanges;
    QQuickItemViewChangeSet bufferedChanges;
    QPauseAnimationJob bufferPause;

    QQmlComponent *highlightComponent;
    std::unique_ptr<FxViewItem> highlight;
    int highlightRange;     // enum value
    qreal highlightRangeStart;
    qreal highlightRangeEnd;
    int highlightMoveDuration;

    QQmlComponent *headerComponent;
    FxViewItem *header;
    QQmlComponent *footerComponent;
    FxViewItem *footer;

    // Reusing delegate items cannot be on by default for backwards compatibility.
    // Reusing an item will e.g mean that Component.onCompleted will only be called for an
    // item when it's created and not when it's reused, which will break legacy applications.
    QQmlInstanceModel::ReusableFlag reusableFlag = QQmlInstanceModel::NotReusable;

    struct MovedItem {
        FxViewItem *item;
        QQmlChangeSet::MoveKey moveKey;
        MovedItem(FxViewItem *i, QQmlChangeSet::MoveKey k)
            : item(i), moveKey(k) {}
    };
#if QT_CONFIG(quick_viewtransitions)
    QQuickItemViewTransitioner *transitioner;
    QVector<FxViewItem *> releasePendingTransition;
#endif

    mutable qreal minExtent;
    mutable qreal maxExtent;

    bool ownModel : 1;
    bool wrap : 1;
    bool keyNavigationEnabled : 1;
    bool explicitKeyNavigationEnabled : 1;
    bool inLayout : 1;
    bool inViewportMoved : 1;
    bool forceLayout : 1;
    bool currentIndexCleared : 1;
    bool haveHighlightRange : 1;
    bool autoHighlight : 1;
    bool highlightRangeStartValid : 1;
    bool highlightRangeEndValid : 1;
    bool fillCacheBuffer : 1;
    bool inRequest : 1;
#if QT_CONFIG(quick_viewtransitions)
    bool runDelayedRemoveTransition : 1;
#endif
    bool delegateValidated : 1;
    bool isClearing : 1;
    bool explicitDelegate: 1;
    bool explicitDelegateModelAccess: 1;
    bool inRefill : 1;

protected:
    virtual Qt::Orientation layoutOrientation() const = 0;
    virtual bool isContentFlowReversed() const = 0;

    virtual qreal positionAt(int index) const = 0;
    virtual qreal endPositionAt(int index) const = 0;
    virtual qreal originPosition() const = 0;
    virtual qreal lastPosition() const = 0;

    virtual qreal headerSize() const = 0;
    virtual qreal footerSize() const = 0;
    virtual bool showHeaderForIndex(int index) const = 0;
    virtual bool showFooterForIndex(int index) const = 0;
    virtual void updateHeader() = 0;
    virtual void updateFooter() = 0;
    virtual bool hasStickyHeader() const { return false; }
    virtual bool hasStickyFooter() const { return false; }

    virtual void createHighlight(bool onDestruction = false) = 0;
    virtual void updateHighlight() = 0;
    virtual void resetHighlightPosition() = 0;
    virtual bool movingFromHighlight() { return false; }

    virtual void setPosition(qreal pos) = 0;
    virtual void fixupPosition() = 0;

    virtual bool addVisibleItems(qreal fillFrom, qreal fillTo, qreal bufferFrom, qreal bufferTo, bool doBuffer) = 0;
    virtual bool removeNonVisibleItems(qreal bufferFrom, qreal bufferTo) = 0;
    virtual void visibleItemsChanged() {}

    virtual FxViewItem *newViewItem(int index, QQuickItem *item) = 0;
    virtual void repositionItemAt(FxViewItem *item, int index, qreal sizeBuffer) = 0;
    virtual void repositionPackageItemAt(QQuickItem *item, int index) = 0;
    virtual void resetFirstItemPosition(qreal pos = 0.0) = 0;
    virtual void adjustFirstItem(qreal forwards, qreal backwards, int changeBeforeVisible) = 0;

    virtual void layoutVisibleItems(int fromModelIndex = 0) = 0;
    virtual void changedVisibleIndex(int newIndex) = 0;

    virtual bool applyInsertionChange(const QQmlChangeSet::Change &insert, ChangeResult *changeResult,
                QList<FxViewItem *> *newItems, QList<MovedItem> *movingIntoView) = 0;

    virtual bool needsRefillForAddedOrRemovedIndex(int) const { return false; }
#if QT_CONFIG(quick_viewtransitions)
    virtual void translateAndTransitionItemsAfter(int afterIndex, const ChangeResult &insertionResult, const ChangeResult &removalResult) = 0;
#endif

    virtual void initializeViewItem(FxViewItem *) {}
    virtual void initializeCurrentItem() {}
    virtual void updateSectionCriteria() {}
    virtual void updateSections() {}

    void itemGeometryChanged(QQuickItem *item, QQuickGeometryChange change, const QRectF &) override;
};


QT_END_NAMESPACE

#endif // QQUICKITEMVIEW_P_P_H
