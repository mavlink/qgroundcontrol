// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QABSTRACTITEMVIEW_P_H
#define QABSTRACTITEMVIEW_P_H

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

#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include "qabstractitemview.h"
#include "private/qabstractscrollarea_p.h"
#include "private/qabstractitemmodel_p.h"
#include "QtWidgets/qapplication.h"
#include "QtGui/qevent.h"
#include "QtCore/qmimedata.h"
#include "QtGui/qpainter.h"
#include "QtGui/qregion.h"

#include "QtCore/qbasictimer.h"
#include "QtCore/qelapsedtimer.h"
#include <QtCore/qpointer.h>


#include <array>

QT_REQUIRE_CONFIG(itemviews);

QT_BEGIN_NAMESPACE

struct QEditorInfo {
    QEditorInfo(QWidget *e, bool s): widget(QPointer<QWidget>(e)), isStatic(s) {}
    QEditorInfo(): isStatic(false) {}

    QPointer<QWidget> widget;
    bool isStatic;
};

//  Fast associativity between Persistent editors and indices.
typedef QHash<QWidget *, QPersistentModelIndex> QEditorIndexHash;
typedef QHash<QPersistentModelIndex, QEditorInfo> QIndexEditorHash;

struct QItemViewPaintPair {
    QRect rect;
    QModelIndex index;
};
template <>
class QTypeInfo<QItemViewPaintPair> : public QTypeInfoMerger<QItemViewPaintPair, QRect, QModelIndex> {};

typedef QList<QItemViewPaintPair> QItemViewPaintPairs;

class Q_AUTOTEST_EXPORT QAbstractItemViewPrivate : public QAbstractScrollAreaPrivate
{
    Q_DECLARE_PUBLIC(QAbstractItemView)

public:
    QAbstractItemViewPrivate();
    virtual ~QAbstractItemViewPrivate();

    void init();

    virtual void rowsRemoved(const QModelIndex &parent, int start, int end);
    virtual void rowsInserted(const QModelIndex &parent, int start, int end);
    virtual void columnsAboutToBeRemoved(const QModelIndex &parent, int start, int end);
    virtual void columnsRemoved(const QModelIndex &parent, int start, int end);
    virtual void columnsInserted(const QModelIndex &parent, int start, int end);
    virtual void modelDestroyed();
    virtual void layoutChanged();
    virtual void rowsMoved(const QModelIndex &source, int sourceStart, int sourceEnd, const QModelIndex &destination, int destinationStart);
    virtual void columnsMoved(const QModelIndex &source, int sourceStart, int sourceEnd, const QModelIndex &destination, int destinationStart);
    virtual QRect intersectedRect(const QRect rect, const QModelIndex &topLeft, const QModelIndex &bottomRight) const;

    void headerDataChanged() { doDelayedItemsLayout(); }
    void scrollerStateChanged();
    void delegateSizeHintChanged(const QModelIndex &index);

    void fetchMore();

    bool shouldEdit(QAbstractItemView::EditTrigger trigger, const QModelIndex &index) const;
    bool shouldForwardEvent(QAbstractItemView::EditTrigger trigger, const QEvent *event) const;
    bool shouldAutoScroll(const QPoint &pos) const;
    void doDelayedItemsLayout(int delay = 0);
    void interruptDelayedItemsLayout() const;

    void updateGeometry();

    void startAutoScroll()
    {   // ### it would be nice to make this into a style hint one day
        int scrollInterval = (verticalScrollMode == QAbstractItemView::ScrollPerItem) ? 150 : 50;
        autoScrollTimer.start(scrollInterval, q_func());
        autoScrollCount = 0;
    }
    void stopAutoScroll() { autoScrollTimer.stop(); autoScrollCount = 0;}

#if QT_CONFIG(draganddrop)
    virtual bool dropOn(QDropEvent *event, int *row, int *col, QModelIndex *index);
#endif
    bool droppingOnItself(QDropEvent *event, const QModelIndex &index);

    QWidget *editor(const QModelIndex &index, const QStyleOptionViewItem &options);
    bool sendDelegateEvent(const QModelIndex &index, QEvent *event) const;
    bool openEditor(const QModelIndex &index, QEvent *event);
    void updateEditorData(const QModelIndex &topLeft, const QModelIndex &bottomRight);
    void selectAllInEditor(QWidget *w);

