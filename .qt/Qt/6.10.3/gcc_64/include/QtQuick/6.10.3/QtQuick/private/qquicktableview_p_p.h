// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKTABLEVIEW_P_P_H
#define QQUICKTABLEVIEW_P_P_H

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

#include "qquicktableview_p.h"

#include <QtCore/qtimer.h>
#include <QtCore/qitemselectionmodel.h>
#include <QtQmlModels/private/qqmltableinstancemodel_p.h>
#include <QtQml/private/qqmlincubator_p.h>
#include <QtQmlModels/private/qqmlchangeset_p.h>
#include <QtQml/qqmlinfo.h>

#include <QtQuick/private/qquickflickable_p_p.h>
#include <QtQuick/private/qquickitemviewfxitem_p_p.h>
#include <QtQuick/private/qquickanimation_p.h>
#include <QtQuick/private/qquickselectable_p.h>
#include <QtQuick/private/qquicksinglepointhandler_p.h>
#include <QtQuick/private/qquickhoverhandler_p.h>
#include <QtQuick/private/qquicktaphandler_p.h>

#include <QtCore/private/qminimalflatset_p.h>

#if QT_CONFIG(quick_draganddrop)
#include <QtGui/qdrag.h>
#include <QtQuick/private/qquickdroparea_p.h>
#endif

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcTableViewDelegateLifecycle)

static const qreal kDefaultRowHeight = 50;
static const qreal kDefaultColumnWidth = 50;
static const int kEdgeIndexNotSet = -2;
static const int kEdgeIndexAtEnd = -3;

class FxTableItem;
class QQuickTableSectionSizeProviderPrivate;

/*! \internal
 *  TableView uses QQuickTableViewHoverHandler to track where the pointer is
 *  on top of the table, and change the cursor at the places where a drag
 *  would start a resize of a row or a column.
 */
class QQuickTableViewHoverHandler : public QQuickHoverHandler
{
    Q_OBJECT

public:
    QQuickTableViewHoverHandler(QQuickTableView *view);
    inline bool isHoveringGrid() const { return m_row != -1 || m_column != -1; };

    int m_row = -1;
    int m_column = -1;

    friend class QQuickTableViewPrivate;

protected:
    void handleEventPoint(QPointerEvent *event, QEventPoint &point) override;
};

class QQuickTableViewPointerHandler : public QQuickSinglePointHandler
{
    Q_OBJECT

public:
    enum State {
        Listening, // the pointer is not being pressed between the cells
        Tracking, // the pointer is being pressed between the cells
        DraggingStarted, // dragging started
        Dragging, // a drag is ongoing
        DraggingFinished // dragging was finished
    };

    QQuickTableViewPointerHandler(QQuickTableView *view);

    State m_state = Listening;
    State state() { return m_state; }

protected:
    bool wantsEventPoint(const QPointerEvent *event, const QEventPoint &point) override;
};

/*! \internal
 *  TableView uses QQuickTableViewResizeHandler to enable the user to resize
 *  rows and columns. By using a custom pointer handler, we can get away with
 *  using a single pointer handler for the whole content item, rather than
 *  e.g having to split it up into multiple items with drag handlers placed
 *  between the cells.
 */
class QQuickTableViewResizeHandler : public QQuickTableViewPointerHandler
{
    Q_OBJECT

public:
    QQuickTableViewResizeHandler(QQuickTableView *view);

    int m_row = -1;
    qreal m_rowStartY = -1;
    qreal m_rowStartHeight = -1;

    int m_column = -1;
    qreal m_columnStartX = -1;
    qreal m_columnStartWidth = -1;

    void updateState(QEventPoint &point);
    void updateDrag(QPointerEvent *event, QEventPoint &point);

    friend class QQuickTableViewPrivate;

protected:
    void handleEventPoint(QPointerEvent *event, QEventPoint &point) override;
    void onGrabChanged(QQuickPointerHandler *grabber, QPointingDevice::GrabTransition transition,
                       QPointerEvent *ev, QEventPoint &point) override;
};

