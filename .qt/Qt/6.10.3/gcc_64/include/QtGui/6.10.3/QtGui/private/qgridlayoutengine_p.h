// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QGRIDLAYOUTENGINE_P_H
#define QGRIDLAYOUTENGINE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the graphics view layout classes.  This header
// file may change from version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/private/qtguiglobal_p.h>

#include <QtCore/qalgorithms.h>
#include <QtCore/qbitarray.h>
#include <QtCore/qlist.h>
#include <QtCore/qmap.h>
#include <QtCore/qsize.h>
#include <QtCore/qrect.h>
#include <QtCore/qdebug.h>

#include <float.h>
#include "qlayoutpolicy_p.h"
#include "qabstractlayoutstyleinfo_p.h"

// #define QGRIDLAYOUTENGINE_DEBUG

QT_BEGIN_NAMESPACE

class QStyle;
class QWidget;

// ### deal with Descent in a similar way
enum {
    MinimumSize = Qt::MinimumSize,
    PreferredSize = Qt::PreferredSize,
    MaximumSize = Qt::MaximumSize,
    NSizes
};

// do not reorder
enum LayoutSide {
    Left,
    Top,
    Right,
    Bottom
};

enum {
    NoConstraint,
    HorizontalConstraint,   // Width depends on the height
    VerticalConstraint,     // Height depends on the width
    UnknownConstraint,      // need to update cache
    UnfeasibleConstraint    // not feasible, it be has some items with Vertical and others with Horizontal constraints
};

/*
    Minimal container to store Qt::Orientation-discriminated values.

    The salient feature is the indexing operator, which takes
    Qt::Orientation (and assumes it's passed only Qt::Horizontal or Qt::Vertical).
*/
template <typename T>
class QHVContainer {
    T m_data[2];

    static_assert(Qt::Horizontal == 0x1);
    static_assert(Qt::Vertical == 0x2);
    static constexpr int map(Qt::Orientation o)
    {
        Q_ASSERT(o == Qt::Horizontal || o == Qt::Vertical); // Q_PRE
        return int(o) - 1;
    }
    static constexpr int mapOther(Qt::Orientation o)
    {
        Q_ASSERT(o == Qt::Horizontal || o == Qt::Vertical); // Q_PRE
        return 2 - int(o);
    }
public:
    constexpr QHVContainer(const T &h, const T &v)
            noexcept(std::is_nothrow_copy_constructible_v<T>)
        : m_data{h, v} {}
    QHVContainer() = default;

    constexpr T &operator[](Qt::Orientation o) { return m_data[map(o)]; }
    constexpr const T &operator[](Qt::Orientation o) const { return m_data[map(o)]; }

    constexpr T &other(Qt::Orientation o) { return m_data[mapOther(o)]; }
    constexpr const T &other(Qt::Orientation o) const { return m_data[mapOther(o)]; }

    constexpr void transpose() noexcept { qSwap(m_data[0], m_data[1]); }
    constexpr QHVContainer transposed() const
        noexcept(std::is_nothrow_copy_constructible_v<T>)
    { return {m_data[1], m_data[0]}; }
};

template <typename T>
class QLayoutParameter
{
public:
    enum State { Default, User, Cached };

    inline QLayoutParameter() : q_value(T()), q_state(Default) {}
    inline QLayoutParameter(T value, State state = Default) : q_value(value), q_state(state) {}

    inline void setUserValue(T value) {
        q_value = value;
        q_state = User;
    }
    inline void setCachedValue(T value) const {
        if (q_state != User) {
            q_value = value;
            q_state = Cached;
        }
    }
    inline T value() const { return q_value; }
    inline T value(T defaultValue) const { return isUser() ? q_value : defaultValue; }
    inline bool isDefault() const { return q_state == Default; }
    inline bool isUser() const { return q_state == User; }
    inline bool isCached() const { return q_state == Cached; }

private:
    mutable T q_value;
    mutable State q_state;
};

class QStretchParameter : public QLayoutParameter<int>
{
public:
    QStretchParameter() : QLayoutParameter<int>(-1) {}

};

class Q_GUI_EXPORT QGridLayoutBox
{
public:
    inline QGridLayoutBox()
        : q_minimumSize(0), q_preferredSize(0), q_maximumSize(FLT_MAX),
          q_minimumDescent(-1), q_minimumAscent(-1) {}