    QItemSelectionModel::SelectionFlags multiSelectionCommand(const QModelIndex &index,
                                                              const QEvent *event) const;
    QItemSelectionModel::SelectionFlags extendedSelectionCommand(const QModelIndex &index,
                                                                 const QEvent *event) const;
    QItemSelectionModel::SelectionFlags contiguousSelectionCommand(const QModelIndex &index,
                                                                   const QEvent *event) const;
    virtual void selectAll(QItemSelectionModel::SelectionFlags command);

    void setHoverIndex(const QPersistentModelIndex &index);

    void checkMouseMove(const QPersistentModelIndex &index);
    inline void checkMouseMove(const QPoint &pos) { checkMouseMove(q_func()->indexAt(pos)); }

    inline QItemSelectionModel::SelectionFlags selectionBehaviorFlags() const
    {
        switch (selectionBehavior) {
        case QAbstractItemView::SelectRows: return QItemSelectionModel::Rows;
        case QAbstractItemView::SelectColumns: return QItemSelectionModel::Columns;
        case QAbstractItemView::SelectItems: default: return QItemSelectionModel::NoUpdate;
        }
    }

#if QT_CONFIG(draganddrop)
    virtual QAbstractItemView::DropIndicatorPosition position(const QPoint &pos, const QRect &rect, const QModelIndex &idx) const;

    inline bool canDrop(QDropEvent *event) {
        const QMimeData *mime = event->mimeData();

        // Drag enter event shall always be accepted, if mime type and action match.
        // Whether the data can actually be dropped will be checked in drag move.
        if (event->type() == QEvent::DragEnter && (event->dropAction() & model->supportedDropActions())) {
            const QStringList modelTypes = model->mimeTypes();
            for (const auto &modelType : modelTypes) {
                if (mime->hasFormat(modelType))
                    return true;
            }
        }

        QModelIndex index;
        int col = -1;
        int row = -1;
        if (dropOn(event, &row, &col, &index)) {
            return model->canDropMimeData(mime,
                                          dragDropMode == QAbstractItemView::InternalMove ? Qt::MoveAction : event->dropAction(),
                                          row, col, index);
        }
        return false;
    }

    inline void paintDropIndicator(QPainter *painter)
    {
        if (showDropIndicator && state == QAbstractItemView::DraggingState
            && !dropIndicatorRect.isNull()
#ifndef QT_NO_CURSOR
            && viewport->cursor().shape() != Qt::ForbiddenCursor
#endif
        ) {
            QStyleOption opt;
            opt.initFrom(q_func());
            opt.rect = dropIndicatorRect;
            q_func()->style()->drawPrimitive(QStyle::PE_IndicatorItemViewItemDrop, &opt, painter, q_func());
        }
    }

#endif
    virtual QItemViewPaintPairs draggablePaintPairs(const QModelIndexList &indexes, QRect *r) const;
    // reimplemented in subclasses
    virtual void adjustViewOptionsForIndex(QStyleOptionViewItem*, const QModelIndex&) const {}

    inline void releaseEditor(QWidget *editor, const QModelIndex &index = QModelIndex()) const {
        if (editor) {
            Q_Q(const QAbstractItemView);
            QObject::disconnect(editor, &QWidget::destroyed,
                                q, &QAbstractItemView::editorDestroyed);
            editor->removeEventFilter(itemDelegate);
            editor->hide();
            QAbstractItemDelegate *delegate = q->itemDelegateForIndex(index);

            if (delegate)
                delegate->destroyEditor(editor, index);
            else
                editor->deleteLater();
        }
    }

    inline void executePostedLayout() const {
        if (delayedPendingLayout && state != QAbstractItemView::CollapsingState) {
            interruptDelayedItemsLayout();
            const_cast<QAbstractItemView*>(q_func())->doItemsLayout();
        }
    }

    inline void setDirtyRegion(const QRegion &visualRegion) {
        updateRegion += visualRegion;
        if (!updateTimer.isActive())
            updateTimer.start(0, q_func());
    }

    inline void scrollDirtyRegion(int dx, int dy) {
        scrollDelayOffset = QPoint(-dx, -dy);
        updateDirtyRegion();
        scrollDelayOffset = QPoint(0, 0);
    }

    inline void scrollContentsBy(int dx, int dy) {
        scrollDirtyRegion(dx, dy);
        viewport->scroll(dx, dy);
    }

    void updateDirtyRegion() {
        updateTimer.stop();
        viewport->update(updateRegion);
        updateRegion = QRegion();
    }