#if QT_CONFIG(quick_draganddrop)
class QQuickTableViewSectionDragHandler : public QQuickTableViewPointerHandler
{
    Q_OBJECT

public:
    QQuickTableViewSectionDragHandler(QQuickTableView *view);
    ~QQuickTableViewSectionDragHandler();

    void grabSection();

    void handleDrag(QQuickDragEvent *event);
    void handleDrop(QQuickDragEvent *event);
    void handleDragDropAction(Qt::DropAction action);

    void setSectionOrientation(Qt::Orientation orientation) { m_sectionOrientation = orientation; }

    friend class QQuickTableViewPrivate;

protected:
    void handleEventPoint(QPointerEvent *event, QEventPoint &point) override;

private:
    void resetDragData();
    void resetSectionOverlay();

    QSharedPointer<QQuickItemGrabResult> m_grabResult;
    QPointer<QDrag> m_drag;
    int m_source = -1;
    int m_destination = -1;
    QPointer<QQuickDropArea> m_dropArea;
    Qt::Orientation m_sectionOrientation;

    QPointF m_dragPoint;
    QSizeF m_step = QSizeF(1, 1);
    QTimer m_scrollTimer;
};
#endif // quick_draganddrop

/*! \internal
 *  QQuickTableViewTapHandler used to handle tap events explicitly for table view
 */
class QQuickTableViewTapHandler : public QQuickTapHandler
{
    Q_OBJECT

public:
    explicit QQuickTableViewTapHandler(QQuickTableView *view);
    bool wantsEventPoint(const QPointerEvent *event, const QEventPoint &point) override;

    friend class QQuickTableViewPrivate;
};

class Q_QUICK_EXPORT QQuickTableViewPrivate : public QQuickFlickablePrivate, public QQuickSelectable
{
public:
    Q_DECLARE_PUBLIC(QQuickTableView)

    class TableEdgeLoadRequest
    {
        // Whenever we need to load new rows or columns in the
        // table, we fill out a TableEdgeLoadRequest.
        // TableEdgeLoadRequest is just a struct that keeps track
        // of which cells that needs to be loaded, and which cell
        // the table is currently loading. The loading itself is
        // done by QQuickTableView.

    public:
        void begin(const QPoint &cell, const QPointF &pos, QQmlIncubator::IncubationMode incubationMode)
        {
            Q_ASSERT(!m_active);
            m_active = true;
            m_edge = Qt::Edge(0);
            m_mode = incubationMode;
            m_edgeIndex = cell.x();
            m_visibleCellsInEdge.clear();
            m_visibleCellsInEdge.append(cell.y());
            m_currentIndex = 0;
            m_startPos = pos;
            qCDebug(lcTableViewDelegateLifecycle()) << "begin top-left:" << toString();
        }

        void begin(Qt::Edge edgeToLoad, int edgeIndex, const QVector<int> visibleCellsInEdge, QQmlIncubator::IncubationMode incubationMode)
        {
            Q_ASSERT(!m_active);
            m_active = true;
            m_edge = edgeToLoad;
            m_edgeIndex = edgeIndex;
            m_visibleCellsInEdge = visibleCellsInEdge;
            m_mode = incubationMode;
            m_currentIndex = 0;
            qCDebug(lcTableViewDelegateLifecycle()) << "begin:" << toString();
        }

        inline void markAsDone() { m_active = false; }
        inline bool isActive() const { return m_active; }

        inline QPoint currentCell() const { return cellAt(m_currentIndex); }
        inline bool hasCurrentCell() const { return m_currentIndex < m_visibleCellsInEdge.size(); }
        inline void moveToNextCell() { ++m_currentIndex; }

        inline Qt::Edge edge() const { return m_edge; }
        inline int row() const { return cellAt(0).y(); }
        inline int column() const { return cellAt(0).x(); }
        inline QQmlIncubator::IncubationMode incubationMode() const { return m_mode; }

        inline QPointF startPosition() const { return m_startPos; }