    void add(const QGridLayoutBox &other, int stretch, qreal spacing);
    void combine(const QGridLayoutBox &other);
    void normalize();

#ifdef QGRIDLAYOUTENGINE_DEBUG
    void dump(int indent = 0) const;
#endif
    // This code could use the union-struct-array trick, but a compiler
    // bug prevents this from working.
    qreal q_minimumSize;
    qreal q_preferredSize;
    qreal q_maximumSize;
    qreal q_minimumDescent;
    qreal q_minimumAscent;
    inline qreal &q_sizes(int which)
    {
        return const_cast<qreal&>(static_cast<const QGridLayoutBox*>(this)->q_sizes(which));
    }
    inline const qreal &q_sizes(int which) const
    {
        switch (which) {
        case Qt::MinimumSize:
            return q_minimumSize;
        case Qt::PreferredSize:
            return q_preferredSize;
        case Qt::MaximumSize:
            return q_maximumSize;
        case Qt::MinimumDescent:
            return q_minimumDescent;
        case (Qt::MinimumDescent + 1):
            return q_minimumAscent;
        default:
            Q_UNREACHABLE();
        }
    }
};
Q_DECLARE_TYPEINFO(QGridLayoutBox, Q_RELOCATABLE_TYPE); // cannot be Q_PRIMITIVE_TYPE, as q_maximumSize, say, is != 0

bool operator==(const QGridLayoutBox &box1, const QGridLayoutBox &box2);
inline bool operator!=(const QGridLayoutBox &box1, const QGridLayoutBox &box2)
    { return !operator==(box1, box2); }

class QGridLayoutMultiCellData
{
public:
    inline QGridLayoutMultiCellData() : q_stretch(-1) {}

    QGridLayoutBox q_box;
    int q_stretch;
};

typedef QMap<std::pair<int, int>, QGridLayoutMultiCellData> MultiCellMap;

class QGridLayoutRowInfo;

class QGridLayoutRowData
{
public:
    void reset(int count);
    void distributeMultiCells(const QGridLayoutRowInfo &rowInfo, bool snapToPixelGrid);
    void calculateGeometries(int start, int end, qreal targetSize, qreal *positions, qreal *sizes,
                             qreal *descents, const QGridLayoutBox &totalBox,
                             const QGridLayoutRowInfo &rowInfo, bool snapToPixelGrid);
    QGridLayoutBox totalBox(int start, int end) const;
    void stealBox(int start, int end, int which, qreal *positions, qreal *sizes);

#ifdef QGRIDLAYOUTENGINE_DEBUG
    void dump(int indent = 0) const;
#endif

    QBitArray ignore;   // ### rename q_
    QList<QGridLayoutBox> boxes;
    MultiCellMap multiCellMap;
    QList<int> stretches;
    QList<qreal> spacings;
    bool hasIgnoreFlag;
};

class QGridLayoutRowInfo
{
public:
    inline QGridLayoutRowInfo() : count(0) {}

    void insertOrRemoveRows(int row, int delta);

#ifdef QGRIDLAYOUTENGINE_DEBUG
    void dump(int indent = 0) const;
#endif

    int count;
    QList<QStretchParameter> stretches;
    QList<QLayoutParameter<qreal>> spacings;
    QList<Qt::Alignment> alignments;
    QList<QGridLayoutBox> boxes;
};


class Q_GUI_EXPORT QGridLayoutItem
{
public:
    QGridLayoutItem(int row, int column, int rowSpan = 1, int columnSpan = 1,
                    Qt::Alignment alignment = { });
    virtual ~QGridLayoutItem() {}

    inline int firstRow() const { return q_firstRows[Qt::Vertical]; }
    inline int firstColumn() const { return q_firstRows[Qt::Horizontal]; }
    inline int rowSpan() const { return q_rowSpans[Qt::Vertical]; }
    inline int columnSpan() const { return q_rowSpans[Qt::Horizontal]; }
    inline int lastRow() const { return firstRow() + rowSpan() - 1; }
    inline int lastColumn() const { return firstColumn() + columnSpan() - 1; }

    int firstRow(Qt::Orientation orientation) const;
    int firstColumn(Qt::Orientation orientation) const;
    int lastRow(Qt::Orientation orientation) const;
    int lastColumn(Qt::Orientation orientation) const;
    int rowSpan(Qt::Orientation orientation) const;
    int columnSpan(Qt::Orientation orientation) const;
    void setFirstRow(int row, Qt::Orientation orientation = Qt::Vertical);
    void setRowSpan(int rowSpan, Qt::Orientation orientation = Qt::Vertical);

    int stretchFactor(Qt::Orientation orientation) const;
    void setStretchFactor(int stretch, Qt::Orientation orientation);