    void clearOrRemove();
    void checkPersistentEditorFocus();

    QPixmap renderToPixmap(const QModelIndexList &indexes, QRect *r) const;

    inline QPoint offset() const {
        const Q_Q(QAbstractItemView);
        return QPoint(q->isRightToLeft() ? -q->horizontalOffset()
                      : q->horizontalOffset(), q->verticalOffset());
    }

    const QEditorInfo &editorForIndex(const QModelIndex &index) const;
    bool hasEditor(const QModelIndex &index) const;

    QModelIndex indexForEditor(QWidget *editor) const;
    void addEditor(const QModelIndex &index, QWidget *editor, bool isStatic);
    void removeEditor(QWidget *editor);

    inline bool isAnimating() const {
        return state == QAbstractItemView::AnimatingState;
    }

    inline bool isIndexValid(const QModelIndex &index) const {
         return (index.row() >= 0) && (index.column() >= 0) && (index.model() == model);
    }
    inline bool isIndexSelectable(const QModelIndex &index) const {
        return (model->flags(index) & Qt::ItemIsSelectable);
    }
    inline bool isIndexEnabled(const QModelIndex &index) const {
        return (model->flags(index) & Qt::ItemIsEnabled);
    }
#if QT_CONFIG(draganddrop)
    inline bool isIndexDropEnabled(const QModelIndex &index) const {
        return (model->flags(index) & Qt::ItemIsDropEnabled);
    }
    inline bool isIndexDragEnabled(const QModelIndex &index) const {
        return (model->flags(index) & Qt::ItemIsDragEnabled);
    }
#endif

    virtual bool selectionAllowed(const QModelIndex &index) const {
        // in some views we want to go ahead with selections, even if the index is invalid
        return isIndexValid(index) && isIndexSelectable(index);
    }

    // reimplemented from QAbstractScrollAreaPrivate
    QPoint contentsOffset() const override {
        Q_Q(const QAbstractItemView);
        return QPoint(q->horizontalOffset(), q->verticalOffset());
    }

    /**
     * For now, assume that we have few editors, if we need a more efficient implementation
     * we should add a QMap<QAbstractItemDelegate*, int> member.
     */
    int delegateRefCount(const QAbstractItemDelegate *delegate) const
    {
        int ref = 0;
        if (itemDelegate == delegate)
            ++ref;

        for (int maps = 0; maps < 2; ++maps) {
            const QMap<int, QPointer<QAbstractItemDelegate> > *delegates = maps ? &columnDelegates : &rowDelegates;
            for (QMap<int, QPointer<QAbstractItemDelegate> >::const_iterator it = delegates->begin();
                it != delegates->end(); ++it) {
                    if (it.value() == delegate) {
                        ++ref;
                        // optimization, we are only interested in the ref count values 0, 1 or >=2
                        if (ref >= 2) {
                            return ref;
                        }
                    }
            }
        }
        return ref;
    }

    /**
     * return true if the index is registered as a QPersistentModelIndex
     */
    inline bool isPersistent(const QModelIndex &index) const
    {
        return static_cast<QAbstractItemModelPrivate *>(model->d_ptr.data())->persistent.indexes.contains(index);
    }

#if QT_CONFIG(draganddrop)
    QModelIndexList selectedDraggableIndexes() const;
    void maybeStartDrag(QPoint eventPoint);
#endif

    void doDelayedReset()
    {
        //we delay the reset of the timer because some views (QTableView)
        //with headers can't handle the fact that the model has been destroyed
        //all modelDestroyed() slots must have been called
        if (!delayedReset.isActive())
            delayedReset.start(0, q_func());
    }

    QAbstractItemModel *model;
    QPointer<QAbstractItemDelegate> itemDelegate;
    QMap<int, QPointer<QAbstractItemDelegate> > rowDelegates;
    QMap<int, QPointer<QAbstractItemDelegate> > columnDelegates;
    QPointer<QItemSelectionModel> selectionModel;
    QItemSelectionModel::SelectionFlag ctrlDragSelectionFlag;
    bool noSelectionOnMousePress;

    QAbstractItemView::SelectionMode selectionMode;
    QAbstractItemView::SelectionBehavior selectionBehavior;

    QEditorIndexHash editorIndexHash;
    QIndexEditorHash indexEditorHash;
    QSet<QWidget*> persistent;
    QWidget *currentlyCommittingEditor;
    QBasicTimer pressClosedEditorWatcher;
    QPersistentModelIndex lastEditedIndex;
    bool pressClosedEditor;
    bool waitForIMCommit;