        QString toString() const
        {
            QString str;
            QDebug dbg(&str);
            dbg.nospace() << "TableSectionLoadRequest(" << "edge:"
                << m_edge << ", edgeIndex:" << m_edgeIndex << ", incubation:";

            switch (m_mode) {
            case QQmlIncubator::Asynchronous:
                dbg << "Asynchronous";
                break;
            case QQmlIncubator::AsynchronousIfNested:
                dbg << "AsynchronousIfNested";
                break;
            case QQmlIncubator::Synchronous:
                dbg << "Synchronous";
                break;
            }

            return str;
        }

    private:
        Qt::Edge m_edge = Qt::Edge(0);
        QVector<int> m_visibleCellsInEdge;
        int m_edgeIndex = 0;
        int m_currentIndex = 0;
        bool m_active = false;
        QQmlIncubator::IncubationMode m_mode = QQmlIncubator::AsynchronousIfNested;
        QPointF m_startPos;

        inline QPoint cellAt(int index) const {
            return !m_edge || (m_edge & (Qt::LeftEdge | Qt::RightEdge))
                    ? QPoint(m_edgeIndex, m_visibleCellsInEdge[index])
                    : QPoint(m_visibleCellsInEdge[index], m_edgeIndex);
        }
    };

    class EdgeRange {
    public:
        EdgeRange();
        bool containsIndex(Qt::Edge edge, int index);

        int startIndex;
        int endIndex;
        qreal size;
    };

    enum class RebuildState {
        Begin = 0,
        LoadInitalTable,
        VerifyTable,
        LayoutTable,
        CancelOvershoot,
        UpdateContentSize,
        PreloadColumns,
        PreloadRows,
        MovePreloadedItemsToPool,
        Done
    };

    enum class SectionState {
        Idle = 0,
        Moving
    };

    enum class RebuildOption {
        None = 0,
        All = 0x1,
        LayoutOnly = 0x2,
        ViewportOnly = 0x4,
        CalculateNewTopLeftRow = 0x8,
        CalculateNewTopLeftColumn = 0x10,
        CalculateNewContentWidth = 0x20,
        CalculateNewContentHeight = 0x40,
        PositionViewAtRow = 0x80,
        PositionViewAtColumn = 0x100,
    };
    Q_DECLARE_FLAGS(RebuildOptions, RebuildOption)

public:
    QQuickTableViewPrivate();
    ~QQuickTableViewPrivate() override;

    static inline QQuickTableViewPrivate *get(QQuickTableView *q) { return q->d_func(); }

    void updatePolish() override;
    void fixup(AxisData &data, qreal minExtent, qreal maxExtent) override;

public:
    QHash<int, FxTableItem *> loadedItems;

    // model, tableModel and modelVariant all point to the same model. modelVariant
    // is the model assigned by the user. And tableModel is the wrapper model we create
    // around it. But if the model is an instance model directly, we cannot wrap it, so
    // we need a pointer for that case as well.
    QQmlInstanceModel* model = nullptr;
    QPointer<QQmlTableInstanceModel> tableModel = nullptr;

    // When the applications assignes a new model or delegate to the view, we keep them
    // around until we're ready to take them into use (syncWithPendingChanges).
    QVariant assignedModel = QVariant(int(0));
    QQmlGuard<QQmlComponent> assignedDelegate;

    // loadedRows/Columns describes the rows and columns that are currently loaded (from top left
    // row/column to bottom right row/column). loadedTableOuterRect describes the actual
    // pixels that all the loaded delegate items cover, and is matched agains the viewport to determine when
    // we need to fill up with more rows/columns. loadedTableInnerRect describes the pixels
    // that the loaded table covers if you remove one row/column on each side of the table, and
    // is used to determine rows/columns that are no longer visible and can be unloaded.
    QMinimalFlatSet<int> loadedColumns;
    QMinimalFlatSet<int> loadedRows;
    QRectF loadedTableOuterRect;
    QRectF loadedTableInnerRect;

    QPointF origin = QPointF(0, 0);
    QSizeF endExtent = QSizeF(0, 0);

    QRectF viewportRect = QRectF(0, 0, -1, -1);

    QSize tableSize;

    RebuildState rebuildState = RebuildState::Done;
    RebuildOptions rebuildOptions = RebuildOption::All;
    RebuildOptions scheduledRebuildOptions = RebuildOption::All;