    inline Qt::Alignment alignment() const { return q_alignment; }
    inline void setAlignment(Qt::Alignment alignment) { q_alignment = alignment; }

    virtual QLayoutPolicy::Policy sizePolicy(Qt::Orientation orientation) const = 0;
    virtual QSizeF sizeHint(Qt::SizeHint which, const QSizeF &constraint) const = 0;
    virtual bool isEmpty() const { return false; }

    virtual void setGeometry(const QRectF &rect) = 0;
    /*
      returns true if the size policy returns true for either hasHeightForWidth()
      or hasWidthForHeight()
     */
    virtual bool hasDynamicConstraint() const { return false; }
    virtual Qt::Orientation dynamicConstraintOrientation() const { return Qt::Horizontal; }


    virtual QLayoutPolicy::ControlTypes controlTypes(LayoutSide side) const;

    inline virtual QString toString() const { return QDebug::toString(this); }

    QRectF geometryWithin(qreal x, qreal y, qreal width, qreal height, qreal rowDescent, Qt::Alignment align, bool snapToPixelGrid) const;
    QGridLayoutBox box(Qt::Orientation orientation, bool snapToPixelGrid, qreal constraint = -1.0) const;


    void transpose();
    void insertOrRemoveRows(int row, int delta, Qt::Orientation orientation = Qt::Vertical);
    QSizeF effectiveMaxSize(const QSizeF &constraint) const;

#ifdef QGRIDLAYOUTENGINE_DEBUG
    void dump(int indent = 0) const;
#endif

private:
    QHVContainer<int> q_firstRows;
    QHVContainer<int> q_rowSpans;
    QHVContainer<int> q_stretches;
    Qt::Alignment q_alignment;

};

class Q_GUI_EXPORT QGridLayoutEngine
{
public:
    QGridLayoutEngine(Qt::Alignment defaultAlignment = { }, bool snapToPixelGrid = false);
    inline ~QGridLayoutEngine() { qDeleteAll(q_items); }

    int rowCount(Qt::Orientation orientation) const;
    int columnCount(Qt::Orientation orientation) const;
    inline int rowCount() const { return q_infos[Qt::Vertical].count; }
    inline int columnCount() const { return q_infos[Qt::Horizontal].count; }
    // returns the number of items inserted, which may be less than (rowCount * columnCount)
    int itemCount() const;
    QGridLayoutItem *itemAt(int index) const;

    int effectiveFirstRow(Qt::Orientation orientation = Qt::Vertical) const;
    int effectiveLastRow(Qt::Orientation orientation = Qt::Vertical) const;

    void setSpacing(qreal spacing, Qt::Orientations orientations);
    qreal spacing(Qt::Orientation orientation, const QAbstractLayoutStyleInfo *styleInfo) const;
    // ### setSpacingAfterRow(), spacingAfterRow()
    void setRowSpacing(int row, qreal spacing, Qt::Orientation orientation = Qt::Vertical);
    qreal rowSpacing(int row, Qt::Orientation orientation = Qt::Vertical) const;

    void setRowStretchFactor(int row, int stretch, Qt::Orientation orientation = Qt::Vertical);
    int rowStretchFactor(int row, Qt::Orientation orientation = Qt::Vertical) const;

    void setRowSizeHint(Qt::SizeHint which, int row, qreal size,
                        Qt::Orientation orientation = Qt::Vertical);
    qreal rowSizeHint(Qt::SizeHint which, int row,
                      Qt::Orientation orientation = Qt::Vertical) const;

    bool uniformCellWidths() const;
    void setUniformCellWidths(bool uniformCellWidths);

    bool uniformCellHeights() const;
    void setUniformCellHeights(bool uniformCellHeights);

    void setRowAlignment(int row, Qt::Alignment alignment, Qt::Orientation orientation);
    Qt::Alignment rowAlignment(int row, Qt::Orientation orientation) const;

    Qt::Alignment effectiveAlignment(const QGridLayoutItem *layoutItem) const;


    void insertItem(QGridLayoutItem *item, int index);
    void addItem(QGridLayoutItem *item);
    void removeItem(QGridLayoutItem *item);
    void deleteItems()
    {
        const QList<QGridLayoutItem *> oldItems = q_items;
        q_items.clear();    // q_items are used as input when the grid is regenerated in removeRows
        // The following calls to removeRows are suboptimal
        int rows = rowCount(Qt::Vertical);
        removeRows(0, rows, Qt::Vertical);
        rows = rowCount(Qt::Horizontal);
        removeRows(0, rows, Qt::Horizontal);
        qDeleteAll(oldItems);
    }

