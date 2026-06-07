// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKTABLEVIEW_P_H
#define QQUICKTABLEVIEW_P_H

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

#include <private/qtquickglobal_p.h>
QT_REQUIRE_CONFIG(quick_tableview);

#include <QtCore/qpointer.h>
#include <QtCore/qpoint.h>
#include <QtCore/qrect.h>
#include <QtQuick/private/qtquickglobal_p.h>
#include <QtQuick/private/qquickflickable_p.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQml/private/qqmlnullablevalue_p.h>
#include <QtQml/private/qqmlfinalizer_p.h>
#include <QtQml/private/qqmlguard_p.h>
#include <QtQmlModels/private/qqmldelegatemodel_p.h>

QT_BEGIN_NAMESPACE

class QQuickTableViewAttached;
class QQuickTableViewPrivate;
class QItemSelectionModel;

class Q_QUICK_EXPORT QQuickTableView : public QQuickFlickable, public QQmlFinalizerHook
{
    Q_OBJECT
    Q_INTERFACES(QQmlFinalizerHook)

    Q_PROPERTY(int rows READ rows NOTIFY rowsChanged)
    Q_PROPERTY(int columns READ columns NOTIFY columnsChanged)
    Q_PROPERTY(qreal rowSpacing READ rowSpacing WRITE setRowSpacing NOTIFY rowSpacingChanged)
    Q_PROPERTY(qreal columnSpacing READ columnSpacing WRITE setColumnSpacing NOTIFY columnSpacingChanged)
    Q_PROPERTY(QJSValue rowHeightProvider READ rowHeightProvider WRITE setRowHeightProvider NOTIFY rowHeightProviderChanged)
    Q_PROPERTY(QJSValue columnWidthProvider READ columnWidthProvider WRITE setColumnWidthProvider NOTIFY columnWidthProviderChanged)
    Q_PROPERTY(QVariant model READ model WRITE setModel NOTIFY modelChanged)
    Q_PROPERTY(QQmlComponent *delegate READ delegate WRITE setDelegate NOTIFY delegateChanged)
    Q_PROPERTY(bool reuseItems READ reuseItems WRITE setReuseItems NOTIFY reuseItemsChanged)
    Q_PROPERTY(qreal contentWidth READ contentWidth WRITE setContentWidth NOTIFY contentWidthChanged)
    Q_PROPERTY(qreal contentHeight READ contentHeight WRITE setContentHeight NOTIFY contentHeightChanged)
    Q_PROPERTY(QQuickTableView *syncView READ syncView WRITE setSyncView NOTIFY syncViewChanged REVISION(2, 14))
    Q_PROPERTY(Qt::Orientations syncDirection READ syncDirection WRITE setSyncDirection NOTIFY syncDirectionChanged REVISION(2, 14))
    Q_PROPERTY(int leftColumn READ leftColumn NOTIFY leftColumnChanged REVISION(6, 0))
    Q_PROPERTY(int rightColumn READ rightColumn NOTIFY rightColumnChanged REVISION(6, 0))
    Q_PROPERTY(int topRow READ topRow NOTIFY topRowChanged REVISION(6, 0))
    Q_PROPERTY(int bottomRow READ bottomRow NOTIFY bottomRowChanged REVISION(6, 0))
    Q_PROPERTY(QItemSelectionModel *selectionModel READ selectionModel WRITE setSelectionModel NOTIFY selectionModelChanged REVISION(6, 2))
    Q_PROPERTY(bool animate READ animate WRITE setAnimate NOTIFY animateChanged REVISION(6, 4))
    Q_PROPERTY(bool keyNavigationEnabled READ keyNavigationEnabled WRITE setKeyNavigationEnabled NOTIFY keyNavigationEnabledChanged REVISION(6, 4))
    Q_PROPERTY(bool pointerNavigationEnabled READ pointerNavigationEnabled WRITE setPointerNavigationEnabled NOTIFY pointerNavigationEnabledChanged REVISION(6, 4))
    Q_PROPERTY(int currentRow READ currentRow NOTIFY currentRowChanged REVISION(6, 4) FINAL)
    Q_PROPERTY(int currentColumn READ currentColumn NOTIFY currentColumnChanged REVISION(6, 4) FINAL)
    Q_PROPERTY(bool alternatingRows READ alternatingRows WRITE setAlternatingRows NOTIFY alternatingRowsChanged REVISION(6, 4) FINAL)
    Q_PROPERTY(SelectionBehavior selectionBehavior READ selectionBehavior WRITE setSelectionBehavior NOTIFY selectionBehaviorChanged REVISION(6, 4) FINAL)
    Q_PROPERTY(bool resizableColumns READ resizableColumns WRITE setResizableColumns NOTIFY resizableColumnsChanged REVISION(6, 5) FINAL)
    Q_PROPERTY(bool resizableRows READ resizableRows WRITE setResizableRows NOTIFY resizableRowsChanged REVISION(6, 5) FINAL)
    Q_PROPERTY(EditTriggers editTriggers READ editTriggers WRITE setEditTriggers NOTIFY editTriggersChanged REVISION(6, 5) FINAL)
    Q_PROPERTY(SelectionMode selectionMode READ selectionMode WRITE setSelectionMode NOTIFY selectionModeChanged REVISION(6, 6) FINAL)
    Q_PROPERTY(QQmlDelegateModel::DelegateModelAccess delegateModelAccess READ delegateModelAccess
            WRITE setDelegateModelAccess NOTIFY delegateModelAccessChanged REVISION(6, 10) FINAL)

