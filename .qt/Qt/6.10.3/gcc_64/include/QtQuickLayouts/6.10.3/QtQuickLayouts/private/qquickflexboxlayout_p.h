// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKFLEXBOXLAYOUT_H
#define QQUICKFLEXBOXLAYOUT_H

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

#include <bitset>
#include <QtQuickLayouts/private/qquicklayoutglobal_p.h>
#include <QtQuickLayouts/private/qquicklayout_p.h>

QT_BEGIN_NAMESPACE

class QQuickFlexboxLayoutPrivate;
class QQuickFlexboxLayoutAttached;

class Q_QUICKLAYOUTS_EXPORT QQuickFlexboxLayout : public QQuickLayout
{
    Q_OBJECT

    Q_PROPERTY(FlexboxDirection direction READ direction WRITE setDirection NOTIFY directionChanged FINAL)
    Q_PROPERTY(FlexboxWrap wrap READ wrap WRITE setWrap NOTIFY wrapChanged FINAL)
    Q_PROPERTY(FlexboxAlignment alignItems READ alignItems WRITE setAlignItems NOTIFY alignItemsChanged FINAL)
    Q_PROPERTY(FlexboxAlignment alignContent READ alignContent WRITE setAlignContent NOTIFY alignContentChanged FINAL)
    Q_PROPERTY(FlexboxJustify justifyContent READ justifyContent WRITE setJustifyContent NOTIFY justifyContentChanged FINAL)
    Q_PROPERTY(qreal gap READ gap WRITE setGap NOTIFY gapChanged RESET resetGap FINAL)
    Q_PROPERTY(qreal rowGap READ rowGap WRITE setRowGap NOTIFY rowGapChanged RESET resetRowGap FINAL)
    Q_PROPERTY(qreal columnGap READ columnGap WRITE setColumnGap NOTIFY columnGapChanged RESET resetColumnGap FINAL)

    QML_NAMED_ELEMENT(FlexboxLayout)
    QML_ADDED_IN_VERSION(6, 10)
    QML_ATTACHED(QQuickFlexboxLayoutAttached)

public:
    explicit QQuickFlexboxLayout(QQuickItem *parent = nullptr);
    ~QQuickFlexboxLayout();

    enum FlexboxDirection { // Used as similar to CSS standard
        Column,
        ColumnReverse,
        Row,
        RowReverse
    };
    Q_ENUM(FlexboxDirection);

    enum FlexboxWrap {
        NoWrap,
        Wrap,
        WrapReverse
    };
    Q_ENUM(FlexboxWrap);

    // The alignments here can be mapped to the flexbox CSS assignments: align-items, align-content
    //
    // alignItems: AlignStart | AlignCenter | AlignEnd | AlignStretch
    // Note: AlignSpace* not supported by the flexAlignItems
    //
    // alignContent: AlignStart | AlignEnd | AlignCenter | AlignStretch | AlignSpaceBetween |
    //                   AlignSpaceAround
    //
    //      For instance, consider placing the items are placed within the flex with flexDirection
    //      set to Row
    //
    //      alignItems - This property causes flex items to be positioned as below with respective
    //                       value set
    //
    //      AlignStart - Flex items are positioned from the start of the cross axis
    //                          [[Item1][Item2][Item3][Item4][Item5]...]
    //      AlignEnd - Flex items are positioned from the end of the cross axis
    //                          [...[Item1][Item2][Item3][Item4][Item5]]
    //      AlignStretch - Flex items are stretched along the cross axis
    //                              ||     |     |     |     |     ||
    //                              ||Item1|Item2|Item3|Item4|Item5||
    //                              ||     |     |     |     |     ||
    //      AlignCenter - Flex items are centered along the cross axis
    //                              |                            |     ||
    //                              |[Item1][Item2][Item3][Item4]|Item5||
    //                              |                            |     ||
    //
    //      alignContent - This property causes flex items to be positioned considering space around
    //                         edge lines and in-between with respective value set
    //
    //      AlignStart - Lines are packed towards the start of the container
    //                          [[Item1][Item2][Item3][Item4][Item5]...]
    //      AlignEnd - Lines are packed towards the end of the container
    //                          [...[Item1][Item2][Item3][Item4][Item5]]
    //      AlignCenter - Lines are packed towards the center
    //                              ||     |     |     |     |     ||
    //                              ||Item1|Item2|Item3|Item4|Item5||
    //                              ||     |     |     |     |     ||
    //      AlignSpaceBetween - Lines are packed at the edges of the container and spaces
    //                          are placed in-between rows of the flex items
    //                                  |[Item1][Item2][Item3]|
    //                                  |                     |
    //                                  |[Item4][Item5][Item6]|
    //      AlignSpaceAround - Spaces are placed in-between the rows of the flex items and
    //                         would be shared around the edges (i.e. the space between the
    //                         items and at the edge of the container will vary)
    //                                  |                     |
    //                                  |[Item1][Item2][Item3]|
    //                                  |                     |
    //                                  |                     |
    //                                  |[Item4][Item5][Item6]|
    //                                  |                     |
    //      AlignStretch - Lines are stretched and there will be no space in-between
    //                                  |[Item1][Item2][Item3]|
    //                                  |[Item4][Item5][Item6]|
    enum FlexboxAlignment {
        AlignAuto = 0,
        AlignStart,
        AlignCenter,
        AlignEnd,
        AlignStretch, // Same as Layout.fillHeight or Layout.fillWidth
        AlignBaseline,
        AlignSpaceBetween,
        AlignSpaceAround,
        AlignSpaceEvenly
    };
    Q_ENUM(FlexboxAlignment)