    TableEdgeLoadRequest loadRequest;

    QSizeF cellSpacing = QSizeF(0, 0);

    QQmlTableInstanceModel::ReusableFlag reusableFlag = QQmlTableInstanceModel::Reusable;

    bool blockItemCreatedCallback = false;
    mutable bool layoutWarningIssued = false;
    bool polishing = false;
    bool syncVertically = false;
    bool syncHorizontally = false;
    bool inSetLocalViewportPos = false;
    bool inSyncViewportPosRecursive = false;
    bool inUpdateContentSize = false;
    bool animate = true;
    bool keyNavigationEnabled = true;
    bool pointerNavigationEnabled = true;
    bool alternatingRows = true;
    bool resizableColumns = false;
    bool resizableRows = false;
#if QT_CONFIG(cursor)
    bool m_cursorSet = false;
#endif

    // isTransposed is currently only used by HeaderView.
    // Consider making it public.
    bool isTransposed = false;

    bool warnNoSelectionModel = true;

    QQmlDelegateModel::DelegateModelAccess assignedDelegateModelAccess
            = QQmlDelegateModel::Qt5ReadWrite;

    QJSValue rowHeightProvider;
    QJSValue columnWidthProvider;

    mutable EdgeRange cachedNextVisibleEdgeIndex[4];
    mutable EdgeRange cachedColumnWidth;
    mutable EdgeRange cachedRowHeight;

    // TableView uses contentWidth/height to report the size of the table (this
    // will e.g make scrollbars written for Flickable work out of the box). This
    // value is continuously calculated, and will change/improve as more columns
    // are loaded into view. At the same time, we want to open up for the
    // possibility that the application can set the content width explicitly, in
    // case it knows what the exact width should be from the start. We therefore
    // override the contentWidth/height properties from QQuickFlickable, to be able
    // to implement this combined behavior. This also lets us lazy build the table
    // if the application needs to know the content size early on.
    QQmlNullableValue<qreal> explicitContentWidth;
    QQmlNullableValue<qreal> explicitContentHeight;

    QSizeF averageEdgeSize;

    QPointer<QQuickTableView> assignedSyncView;
    QPointer<QQuickTableView> syncView;
    QList<QPointer<QQuickTableView> > syncChildren;
    Qt::Orientations assignedSyncDirection = Qt::Horizontal | Qt::Vertical;

    QPointer<QItemSelectionModel> selectionModel;
    QQuickTableView::SelectionBehavior selectionBehavior = QQuickTableView::SelectCells;
    QQuickTableView::SelectionMode selectionMode = QQuickTableView::ExtendedSelection;
    QItemSelectionModel::SelectionFlag selectionFlag = QItemSelectionModel::NoUpdate;
    std::function<void(CallBackFlag)> selectableCallbackFunction;
    bool inSelectionModelUpdate = false;
    bool needsModelSynchronization = false;

    int assignedPositionViewAtRowAfterRebuild = 0;
    int assignedPositionViewAtColumnAfterRebuild = 0;
    int positionViewAtRowAfterRebuild = 0;
    int positionViewAtColumnAfterRebuild = 0;
    qreal positionViewAtRowOffset = 0;
    qreal positionViewAtColumnOffset = 0;
    QRectF positionViewAtRowSubRect;
    QRectF positionViewAtColumnSubRect;
    Qt::Alignment positionViewAtRowAlignment = Qt::AlignTop;
    Qt::Alignment positionViewAtColumnAlignment = Qt::AlignLeft;

    QQuickPropertyAnimation positionXAnimation;
    QQuickPropertyAnimation positionYAnimation;

    QPoint selectionStartCell = {-1, -1};
    QPoint selectionEndCell = {-1, -1};
    QItemSelection existingSelection;

    QMargins edgesBeforeRebuild;
    QSize tableSizeBeforeRebuild;

    int currentRow = -1;
    int currentColumn = -1;

    QHash<int, qreal> explicitColumnWidths;
    QHash<int, qreal> explicitRowHeights;