    QGridLayoutItem *itemAt(int row, int column, Qt::Orientation orientation = Qt::Vertical) const;
    inline void insertRow(int row, Qt::Orientation orientation = Qt::Vertical)
        { insertOrRemoveRows(row, +1, orientation); }
    inline void removeRows(int row, int count, Qt::Orientation orientation)
        { insertOrRemoveRows(row, -count, orientation); }

    void invalidate();
    void setGeometries(const QRectF &contentsGeometry, const QAbstractLayoutStyleInfo *styleInfo);
    QRectF cellRect(const QRectF &contentsGeometry, int row, int column, int rowSpan, int columnSpan,
                    const QAbstractLayoutStyleInfo *styleInfo) const;
    QSizeF sizeHint(Qt::SizeHint which, const QSizeF &constraint,
                    const QAbstractLayoutStyleInfo *styleInfo) const;

    // heightForWidth / widthForHeight support
    QSizeF dynamicallyConstrainedSizeHint(Qt::SizeHint which, const QSizeF &constraint) const;
    bool ensureDynamicConstraint() const;
    bool hasDynamicConstraint() const;
    Qt::Orientation constraintOrientation() const;


    QLayoutPolicy::ControlTypes controlTypes(LayoutSide side) const;
    void transpose();
    void setVisualDirection(Qt::LayoutDirection direction);
    Qt::LayoutDirection visualDirection() const;
#ifdef QGRIDLAYOUTENGINE_DEBUG
    void dump(int indent = 0) const;
#endif

private:
    static int grossRoundUp(int n) { return ((n + 2) | 0x3) - 2; }

    void maybeExpandGrid(int row, int column, Qt::Orientation orientation = Qt::Vertical);
    void regenerateGrid();
    inline int internalGridRowCount() const { return grossRoundUp(rowCount()); }
    inline int internalGridColumnCount() const { return grossRoundUp(columnCount()); }
    void setItemAt(int row, int column, QGridLayoutItem *item);
    void insertOrRemoveRows(int row, int delta, Qt::Orientation orientation = Qt::Vertical);
    void fillRowData(QGridLayoutRowData *rowData,
                     const qreal *colPositions, const qreal *colSizes,
                     Qt::Orientation orientation,
                     const QAbstractLayoutStyleInfo *styleInfo) const;
    void ensureEffectiveFirstAndLastRows() const;
    void ensureColumnAndRowData(QGridLayoutRowData *rowData, QGridLayoutBox *totalBox,
                                            const qreal *colPositions, const qreal *colSizes,
                                            Qt::Orientation orientation,
                                            const QAbstractLayoutStyleInfo *styleInfo) const;

    void ensureGeometries(const QSizeF &size, const QAbstractLayoutStyleInfo *styleInfo) const;
protected:
    QList<QGridLayoutItem *> q_items;
private:
    // User input
    QList<QGridLayoutItem *> q_grid;
    QHVContainer<QLayoutParameter<qreal>> q_defaultSpacings;
    QHVContainer<QGridLayoutRowInfo> q_infos;
    Qt::LayoutDirection m_visualDirection;

    // Configuration
    Qt::Alignment m_defaultAlignment;
    unsigned m_snapToPixelGrid : 1;
    unsigned m_uniformCellWidths : 1;
    unsigned m_uniformCellHeights : 1;

    // Lazily computed from the above user input
    mutable QHVContainer<int> q_cachedEffectiveFirstRows;
    mutable QHVContainer<int> q_cachedEffectiveLastRows;
    mutable quint8 q_cachedConstraintOrientation : 3;

    // this is useful to cache
    mutable QHVContainer<QGridLayoutBox> q_totalBoxes;
    enum {
        NotCached = -2,             // Cache is empty. Happens when the engine is invalidated.
        CachedWithNoConstraint = -1 // cache has a totalBox without any HFW/WFH constraints.
        // >= 0                     // cache has a totalBox with this specific constraint.
    };
    mutable QHVContainer<qreal> q_totalBoxCachedConstraints;   // holds the constraint used for the cached totalBox

    // Layout item input
    mutable QGridLayoutRowData q_columnData;
    mutable QGridLayoutRowData q_rowData;

    // Output
    mutable QSizeF q_cachedSize;
    mutable QList<qreal> q_xx;
    mutable QList<qreal> q_yy;
    mutable QList<qreal> q_widths;
    mutable QList<qreal> q_heights;
    mutable QList<qreal> q_descents;

    friend class QGridLayoutItem;
};

QT_END_NAMESPACE

#endif