    QML_NAMED_ELEMENT(TableView)
    QML_ADDED_IN_VERSION(2, 12)
    QML_ATTACHED(QQuickTableViewAttached)

public:
    enum PositionModeFlag {
        AlignLeft = Qt::AlignLeft,
        AlignRight = Qt::AlignRight,
        AlignHCenter = Qt::AlignHCenter,
        AlignTop = Qt::AlignTop,
        AlignBottom = Qt::AlignBottom,
        AlignVCenter = Qt::AlignVCenter,
        AlignCenter = AlignVCenter | AlignHCenter,
        Visible = 0x01000,
        Contain = 0x02000
    };
    Q_DECLARE_FLAGS(PositionMode, PositionModeFlag)
    Q_FLAG(PositionMode)

    enum SelectionBehavior {
        SelectionDisabled,
        SelectCells,
        SelectRows,
        SelectColumns
    };
    Q_ENUM(SelectionBehavior)

    enum SelectionMode {
        SingleSelection,
        ContiguousSelection,
        ExtendedSelection
    };
    Q_ENUM(SelectionMode)

    enum EditTrigger {
        NoEditTriggers = 0x0,
        SingleTapped = 0x1,
        DoubleTapped = 0x2,
        SelectedTapped = 0x4,
        EditKeyPressed = 0x8,
        AnyKeyPressed = 0x10,
    };
    Q_DECLARE_FLAGS(EditTriggers, EditTrigger)
    Q_FLAG(EditTriggers)

    QQuickTableView(QQuickItem *parent = nullptr);
    ~QQuickTableView() override;
    int rows() const;
    int columns() const;

    qreal rowSpacing() const;
    void setRowSpacing(qreal spacing);

    qreal columnSpacing() const;
    void setColumnSpacing(qreal spacing);

    QJSValue rowHeightProvider() const;
    void setRowHeightProvider(const QJSValue &provider);

    QJSValue columnWidthProvider() const;
    void setColumnWidthProvider(const QJSValue &provider);

    QVariant model() const;
    void setModel(const QVariant &newModel);

    QQmlComponent *delegate() const;
    void setDelegate(QQmlComponent *);

    bool reuseItems() const;
    void setReuseItems(bool reuseItems);

    void setContentWidth(qreal width);
    void setContentHeight(qreal height);

    QQuickTableView *syncView() const;
    void setSyncView(QQuickTableView *view);

    Qt::Orientations syncDirection() const;
    void setSyncDirection(Qt::Orientations direction);

    QItemSelectionModel *selectionModel() const;
    void setSelectionModel(QItemSelectionModel *selectionModel);

    bool animate() const;
    void setAnimate(bool animate);

    bool keyNavigationEnabled() const;
    void setKeyNavigationEnabled(bool enabled);
    bool pointerNavigationEnabled() const;
    void setPointerNavigationEnabled(bool enabled);

    int leftColumn() const;
    int rightColumn() const;
    int topRow() const;
    int bottomRow() const;

    int currentRow() const;
    int currentColumn() const;

    bool alternatingRows() const;
    void setAlternatingRows(bool alternatingRows);

    SelectionBehavior selectionBehavior() const;
    void setSelectionBehavior(SelectionBehavior selectionBehavior);
    SelectionMode selectionMode() const;
    void setSelectionMode(SelectionMode selectionMode);

    bool resizableColumns() const;
    void setResizableColumns(bool enabled);

    bool resizableRows() const;
    void setResizableRows(bool enabled);