    QQuickTableViewHoverHandler *hoverHandler = nullptr;
    QQuickTableViewResizeHandler *resizeHandler = nullptr;
#if QT_CONFIG(quick_draganddrop)
    QQuickTableViewSectionDragHandler *sectionDragHandler = nullptr;
#endif
    QQuickTableViewPointerHandler *activePtrHandler = nullptr;

    QQmlTableInstanceModel *editModel = nullptr;
    QQuickItem *editItem = nullptr;
    QPersistentModelIndex editIndex;
    QQuickTableView::EditTriggers editTriggers = QQuickTableView::DoubleTapped | QQuickTableView::EditKeyPressed;

#ifdef QT_DEBUG
    QString forcedIncubationMode = qEnvironmentVariable("QT_TABLEVIEW_INCUBATION_MODE");
#endif

    struct SectionData {
        int index = -1;
        int prevIndex = -1;
    };

    QList<SectionData> visualIndices[Qt::Vertical];
    QList<SectionData> logicalIndices[Qt::Vertical];

    SectionState m_sectionState = SectionState::Idle;

public:
    void init();

    QQuickTableViewAttached *getAttachedObject(const QObject *object) const;

    int modelIndexAtCell(const QPoint &cell) const;
    QPoint cellAtModelIndex(int modelIndex) const;
    int modelIndexToCellIndex(const QModelIndex &modelIndex, bool visualIndex = true) const;
    inline bool cellIsValid(const QPoint &cell) const { return cell.x() != -1 && cell.y() != -1; }

    qreal sizeHintForColumn(int column) const;
    qreal sizeHintForRow(int row) const;
    QSize calculateTableSize();
    void updateTableSize();

    inline bool isColumnHidden(int column) const;
    inline bool isRowHidden(int row) const;

    qreal getColumnLayoutWidth(int column);
    qreal getRowLayoutHeight(int row);
    qreal getColumnWidth(int column) const;
    qreal getRowHeight(int row) const;
    qreal getEffectiveRowY(int row) const;
    qreal getEffectiveRowHeight(int row) const;
    qreal getEffectiveColumnX(int column) const;
    qreal getEffectiveColumnWidth(int column) const;
    qreal getAlignmentContentX(int column, Qt::Alignment alignment, const qreal offset, const QRectF &subRect);
    qreal getAlignmentContentY(int row, Qt::Alignment alignment, const qreal offset, const QRectF &subRect);

    int topRow() const { return *loadedRows.cbegin(); }
    int bottomRow() const { return *loadedRows.crbegin(); }
    int leftColumn() const { return *loadedColumns.cbegin(); }
    int rightColumn() const { return *loadedColumns.crbegin(); }

    QQuickTableView *rootSyncView() const;

    bool updateTableRecursive();
    bool updateTable();
    void relayoutTableItems();

    void layoutVerticalEdge(Qt::Edge tableEdge);
    void layoutHorizontalEdge(Qt::Edge tableEdge);
    void layoutTopLeftItem();
    void layoutTableEdgeFromLoadRequest();

    void updateContentWidth();
    void updateContentHeight();
    void updateAverageColumnWidth();
    void updateAverageRowHeight();
    RebuildOptions checkForVisibilityChanges();
    void forceLayout(bool immediate);

    void updateExtents();
    void syncLoadedTableRectFromLoadedTable();
    void syncLoadedTableFromLoadRequest();

    int nextVisibleEdgeIndex(Qt::Edge edge, int startIndex) const;
    int nextVisibleEdgeIndexAroundLoadedTable(Qt::Edge edge) const;
    inline bool atTableEnd(Qt::Edge edge) const { return nextVisibleEdgeIndexAroundLoadedTable(edge) == kEdgeIndexAtEnd; }
    inline bool atTableEnd(Qt::Edge edge, int startIndex) const { return nextVisibleEdgeIndex(edge, startIndex) == kEdgeIndexAtEnd; }
    inline int edgeToArrayIndex(Qt::Edge edge) const;
    void clearEdgeSizeCache();