    QPersistentModelIndex enteredIndex;
    QPersistentModelIndex pressedIndex;
    QPersistentModelIndex currentSelectionStartIndex;
    Qt::KeyboardModifiers pressedModifiers;
    QPoint pressedPosition;
    QPoint draggedPosition;
    QPoint draggedPositionOffset;
    bool pressedAlreadySelected;
    bool releaseFromDoubleClick;

    //forces the next mouseMoveEvent to send the viewportEntered signal
    //if the mouse is over the viewport and not over an item
    bool viewportEnteredNeeded;

    QAbstractItemView::State state;
    QAbstractItemView::State stateBeforeAnimation;
    QAbstractItemView::EditTriggers editTriggers;
    QAbstractItemView::EditTrigger lastTrigger;

    QPersistentModelIndex root;
    QPersistentModelIndex hover;

    bool tabKeyNavigation;

#if QT_CONFIG(draganddrop)
    bool showDropIndicator;
    QRect dropIndicatorRect;
    bool dragEnabled;
    QAbstractItemView::DragDropMode dragDropMode;
    bool overwrite;
    bool dropEventMoved;
    QAbstractItemView::DropIndicatorPosition dropIndicatorPosition;
    Qt::DropAction defaultDropAction;
#endif

    QString keyboardInput;
    QElapsedTimer keyboardInputTime;

    bool autoScroll;
    QBasicTimer autoScrollTimer;
    int autoScrollMargin;
    int autoScrollCount;
    bool shouldScrollToCurrentOnShow; //used to know if we should scroll to current on show event
    bool shouldClearStatusTip; //if there is a statustip currently shown that need to be cleared when leaving.

    bool alternatingColors;

    QSize iconSize;
    Qt::TextElideMode textElideMode;

    QRegion updateRegion; // used for the internal update system
    QPoint scrollDelayOffset;

    QBasicTimer updateTimer;
    QBasicTimer delayedEditing;
    QBasicTimer delayedAutoScroll; //used when an item is clicked
    QBasicTimer delayedReset;

    QAbstractItemView::ScrollMode verticalScrollMode;
    QAbstractItemView::ScrollMode horizontalScrollMode;

#ifndef QT_NO_GESTURES
    // the selection before the last mouse down. In case we have to restore it for scrolling
    QItemSelection oldSelection;
    QModelIndex oldCurrent;
#endif

    bool currentIndexSet;

    bool wrapItemText;
    mutable bool delayedPendingLayout;
    bool moveCursorUpdatedView;

    // Whether scroll mode has been explicitly set or its value come from SH_ItemView_ScrollMode
    bool verticalScrollModeSet;
    bool horizontalScrollModeSet;

    int updateThreshold;

    virtual QRect visualRect(const QModelIndex &index) const { return q_func()->visualRect(index); }

    std::array<QMetaObject::Connection, 14> modelConnections;
    std::array<QMetaObject::Connection, 4> scrollbarConnections;
#if QT_CONFIG(gestures) && QT_CONFIG(scroller)
    QMetaObject::Connection scollerConnection;
#endif

private:
    void connectDelegate(QAbstractItemDelegate *delegate);
    void disconnectDelegate(QAbstractItemDelegate *delegate);
    void disconnectAll();
    inline QAbstractItemDelegate *delegateForIndex(const QModelIndex &index) const {
        QMap<int, QPointer<QAbstractItemDelegate> >::ConstIterator it;

        it = rowDelegates.find(index.row());
        if (it != rowDelegates.end())
            return it.value();

        it = columnDelegates.find(index.column());
        if (it != columnDelegates.end())
            return it.value();

        return itemDelegate;
    }

    mutable QBasicTimer delayedLayout;
    mutable QBasicTimer fetchMoreTimer;
};

QT_BEGIN_INCLUDE_NAMESPACE
#include <qlist.h>
QT_END_INCLUDE_NAMESPACE

template<typename T>
inline int qBinarySearch(const QList<T> &vec, const T &item, int start, int end)
{
    int i = (start + end + 1) >> 1;
    while (end - start > 0) {
        if (vec.at(i) > item)
            end = i - 1;
        else
            start = i;
        i = (start + end + 1) >> 1;
    }
    return i;
}

QT_END_NAMESPACE

#endif // QABSTRACTITEMVIEW_P_H
