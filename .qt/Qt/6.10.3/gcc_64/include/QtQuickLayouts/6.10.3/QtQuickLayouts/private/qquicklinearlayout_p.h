// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKLINEARLAYOUT_P_H
#define QQUICKLINEARLAYOUT_P_H

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

#include <QtQuickLayouts/private/qquicklayoutglobal_p.h>
#include "qquicklayout_p.h"
#include "qquickgridlayoutengine_p.h"

QT_BEGIN_NAMESPACE

/**********************************
 **
 ** QQuickGridLayoutBase
 **
 **/
class QQuickGridLayoutBasePrivate;

class Q_QUICKLAYOUTS_EXPORT QQuickGridLayoutBase : public QQuickLayout
{
    Q_OBJECT

    Q_PROPERTY(Qt::LayoutDirection layoutDirection READ layoutDirection WRITE setLayoutDirection
               NOTIFY layoutDirectionChanged REVISION(1, 1))
    QML_ANONYMOUS
    QML_ADDED_IN_VERSION(1, 1)

public:

    QQuickGridLayoutBase();

    explicit QQuickGridLayoutBase(QQuickGridLayoutBasePrivate &dd,
                                  Qt::Orientation orientation,
                                  QQuickItem *parent = nullptr);

    ~QQuickGridLayoutBase();
    void componentComplete() override;
    void invalidate(QQuickItem *childItem = nullptr) override;
    Qt::Orientation orientation() const;
    void setOrientation(Qt::Orientation orientation);
    QSizeF sizeHint(Qt::SizeHint whichSizeHint) const override;
    Qt::LayoutDirection layoutDirection() const;
    void setLayoutDirection(Qt::LayoutDirection dir);
    Qt::LayoutDirection effectiveLayoutDirection() const;
    void setAlignment(QQuickItem *item, Qt::Alignment align) override;
    void setStretchFactor(QQuickItem *item, int stretchFactor, Qt::Orientation orient) override;

    /* QQuickItemChangeListener */
    void itemDestroyed(QQuickItem *item) override;
    void itemVisibilityChanged(QQuickItem *item) override;

protected:
    void updateLayoutItems() override;
    QQuickItem *itemAt(int index) const override;
    int itemCount() const override;

    void rearrange(const QSizeF &size) override;
    virtual void insertLayoutItems() {}

Q_SIGNALS:
    Q_REVISION(1, 1) void layoutDirectionChanged();

private:
    void removeGridItem(QGridLayoutItem *gridItem);
    Q_DECLARE_PRIVATE(QQuickGridLayoutBase)
};

class QQuickLayoutStyleInfo;

class QQuickGridLayoutBasePrivate : public QQuickLayoutPrivate
{
    Q_DECLARE_PUBLIC(QQuickGridLayoutBase)

public:
    QQuickGridLayoutBasePrivate() : m_recurRearrangeCounter(0)
                                    , m_rearranging(false)
                                    , m_layoutDirection(Qt::LeftToRight)
                                    {}

    void mirrorChange() override
    {
        Q_Q(QQuickGridLayoutBase);
        q->invalidate();
    }

    QQuickGridLayoutEngine engine;
    Qt::Orientation orientation;
    unsigned m_recurRearrangeCounter : 2;
    unsigned m_rearranging : 1;
    QVector<QQuickItem *> m_invalidateAfterRearrange;
    Qt::LayoutDirection m_layoutDirection : 2;

    QQuickLayoutStyleInfo *styleInfo;
};

/**********************************
 **
 ** QQuickGridLayout
 **
 **/
class QQuickGridLayoutPrivate;
class Q_QUICKLAYOUTS_EXPORT QQuickGridLayout : public QQuickGridLayoutBase
{
    Q_OBJECT

    Q_PROPERTY(qreal columnSpacing READ columnSpacing WRITE setColumnSpacing NOTIFY columnSpacingChanged)
    Q_PROPERTY(qreal rowSpacing READ rowSpacing WRITE setRowSpacing NOTIFY rowSpacingChanged)
    Q_PROPERTY(int columns READ columns WRITE setColumns NOTIFY columnsChanged)
    Q_PROPERTY(int rows READ rows WRITE setRows NOTIFY rowsChanged)
    Q_PROPERTY(Flow flow READ flow WRITE setFlow NOTIFY flowChanged)
    Q_PROPERTY(bool uniformCellWidths READ uniformCellWidths WRITE setUniformCellWidths
               NOTIFY uniformCellWidthsChanged REVISION(6, 6) FINAL)
    Q_PROPERTY(bool uniformCellHeights READ uniformCellHeights WRITE setUniformCellHeights
               NOTIFY uniformCellHeightsChanged REVISION(6, 6) FINAL)