    bool canLoadTableEdge(Qt::Edge tableEdge, const QRectF fillRect) const;
    bool canUnloadTableEdge(Qt::Edge tableEdge, const QRectF fillRect) const;
    Qt::Edge nextEdgeToLoad(const QRectF rect);
    Qt::Edge nextEdgeToUnload(const QRectF rect);

    qreal cellWidth(const QPoint &cell) const;
    qreal cellHeight(const QPoint &cell) const;

    FxTableItem *loadedTableItem(const QPoint &cell) const;
    FxTableItem *createFxTableItem(const QPoint &cell, QQmlIncubator::IncubationMode incubationMode);
    FxTableItem *loadFxTableItem(const QPoint &cell, QQmlIncubator::IncubationMode incubationMode);

    void releaseItem(FxTableItem *fxTableItem, QQmlTableInstanceModel::ReusableFlag reusableFlag);
    void releaseLoadedItems(QQmlTableInstanceModel::ReusableFlag reusableFlag);

    void unloadItem(const QPoint &cell);
    void loadEdge(Qt::Edge edge, QQmlIncubator::IncubationMode incubationMode);
    void unloadEdge(Qt::Edge edge);
    void loadAndUnloadVisibleEdges(QQmlIncubator::IncubationMode incubationMode = QQmlIncubator::AsynchronousIfNested);
    void drainReusePoolAfterLoadRequest();
    void processLoadRequest();

    void processRebuildTable();
    bool moveToNextRebuildState();
    void calculateTopLeft(QPoint &topLeft, QPointF &topLeftPos);
    void loadInitialTable();

    void layoutAfterLoadingInitialTable();
    void adjustViewportXAccordingToAlignment();
    void adjustViewportYAccordingToAlignment();
    void cancelOvershootAfterLayout();

    void scheduleRebuildTable(QQuickTableViewPrivate::RebuildOptions options);

#if QT_CONFIG(cursor)
    void updateCursor();
#endif
    void updateEditItem();
    void updateContentSize();

    QTypeRevision resolveImportVersion();
    void createWrapperModel();
    QAbstractItemModel *qaim(QVariant modelAsVariant) const;

    virtual void initItemCallback(int modelIndex, QObject *item);
    virtual void itemCreatedCallback(int modelIndex, QObject *object);
    virtual void itemPooledCallback(int modelIndex, QObject *object);
    virtual void itemReusedCallback(int modelIndex, QObject *object);
    virtual void modelUpdated(const QQmlChangeSet &changeSet, bool reset);

    virtual void syncWithPendingChanges();
    virtual void syncDelegate();
    virtual void syncDelegateModelAccess();
    virtual QVariant modelImpl() const;
    virtual void setModelImpl(const QVariant &newModel);
    virtual void syncModel();
    virtual void syncSyncView();
    virtual void syncPositionView();
    virtual QAbstractItemModel *selectionSourceModel();
    inline void syncRebuildOptions();

    void connectToModel();
    void disconnectFromModel();

    void rowsMovedCallback(const QModelIndex &parent, int start, int end, const QModelIndex &destination, int row);
    void columnsMovedCallback(const QModelIndex &parent, int start, int end, const QModelIndex &destination, int column);
    void rowsInsertedCallback(const QModelIndex &parent, int begin, int end);
    void rowsRemovedCallback(const QModelIndex &parent, int begin, int end);
    void columnsInsertedCallback(const QModelIndex &parent, int begin, int end);
    void columnsRemovedCallback(const QModelIndex &parent, int begin, int end);
    void layoutChangedCallback(const QList<QPersistentModelIndex> &parents, QAbstractItemModel::LayoutChangeHint hint);
    void modelResetCallback();
    bool compareModel(const QVariant& model1, const QVariant& model2) const;

    void positionViewAtRow(int row, Qt::Alignment alignment, qreal offset, const QRectF subRect = QRectF());
    void positionViewAtColumn(int column, Qt::Alignment alignment, qreal offset, const QRectF subRect = QRectF());
    bool scrollToRow(int row, Qt::Alignment alignment, qreal offset, const QRectF subRect = QRectF());
    bool scrollToColumn(int column, Qt::Alignment alignment, qreal offset, const QRectF subRect = QRectF());