    // The alignments can be used for justify-content
    enum FlexboxJustify {
        JustifyStart,
        JustifyCenter,
        JustifyEnd,
        JustifySpaceBetween,
        JustifySpaceAround,
        JustifySpaceEvenly
    };
    Q_ENUM(FlexboxJustify)

    // The alignments can be used for justify-content
    enum FlexboxEdge {
        EdgeLeft,
        EdgeRight,
        EdgeTop,
        EdgeBottom,
        EdgeAll,
        EdgeMax
    };
    Q_ENUM(FlexboxEdge)

    // The alignments can be used for justify-content
    enum FlexboxGap {
        GapRow,
        GapColumn,
        GapAll,
        GapMax
    };
    Q_ENUM(FlexboxGap)

    FlexboxDirection direction() const;
    void setDirection(FlexboxDirection);

    FlexboxWrap wrap() const;
    void setWrap(FlexboxWrap);

    FlexboxAlignment alignItems() const;
    void setAlignItems(FlexboxAlignment);

    FlexboxJustify justifyContent() const;
    void setJustifyContent(FlexboxJustify);

    FlexboxAlignment alignContent() const;
    void setAlignContent(FlexboxAlignment);

    qreal gap() const;
    void setGap(qreal);
    void resetGap();

    qreal rowGap() const;
    void setRowGap(qreal);
    void resetRowGap();

    qreal columnGap() const;
    void setColumnGap(qreal);
    void resetColumnGap();

    void componentComplete() override;
    QSizeF sizeHint(Qt::SizeHint whichSizeHint) const override;
    void setAlignment(QQuickItem *, Qt::Alignment)  override {}
    void setStretchFactor(QQuickItem *, int, Qt::Orientation) override {}

    void invalidate(QQuickItem *childItem = nullptr)  override;
    void updateLayoutItems() override;
    void rearrange(const QSizeF &) override;

    // iterator
    QQuickItem *itemAt(int index) const override;
    int itemCount() const override;

    /* QQuickItemChangeListener */
    void itemSiblingOrderChanged(QQuickItem *item) override;
    void itemVisibilityChanged(QQuickItem *item) override;

    /* internal */
    static QQuickFlexboxLayoutAttached *qmlAttachedProperties(QObject *object);
    bool isGapBitSet(QQuickFlexboxLayout::FlexboxGap gap) const;
    void checkAnchors(QQuickItem *item) const;

Q_SIGNALS:
    void countChanged();
    void directionChanged();
    void wrapChanged();
    void alignItemsChanged();
    void alignContentChanged();
    void justifyContentChanged();
    void gapChanged();
    void rowGapChanged();
    void columnGapChanged();

private:
    void childItemsChanged();

    friend class QQuickFlexboxLayoutAttached;
    Q_DECLARE_PRIVATE(QQuickFlexboxLayout)
};

class Q_QUICKLAYOUTS_EXPORT QQuickFlexboxLayoutAttached : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QQuickFlexboxLayout::FlexboxAlignment alignSelf READ alignSelf WRITE setAlignSelf NOTIFY alignSelfChanged FINAL)

public:
    QQuickFlexboxLayoutAttached(QObject *object);

    QQuickFlexboxLayout::FlexboxAlignment alignSelf() const;
    void setAlignSelf(const QQuickFlexboxLayout::FlexboxAlignment);

Q_SIGNALS:
    void alignSelfChanged();

private:
    // The child item in the flex layout allowed to override the parent align-item property
    QQuickFlexboxLayout::FlexboxAlignment m_alignSelf = QQuickFlexboxLayout::AlignAuto;
};

QT_END_NAMESPACE

#endif // QQUICKFLEXBOXLAYOUT_H