    EditTriggers editTriggers() const;
    void setEditTriggers(EditTriggers editTriggers);

    QQmlDelegateModel::DelegateModelAccess delegateModelAccess() const;
    void setDelegateModelAccess(QQmlDelegateModel::DelegateModelAccess delegateModelAccess);

    Q_INVOKABLE void forceLayout();
    Q_INVOKABLE void positionViewAtCell(const QPoint &cell, QQuickTableView::PositionMode mode, const QPointF &offset = QPointF(), const QRectF &subRect = QRectF());
    Q_INVOKABLE void positionViewAtIndex(const QModelIndex &index, QQuickTableView::PositionMode mode, const QPointF &offset = QPointF(), const QRectF &subRect = QRectF());
    Q_INVOKABLE void positionViewAtRow(int row, QQuickTableView::PositionMode mode, qreal offset = 0, const QRectF &subRect = QRectF());
    Q_INVOKABLE void positionViewAtColumn(int column, QQuickTableView::PositionMode mode, qreal offset = 0, const QRectF &subRect = QRectF());
    Q_INVOKABLE QQuickItem *itemAtCell(const QPoint &cell) const;

    Q_REVISION(6, 4) Q_INVOKABLE QPoint cellAtPosition(const QPointF &position, bool includeSpacing = false) const;
    Q_REVISION(6, 4) Q_INVOKABLE QPoint cellAtPosition(qreal x, qreal y, bool includeSpacing = false) const;
#if QT_DEPRECATED_SINCE(6, 4)
    QT_DEPRECATED_VERSION_X_6_4("Use index(row, column) instead")
    Q_REVISION(6, 4) Q_INVOKABLE virtual QModelIndex modelIndex(int row, int column) const;
    QT_DEPRECATED_VERSION_X_6_4("Use cellAtPosition() instead")
    Q_INVOKABLE QPoint cellAtPos(const QPointF &position, bool includeSpacing = false) const;
    Q_INVOKABLE QPoint cellAtPos(qreal x, qreal y, bool includeSpacing = false) const;
#endif

    Q_REVISION(6, 2) Q_INVOKABLE bool isColumnLoaded(int column) const;
    Q_REVISION(6, 2) Q_INVOKABLE bool isRowLoaded(int row) const;

    Q_REVISION(6, 2) Q_INVOKABLE qreal columnWidth(int column) const;
    Q_REVISION(6, 2) Q_INVOKABLE qreal rowHeight(int row) const;
    Q_REVISION(6, 2) Q_INVOKABLE qreal implicitColumnWidth(int column) const;
    Q_REVISION(6, 2) Q_INVOKABLE qreal implicitRowHeight(int row) const;

    Q_REVISION(6, 4) Q_INVOKABLE QModelIndex index(int row, int column) const;
    Q_REVISION(6, 4) Q_INVOKABLE virtual QModelIndex modelIndex(const QPoint &cell) const;
    Q_REVISION(6, 4) Q_INVOKABLE virtual QPoint cellAtIndex(const QModelIndex &index) const;
    Q_REVISION(6, 4) Q_INVOKABLE int rowAtIndex(const QModelIndex &index) const;
    Q_REVISION(6, 4) Q_INVOKABLE int columnAtIndex(const QModelIndex &index) const;

    Q_REVISION(6, 5) Q_INVOKABLE void setColumnWidth(int column, qreal size);
    Q_REVISION(6, 5) Q_INVOKABLE void clearColumnWidths();
    Q_REVISION(6, 5) Q_INVOKABLE qreal explicitColumnWidth(int column) const;

    Q_REVISION(6, 5) Q_INVOKABLE void setRowHeight(int row, qreal size);
    Q_REVISION(6, 5) Q_INVOKABLE void clearRowHeights();
    Q_REVISION(6, 5) Q_INVOKABLE qreal explicitRowHeight(int row) const;

    Q_REVISION(6, 5) Q_INVOKABLE void edit(const QModelIndex &index);
    Q_REVISION(6, 5) Q_INVOKABLE void closeEditor();

    Q_REVISION(6, 5) Q_INVOKABLE QQuickItem *itemAtIndex(const QModelIndex &index) const;

#if QT_DEPRECATED_SINCE(6, 5)
    QT_DEPRECATED_VERSION_X_6_5("Use itemAtIndex(index(row, column)) instead")
    Q_INVOKABLE QQuickItem *itemAtCell(int column, int row) const;
    QT_DEPRECATED_VERSION_X_6_5("Use positionViewAtIndex(index(row, column)) instead")
    Q_INVOKABLE void positionViewAtCell(int column, int row, QQuickTableView::PositionMode mode, const QPointF &offset = QPointF(), const QRectF &subRect = QRectF());
#endif