    void scheduleRebuildIfFastFlick();
    void setLocalViewportX(qreal contentX);
    void setLocalViewportY(qreal contentY);
    void syncViewportRect();
    void syncViewportPosRecursive();

    bool selectedInSelectionModel(const QPoint &cell) const;
    void selectionChangedInSelectionModel(const QItemSelection &selected, const QItemSelection &deselected);
    void updateSelectedOnAllDelegateItems();
    void setSelectedOnDelegateItem(const QModelIndex &modelIndex, bool select);

    bool currentInSelectionModel(const QPoint &cell) const;
    void currentChangedInSelectionModel(const QModelIndex &current, const QModelIndex &previous);
    void setCurrentOnDelegateItem(const QModelIndex &index, bool isCurrent);
    void updateCurrentRowAndColumn();

    void fetchMoreData();

    void _q_componentFinalized();
    void registerCallbackWhenBindingsAreEvaluated();

    inline QString tableLayoutToString() const;
    void dumpTable() const;

    void setRequiredProperty(const char *property,
                             const QVariant &value,
                             int serializedModelIndex,
                             QObject *object, bool init);

    void handleTap(const QQuickHandlerPoint &point);
    void setCurrentIndexFromTap(const QPointF &pos);
    void setCurrentIndex(const QPoint &cell);
    bool setCurrentIndexFromKeyEvent(QKeyEvent *e);
    bool canEdit(const QModelIndex tappedIndex, bool warn);
    bool editFromKeyEvent(QKeyEvent *e);
    void closeEditorAndCommit();
    QObject *installEventFilterOnFocusObjectInsideEditItem();

    // QQuickSelectable
    QQuickItem *selectionPointerHandlerTarget() const override;
    bool hasSelection() const override;
    bool startSelection(const QPointF &pos, Qt::KeyboardModifiers modifiers) override;
    void setSelectionStartPos(const QPointF &pos) override;
    void setSelectionEndPos(const QPointF &pos) override;
    void clearSelection() override;
    void normalizeSelection() override;
    QRectF selectionRectangle() const override;
    QSizeF scrollTowardsPoint(const QPointF &pos, const QSizeF &step) override;
    void setCallback(std::function<void(CallBackFlag)> func) override;
    void cancelSelectionTracking();

    QPoint clampedCellAtPos(const QPointF &pos) const;
    virtual void updateSelection(const QRect &oldSelection, const QRect &newSelection);
    QRect selection() const;
    // ----------------

    // Section drag handler
#if QT_CONFIG(quick_draganddrop)
    void initSectionDragHandler(Qt::Orientation orientation);
    void destroySectionDragHandler();
#endif
    inline void setActivePointerHandler(QQuickTableViewPointerHandler *handler) { activePtrHandler = handler; }
    inline QQuickTableViewPointerHandler* activePointerHandler() const { return activePtrHandler; }
    // Row/Column reordering
    void moveSection(int source , int destination, Qt::Orientations orientation);
    void initializeIndexMapping();
    void clearIndexMapping();
    void clearSection(Qt::Orientations orientation);
    virtual int logicalRowIndex(const int visualIndex) const;
    virtual int logicalColumnIndex(const int visualIndex) const;
    virtual int visualRowIndex(const int logicalIndex) const;
    virtual int visualColumnIndex(const int logicalIndex) const;
    void setContainsDragOnDelegateItem(const QModelIndex &modelIndex, bool overlay);
    int getEditCellIndex(const QModelIndex &index) const;
};

class FxTableItem : public QQuickItemViewFxItem
{
public:
    FxTableItem(QQuickItem *item, QQuickTableView *table, bool own)
        : QQuickItemViewFxItem(item, own, QQuickTableViewPrivate::get(table))
    {
    }

    qreal position() const override { return 0; }
    qreal endPosition() const override { return 0; }
    qreal size() const override { return 0; }
    qreal sectionSize() const override { return 0; }
    bool contains(qreal, qreal) const override { return false; }

    QPoint cell;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QQuickTableViewPrivate::RebuildOptions)

QT_END_NAMESPACE

#endif