    QML_NAMED_ELEMENT(GridLayout)
    QML_ADDED_IN_VERSION(1, 0)
public:
    explicit QQuickGridLayout(QQuickItem *parent = nullptr);
    qreal columnSpacing() const;
    void setColumnSpacing(qreal spacing);
    qreal rowSpacing() const;
    void setRowSpacing(qreal spacing);

    int columns() const;
    void setColumns(int columns);
    int rows() const;
    void setRows(int rows);

    enum Flow { LeftToRight, TopToBottom };
    Q_ENUM(Flow)
    Flow flow() const;
    void setFlow(Flow flow);

    bool uniformCellWidths() const;
    void setUniformCellWidths(bool uniformCellWidths);
    bool uniformCellHeights() const;
    void setUniformCellHeights(bool uniformCellHeights);

    void insertLayoutItems() override;

Q_SIGNALS:
    void columnSpacingChanged();
    void rowSpacingChanged();

    void columnsChanged();
    void rowsChanged();

    void flowChanged();

    Q_REVISION(6, 6) void uniformCellWidthsChanged();
    Q_REVISION(6, 6) void uniformCellHeightsChanged();
private:
    Q_DECLARE_PRIVATE(QQuickGridLayout)
};

class QQuickGridLayoutPrivate : public QQuickGridLayoutBasePrivate
{
    Q_DECLARE_PUBLIC(QQuickGridLayout)
public:
    QQuickGridLayoutPrivate(): columns(-1), rows(-1), flow(QQuickGridLayout::LeftToRight) {}
    int columns;
    int rows;
    QQuickGridLayout::Flow flow;
};


/**********************************
 **
 ** QQuickLinearLayout
 **
 **/
class QQuickLinearLayoutPrivate;
class Q_QUICKLAYOUTS_EXPORT QQuickLinearLayout : public QQuickGridLayoutBase
{
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(qreal spacing READ spacing WRITE setSpacing NOTIFY spacingChanged)
    Q_PROPERTY(bool uniformCellSizes READ uniformCellSizes WRITE setUniformCellSizes
               NOTIFY uniformCellSizesChanged REVISION(6, 6) FINAL)
public:
    explicit QQuickLinearLayout(Qt::Orientation orientation,
                                QQuickItem *parent = nullptr);
    void insertLayoutItem(QQuickItem *item);
    qreal spacing() const;
    void setSpacing(qreal spacing);
    bool uniformCellSizes() const;
    void setUniformCellSizes(bool uniformCellSizes);

    void insertLayoutItems() override;

Q_SIGNALS:
    void spacingChanged();
    Q_REVISION(6, 6) void uniformCellSizesChanged();
private:
    Q_DECLARE_PRIVATE(QQuickLinearLayout)
};

class QQuickLinearLayoutPrivate : public QQuickGridLayoutBasePrivate
{
    Q_DECLARE_PUBLIC(QQuickLinearLayout)
public:
    QQuickLinearLayoutPrivate() {}
};


/**********************************
 **
 ** QQuickRowLayout
 **
 **/
class Q_QUICKLAYOUTS_EXPORT  QQuickRowLayout : public QQuickLinearLayout
{
    Q_OBJECT
    QML_NAMED_ELEMENT(RowLayout)
    QML_ADDED_IN_VERSION(1, 0)

public:
    explicit QQuickRowLayout(QQuickItem *parent = nullptr)
        : QQuickLinearLayout(Qt::Horizontal, parent) {}
};


/**********************************
 **
 ** QQuickColumnLayout
 **
 **/
class Q_QUICKLAYOUTS_EXPORT QQuickColumnLayout : public QQuickLinearLayout
{
    Q_OBJECT
    QML_NAMED_ELEMENT(ColumnLayout)
    QML_ADDED_IN_VERSION(1, 0)

public:
    explicit QQuickColumnLayout(QQuickItem *parent = nullptr)
        : QQuickLinearLayout(Qt::Vertical, parent) {}
};

QT_END_NAMESPACE

#endif // QQUICKLINEARLAYOUT_P_H
