// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKANCHORS_P_H
#define QQUICKANCHORS_P_H

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

#include <qqml.h>

#include <QtCore/QObject>

#include <private/qtquickglobal_p.h>

QT_BEGIN_NAMESPACE

class QQuickItem;
class QQuickAnchorsPrivate;
class QQuickAnchorLine;
class Q_QUICK_EXPORT QQuickAnchors : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QQuickAnchorLine left READ left WRITE setLeft RESET resetLeft NOTIFY leftChanged FINAL)
    Q_PROPERTY(QQuickAnchorLine right READ right WRITE setRight RESET resetRight NOTIFY rightChanged FINAL)
    Q_PROPERTY(QQuickAnchorLine horizontalCenter READ horizontalCenter WRITE setHorizontalCenter RESET resetHorizontalCenter NOTIFY horizontalCenterChanged FINAL)
    Q_PROPERTY(QQuickAnchorLine top READ top WRITE setTop RESET resetTop NOTIFY topChanged FINAL)
    Q_PROPERTY(QQuickAnchorLine bottom READ bottom WRITE setBottom RESET resetBottom NOTIFY bottomChanged FINAL)
    Q_PROPERTY(QQuickAnchorLine verticalCenter READ verticalCenter WRITE setVerticalCenter RESET resetVerticalCenter NOTIFY verticalCenterChanged FINAL)
    Q_PROPERTY(QQuickAnchorLine baseline READ baseline WRITE setBaseline RESET resetBaseline NOTIFY baselineChanged FINAL)
    Q_PROPERTY(qreal margins READ margins WRITE setMargins NOTIFY marginsChanged FINAL)
    Q_PROPERTY(qreal leftMargin READ leftMargin WRITE setLeftMargin RESET resetLeftMargin NOTIFY leftMarginChanged FINAL)
    Q_PROPERTY(qreal rightMargin READ rightMargin WRITE setRightMargin RESET resetRightMargin NOTIFY rightMarginChanged FINAL)
    Q_PROPERTY(qreal horizontalCenterOffset READ horizontalCenterOffset WRITE setHorizontalCenterOffset NOTIFY horizontalCenterOffsetChanged FINAL)
    Q_PROPERTY(qreal topMargin READ topMargin WRITE setTopMargin RESET resetTopMargin NOTIFY topMarginChanged FINAL)
    Q_PROPERTY(qreal bottomMargin READ bottomMargin WRITE setBottomMargin RESET resetBottomMargin NOTIFY bottomMarginChanged FINAL)
    Q_PROPERTY(qreal verticalCenterOffset READ verticalCenterOffset WRITE setVerticalCenterOffset NOTIFY verticalCenterOffsetChanged FINAL)
    Q_PROPERTY(qreal baselineOffset READ baselineOffset WRITE setBaselineOffset NOTIFY baselineOffsetChanged FINAL)
    Q_PROPERTY(QQuickItem *fill READ fill WRITE setFill RESET resetFill NOTIFY fillChanged FINAL)
    Q_PROPERTY(QQuickItem *centerIn READ centerIn WRITE setCenterIn RESET resetCenterIn NOTIFY centerInChanged FINAL)
    Q_PROPERTY(bool alignWhenCentered READ alignWhenCentered WRITE setAlignWhenCentered NOTIFY centerAlignedChanged FINAL)
    QML_ANONYMOUS
    QML_ADDED_IN_VERSION(2, 0)

public:
    QQuickAnchors(QQuickItem *item, QObject *parent=nullptr);
    ~QQuickAnchors() override;

    enum Anchor
#if defined(Q_CC_CLANG) || !defined(Q_CC_GNU) // meaning: clang and msvc, but NOT gcc proper (because, you know, Q_CC_CLANG implies Q_CC_GNU)
            // Not specifying the enum base type will have MSVC 'interpret' it as signed instead of an unsigned bit-field.
            // However, specifying the enum base type breaks many GCCs, which complain that it can't store all values in a 7 bit bitfield.
            : uint
#endif
    {
        InvalidAnchor = 0x0,
        LeftAnchor = 0x01,
        RightAnchor = 0x02,
        TopAnchor = 0x04,
        BottomAnchor = 0x08,
        HCenterAnchor = 0x10,
        VCenterAnchor = 0x20,
        BaselineAnchor = 0x40,
        Horizontal_Mask = LeftAnchor | RightAnchor | HCenterAnchor,
        Vertical_Mask = TopAnchor | BottomAnchor | VCenterAnchor | BaselineAnchor
    };
    Q_DECLARE_FLAGS(Anchors, Anchor)
    Q_FLAG(Anchors)

    QQuickAnchorLine left() const;
    void setLeft(const QQuickAnchorLine &edge);
    void resetLeft();

    QQuickAnchorLine right() const;
    void setRight(const QQuickAnchorLine &edge);
    void resetRight();

    QQuickAnchorLine horizontalCenter() const;
    void setHorizontalCenter(const QQuickAnchorLine &edge);
    void resetHorizontalCenter();

    QQuickAnchorLine top() const;
    void setTop(const QQuickAnchorLine &edge);
    void resetTop();

    QQuickAnchorLine bottom() const;
    void setBottom(const QQuickAnchorLine &edge);
    void resetBottom();

    QQuickAnchorLine verticalCenter() const;
    void setVerticalCenter(const QQuickAnchorLine &edge);
    void resetVerticalCenter();

    QQuickAnchorLine baseline() const;
    void setBaseline(const QQuickAnchorLine &edge);
    void resetBaseline();

    qreal leftMargin() const;
    void setLeftMargin(qreal);
    void resetLeftMargin();

    qreal rightMargin() const;
    void setRightMargin(qreal);
    void resetRightMargin();

    qreal horizontalCenterOffset() const;
    void setHorizontalCenterOffset(qreal);

    qreal topMargin() const;
    void setTopMargin(qreal);
    void resetTopMargin();

    qreal bottomMargin() const;
    void setBottomMargin(qreal);
    void resetBottomMargin();

    qreal margins() const;
    void setMargins(qreal);

    qreal verticalCenterOffset() const;
    void setVerticalCenterOffset(qreal);

    qreal baselineOffset() const;
    void setBaselineOffset(qreal);

    QQuickItem *fill() const;
    void setFill(QQuickItem *);
    void resetFill();

    QQuickItem *centerIn() const;
    void setCenterIn(QQuickItem *);
    void resetCenterIn();

    Anchors usedAnchors() const;
    Qt::Orientations activeDirections() const;

    bool mirrored();

    bool alignWhenCentered() const;
    void setAlignWhenCentered(bool);

    void classBegin();
    void componentComplete();

Q_SIGNALS:
    void leftChanged();
    void rightChanged();
    void topChanged();
    void bottomChanged();
    void verticalCenterChanged();
    void horizontalCenterChanged();
    void baselineChanged();
    void fillChanged();
    void centerInChanged();
    void leftMarginChanged();
    void rightMarginChanged();
    void topMarginChanged();
    void bottomMarginChanged();
    void marginsChanged();
    void verticalCenterOffsetChanged();
    void horizontalCenterOffsetChanged();
    void baselineOffsetChanged();
    void centerAlignedChanged();

private:
    friend class QQuickItemPrivate;
    Q_DISABLE_COPY(QQuickAnchors)
    Q_DECLARE_PRIVATE(QQuickAnchors)
};
Q_DECLARE_OPERATORS_FOR_FLAGS(QQuickAnchors::Anchors)

QT_END_NAMESPACE

#endif // QQUICKANCHORS_P_H