    Q_REVISION(6, 8) Q_INVOKABLE void moveColumn(int source, int destination);
    Q_REVISION(6, 8) Q_INVOKABLE void moveRow(int source, int destination);
    Q_REVISION(6, 8) Q_INVOKABLE void clearColumnReordering();
    Q_REVISION(6, 8) Q_INVOKABLE void clearRowReordering();

    static QQuickTableViewAttached *qmlAttachedProperties(QObject *);

Q_SIGNALS:
    void rowsChanged();
    void columnsChanged();
    void rowSpacingChanged();
    void columnSpacingChanged();
    void rowHeightProviderChanged();
    void columnWidthProviderChanged();
    void modelChanged();
    void delegateChanged();
    void reuseItemsChanged();
    Q_REVISION(2, 14) void syncViewChanged();
    Q_REVISION(2, 14) void syncDirectionChanged();
    Q_REVISION(6, 0) void leftColumnChanged();
    Q_REVISION(6, 0) void rightColumnChanged();
    Q_REVISION(6, 0) void topRowChanged();
    Q_REVISION(6, 0) void bottomRowChanged();
    Q_REVISION(6, 2) void selectionModelChanged();
    Q_REVISION(6, 4) void animateChanged();
    Q_REVISION(6, 4) void keyNavigationEnabledChanged();
    Q_REVISION(6, 4) void pointerNavigationEnabledChanged();
    Q_REVISION(6, 4) void currentRowChanged();
    Q_REVISION(6, 4) void currentColumnChanged();
    Q_REVISION(6, 4) void alternatingRowsChanged();
    Q_REVISION(6, 4) void selectionBehaviorChanged();
    Q_REVISION(6, 5) void resizableColumnsChanged();
    Q_REVISION(6, 5) void resizableRowsChanged();
    Q_REVISION(6, 5) void editTriggersChanged();
    Q_REVISION(6, 5) void layoutChanged();
    Q_REVISION(6, 6) void selectionModeChanged();
    Q_REVISION(6, 8) void rowMoved(int logicalIndex, int oldVisualIndex, int newVisualIndex);
    Q_REVISION(6, 8) void columnMoved(int logicalIndex, int oldVisualIndex, int newVisualIndex);
    Q_REVISION(6, 10) void delegateModelAccessChanged();

protected:
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    void viewportMoved(Qt::Orientations orientation) override;
    void keyPressEvent(QKeyEvent *e) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

protected:
    QQuickTableView(QQuickTableViewPrivate &dd, QQuickItem *parent);
    // QQmlFinalizerHook interface
    void componentFinalized() override;

private:
    Q_DISABLE_COPY(QQuickTableView)
    Q_DECLARE_PRIVATE(QQuickTableView)

    qreal minXExtent() const override;
    qreal maxXExtent() const override;
    qreal minYExtent() const override;
    qreal maxYExtent() const override;
};

class Q_QUICK_EXPORT QQuickTableViewAttached : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQuickTableView *view READ view NOTIFY viewChanged FINAL)
    Q_PROPERTY(QQmlComponent *editDelegate READ editDelegate WRITE setEditDelegate NOTIFY editDelegateChanged FINAL)

public:
    QQuickTableViewAttached(QObject *parent);

    QQuickTableView *view() const { return m_view; }
    void setView(QQuickTableView *newTableView) {
        if (newTableView == m_view)
            return;
        m_view = newTableView;
        Q_EMIT viewChanged();
    }

    QQmlComponent *editDelegate() const { return m_editDelegate; }
    void setEditDelegate(QQmlComponent *newEditDelegate)
    {
        if (m_editDelegate == newEditDelegate)
            return;
        m_editDelegate = newEditDelegate;
        Q_EMIT editDelegateChanged();
    }

Q_SIGNALS:
    void viewChanged();
    void pooled();
    void reused();
    void editDelegateChanged();
    void commit();

private:
    QPointer<QQuickTableView> m_view;
    QQmlGuard<QQmlComponent> m_editDelegate;

    friend class QQuickTableViewPrivate;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QQuickTableView::PositionMode)
Q_DECLARE_OPERATORS_FOR_FLAGS(QQuickTableView::EditTriggers)

QT_END_NAMESPACE

#endif // QQUICKTABLEVIEW_P_H
