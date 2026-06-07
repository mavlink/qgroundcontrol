// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKLAYOUT_P_H
#define QQUICKLAYOUT_P_H

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

#include <QPointer>
#include <QQuickItem>
#include <QtCore/qflags.h>

#include <QtQuickLayouts/private/qquicklayoutglobal_p.h>
#include <private/qquickitem_p.h>
#include <QtQuick/private/qquickitemchangelistener_p.h>
#include <QtGui/private/qlayoutpolicy_p.h>
#include <QtGui/qguiapplication.h>

QT_BEGIN_NAMESPACE

class QQuickLayoutAttached;
Q_DECLARE_LOGGING_CATEGORY(lcQuickLayouts)

class QQuickLayoutPrivate;
class Q_QUICKLAYOUTS_EXPORT QQuickLayout : public QQuickItem, public QQuickItemChangeListener

{
    Q_OBJECT
    QML_NAMED_ELEMENT(Layout)
    QML_ADDED_IN_VERSION(1, 0)
    QML_UNCREATABLE("Do not create objects of type Layout.")
    QML_ATTACHED(QQuickLayoutAttached)

public:
    enum SizeHint {
        MinimumSize = 0,
        PreferredSize,
        MaximumSize,
        NSizes
    };

    enum EnsureLayoutItemsUpdatedOption {
        Recursive                     = 0b001,
        ApplySizeHints                = 0b010
    };

    enum SizePolicy {
        SizePolicyImplicit = 1,
        SizePolicyExplicit
    };
    Q_ENUM(SizePolicy)

    Q_DECLARE_FLAGS(EnsureLayoutItemsUpdatedOptions, EnsureLayoutItemsUpdatedOption)

    explicit QQuickLayout(QQuickLayoutPrivate &dd, QQuickItem *parent = nullptr);
    ~QQuickLayout();

    static QQuickLayoutAttached *qmlAttachedProperties(QObject *object);


    void componentComplete() override;
    virtual QSizeF sizeHint(Qt::SizeHint whichSizeHint) const = 0;
    virtual void setAlignment(QQuickItem *item, Qt::Alignment align) = 0;
    virtual void setStretchFactor(QQuickItem *item, int stretchFactor, Qt::Orientation orient) = 0;

    virtual void invalidate(QQuickItem * childItem = nullptr);
    virtual void updateLayoutItems() = 0;

    void ensureLayoutItemsUpdated(EnsureLayoutItemsUpdatedOptions options = {}) const;

    // iterator
    virtual QQuickItem *itemAt(int index) const = 0;
    virtual int itemCount() const = 0;

    virtual void rearrange(const QSizeF &);

    static void effectiveSizeHints_helper(QQuickItem *item, QSizeF *cachedSizeHints, QQuickLayoutAttached **info, bool useFallbackToWidthOrHeight);
    static QLayoutPolicy::Policy effectiveSizePolicy_helper(QQuickItem *item, Qt::Orientation orientation, QQuickLayoutAttached *info);
    bool shouldIgnoreItem(QQuickItem *child) const;
    void checkAnchors(QQuickItem *item) const;

    void itemChange(ItemChange change, const ItemChangeData &value) override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)  override;
    bool isReady() const;
    void deactivateRecur();

    bool invalidated() const;
    bool invalidatedArrangement() const;
    bool isMirrored() const;

    /* QQuickItemChangeListener */
    void itemSiblingOrderChanged(QQuickItem *item) override;
    void itemImplicitWidthChanged(QQuickItem *item) override;
    void itemImplicitHeightChanged(QQuickItem *item) override;
    void itemDestroyed(QQuickItem *item) override;
    void itemVisibilityChanged(QQuickItem *item) override;

    void maybeSubscribeToBaseLineOffsetChanges(QQuickItem *item);

    Q_INVOKABLE void _q_dumpLayoutTree() const;
    void dumpLayoutTreeRecursive(int level, QString &buf) const;

protected:
    void updatePolish() override;

    enum Orientation {
        Vertical = 0,
        Horizontal,
        NOrientations
    };

protected Q_SLOTS:
    void invalidateSenderItem();

private:
    unsigned m_inUpdatePolish : 1;
    unsigned m_polishInsideUpdatePolish : 2;

    Q_DECLARE_PRIVATE(QQuickLayout)

    friend class QQuickLayoutAttached;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QQuickLayout::EnsureLayoutItemsUpdatedOptions)

class QQuickLayoutPrivate : public QQuickItemPrivate
{
    Q_DECLARE_PUBLIC(QQuickLayout)
public:
    QQuickLayoutPrivate() : m_dirty(true)
                          , m_dirtyArrangement(true)
                          , m_isReady(false)
                          , m_disableRearrange(true)
                          , m_hasItemChangeListeners(false) {}

    void applySizeHints() const;

protected:
    /* m_dirty == true means that something in the layout was changed,
       but its state has not been synced to the internal grid layout engine. It is usually:
       1. A child item was added or removed from the layout (or made visible/invisble)
       2. A child item got one of its size hints changed
    */
    mutable unsigned m_dirty : 1;
    /* m_dirtyArrangement == true means that the layout still needs a rearrange despite that
     * m_dirty == false. This is only used for the case that a layout has been invalidated,
     * but its new size is the same as the old size (in that case the child layout won't get
     * a geometryChanged() notification, which rearrange() usually reacts to)
     */
    mutable unsigned m_dirtyArrangement : 1;
    unsigned m_isReady : 1;
    unsigned m_disableRearrange : 1;
    unsigned m_hasItemChangeListeners : 1;      // if false, we don't need to remove its item change listeners...
};


class Q_QUICKLAYOUTS_EXPORT QQuickLayoutAttached : public QObject
{
    Q_OBJECT
    Q_PROPERTY(qreal minimumWidth READ minimumWidth WRITE setMinimumWidth NOTIFY minimumWidthChanged FINAL)
    Q_PROPERTY(qreal minimumHeight READ minimumHeight WRITE setMinimumHeight NOTIFY minimumHeightChanged FINAL)
    Q_PROPERTY(qreal preferredWidth READ preferredWidth WRITE setPreferredWidth NOTIFY preferredWidthChanged FINAL)
    Q_PROPERTY(qreal preferredHeight READ preferredHeight WRITE setPreferredHeight NOTIFY preferredHeightChanged FINAL)
    Q_PROPERTY(qreal maximumWidth READ maximumWidth WRITE setMaximumWidth NOTIFY maximumWidthChanged FINAL)
    Q_PROPERTY(qreal maximumHeight READ maximumHeight WRITE setMaximumHeight NOTIFY maximumHeightChanged FINAL)
    Q_PROPERTY(bool fillHeight READ fillHeight WRITE setFillHeight NOTIFY fillHeightChanged FINAL)
    Q_PROPERTY(bool fillWidth READ fillWidth WRITE setFillWidth NOTIFY fillWidthChanged FINAL)
    Q_PROPERTY(QQuickLayout::SizePolicy useDefaultSizePolicy READ useDefaultSizePolicy WRITE setUseDefaultSizePolicy NOTIFY useDefaultSizePolicyChanged FINAL)
    Q_PROPERTY(int row READ row WRITE setRow NOTIFY rowChanged FINAL)
    Q_PROPERTY(int column READ column WRITE setColumn NOTIFY columnChanged FINAL)
    Q_PROPERTY(int rowSpan READ rowSpan WRITE setRowSpan NOTIFY rowSpanChanged FINAL)
    Q_PROPERTY(int columnSpan READ columnSpan WRITE setColumnSpan NOTIFY columnSpanChanged FINAL)
    Q_PROPERTY(Qt::Alignment alignment READ alignment WRITE setAlignment NOTIFY alignmentChanged FINAL)
    Q_PROPERTY(int horizontalStretchFactor READ horizontalStretchFactor WRITE setHorizontalStretchFactor NOTIFY horizontalStretchFactorChanged FINAL)
    Q_PROPERTY(int verticalStretchFactor READ verticalStretchFactor WRITE setVerticalStretchFactor NOTIFY verticalStretchFactorChanged FINAL)

    Q_PROPERTY(qreal margins READ margins WRITE setMargins NOTIFY marginsChanged FINAL)
    Q_PROPERTY(qreal leftMargin READ leftMargin WRITE setLeftMargin RESET resetLeftMargin NOTIFY leftMarginChanged FINAL)
    Q_PROPERTY(qreal topMargin READ topMargin WRITE setTopMargin RESET resetTopMargin NOTIFY topMarginChanged FINAL)
    Q_PROPERTY(qreal rightMargin READ rightMargin WRITE setRightMargin RESET resetRightMargin NOTIFY rightMarginChanged FINAL)
    Q_PROPERTY(qreal bottomMargin READ bottomMargin WRITE setBottomMargin RESET resetBottomMargin NOTIFY bottomMarginChanged FINAL)

public:
    QQuickLayoutAttached(QObject *object);

    qreal minimumWidth() const { return !m_isMinimumWidthSet ? sizeHint(Qt::MinimumSize, Qt::Horizontal) : m_minimumWidth; }
    void setMinimumWidth(qreal width);
    bool isMinimumWidthSet() const {return m_isMinimumWidthSet; }

    qreal minimumHeight() const { return !m_isMinimumHeightSet ? sizeHint(Qt::MinimumSize, Qt::Vertical) : m_minimumHeight; }
    void setMinimumHeight(qreal height);
    bool isMinimumHeightSet() const {return m_isMinimumHeightSet; }

    qreal preferredWidth() const { return m_preferredWidth; }
    void setPreferredWidth(qreal width);
    bool isPreferredWidthSet() const {return m_preferredWidth > -1; }

    qreal preferredHeight() const { return m_preferredHeight; }
    void setPreferredHeight(qreal width);
    bool isPreferredHeightSet() const {return m_preferredHeight > -1; }

    qreal maximumWidth() const { return !m_isMaximumWidthSet ? sizeHint(Qt::MaximumSize, Qt::Horizontal) : m_maximumWidth; }
    void setMaximumWidth(qreal width);
    bool isMaximumWidthSet() const {return m_isMaximumWidthSet; }

    qreal maximumHeight() const { return !m_isMaximumHeightSet ? sizeHint(Qt::MaximumSize, Qt::Vertical) : m_maximumHeight; }
    void setMaximumHeight(qreal height);
    bool isMaximumHeightSet() const {return m_isMaximumHeightSet; }

    void setMinimumImplicitSize(const QSizeF &sz);
    void setMaximumImplicitSize(const QSizeF &sz);

    bool fillWidth() const {
        bool effectiveFillWidth = m_fillWidth;
        if (!m_isFillWidthSet && qobject_cast<QQuickLayout *>(item())) {
            effectiveFillWidth = true;
        } else if (auto *itemPriv = itemForSizePolicy(m_isFillWidthSet)) {
            QLayoutPolicy::Policy hPolicy = itemPriv->sizePolicy().horizontalPolicy();
            effectiveFillWidth = (hPolicy & QLayoutPolicy::GrowFlag);
        }
        return effectiveFillWidth;
    }
    void setFillWidth(bool fill);
    bool isFillWidthSet() const { return m_isFillWidthSet; }

    bool fillHeight() const {
        bool effectiveFillHeight = m_fillHeight;
        if (!m_isFillHeightSet && qobject_cast<QQuickLayout *>(item())) {
            effectiveFillHeight = true;
        } else if (auto *itemPriv = itemForSizePolicy(m_isFillHeightSet)) {
            QLayoutPolicy::Policy vPolicy = itemPriv->sizePolicy().verticalPolicy();
            effectiveFillHeight = (vPolicy & QLayoutPolicy::GrowFlag);
        }
        return effectiveFillHeight;
    }
    void setFillHeight(bool fill);
    bool isFillHeightSet() const { return m_isFillHeightSet; }

    QQuickLayout::SizePolicy useDefaultSizePolicy() const {
        const bool appDefSizePolicy = QGuiApplication::testAttribute(Qt::AA_QtQuickUseDefaultSizePolicy);
        return (m_isUseDefaultSizePolicySet ? m_useDefaultSizePolicy : (appDefSizePolicy ? QQuickLayout::SizePolicyImplicit : QQuickLayout::SizePolicyExplicit));
    }
    void setUseDefaultSizePolicy(QQuickLayout::SizePolicy sizePolicy);

    int row() const { return qMax(m_row, 0); }
    void setRow(int row);
    bool isRowSet() const { return m_row >= 0; }
    int column() const { return qMax(m_column, 0); }
    void setColumn(int column);
    bool isColumnSet() const { return m_column >= 0; }

    int rowSpan() const { return m_rowSpan; }
    void setRowSpan(int span);
    int columnSpan() const { return m_columnSpan; }
    void setColumnSpan(int span);

    Qt::Alignment alignment() const { return m_alignment; }
    void setAlignment(Qt::Alignment align);
    bool isAlignmentSet() const {return m_isAlignmentSet; }

    int horizontalStretchFactor() const { return m_horizontalStretch; }
    void setHorizontalStretchFactor(int stretchFactor);
    bool isHorizontalStretchFactorSet() const { return m_horizontalStretch > -1; }
    int verticalStretchFactor() const { return m_verticalStretch; }
    void setVerticalStretchFactor(int stretchFactor);
    bool isVerticalStretchFactorSet() const { return m_verticalStretch > -1; }

    qreal margins() const { return m_defaultMargins; }
    void setMargins(qreal m);
    bool isMarginsSet() const { return m_isMarginsSet; }

    qreal leftMargin() const { return m_isLeftMarginSet ? m_margins.left() : m_defaultMargins; }
    void setLeftMargin(qreal m);
    void resetLeftMargin();
    bool isLeftMarginSet() const { return m_isLeftMarginSet; }

    qreal topMargin() const { return m_isTopMarginSet ? m_margins.top() : m_defaultMargins; }
    void setTopMargin(qreal m);
    void resetTopMargin();
    bool isTopMarginSet() const {return m_isTopMarginSet; }

    qreal rightMargin() const { return m_isRightMarginSet ? m_margins.right() : m_defaultMargins; }
    void setRightMargin(qreal m);
    void resetRightMargin();
    bool isRightMarginSet() const { return m_isRightMarginSet; }

    qreal bottomMargin() const { return m_isBottomMarginSet ? m_margins.bottom() : m_defaultMargins; }
    void setBottomMargin(qreal m);
    void resetBottomMargin();
    bool isBottomMarginSet() const { return m_isBottomMarginSet; }

    QMarginsF qMargins() const {
        return QMarginsF(leftMargin(), topMargin(), rightMargin(), bottomMargin());
    }

    QMarginsF effectiveQMargins() const {
        bool mirrored = parentLayout() && parentLayout()->isMirrored();
        if (mirrored)
            return QMarginsF(rightMargin(), topMargin(), leftMargin(), bottomMargin());
        else
            return qMargins();
    }

    bool setChangesNotificationEnabled(bool enabled)
    {
        const bool old = m_changesNotificationEnabled;
        m_changesNotificationEnabled = enabled;
        return old;
    }

    qreal sizeHint(Qt::SizeHint which, Qt::Orientation orientation) const;

    bool isExtentExplicitlySet(Qt::Orientation o, Qt::SizeHint whichSize) const
    {
        switch (whichSize) {
        case Qt::MinimumSize:
            return o == Qt::Horizontal ? m_isMinimumWidthSet : m_isMinimumHeightSet;
        case Qt::MaximumSize:
            return o == Qt::Horizontal ? m_isMaximumWidthSet : m_isMaximumHeightSet;
        case Qt::PreferredSize:
            return true;            // Layout.preferredWidth is always explicitly set
        case Qt::MinimumDescent:    // Not supported
        case Qt::NSizeHints:
            return false;
        }
        return false;
    }

    QQuickItemPrivate *itemForSizePolicy(bool isFillSet) const
    {
        QQuickItemPrivate *itemPriv = nullptr;
        if (!isFillSet && qobject_cast<QQuickItem *>(item()) &&
            QGuiApplication::testAttribute(Qt::AA_QtQuickUseDefaultSizePolicy))
            itemPriv = QQuickItemPrivate::get(item());
        return itemPriv;
    }

Q_SIGNALS:
    void minimumWidthChanged();
    void minimumHeightChanged();
    void preferredWidthChanged();
    void preferredHeightChanged();
    void maximumWidthChanged();
    void maximumHeightChanged();
    void fillWidthChanged();
    void fillHeightChanged();
    void useDefaultSizePolicyChanged();
    void leftMarginChanged();
    void topMarginChanged();
    void rightMarginChanged();
    void bottomMarginChanged();
    void marginsChanged();
    void rowChanged();
    void columnChanged();
    void rowSpanChanged();
    void columnSpanChanged();
    void alignmentChanged();
    void horizontalStretchFactorChanged();
    void verticalStretchFactorChanged();

private:
    void invalidateItem();
    QQuickLayout *parentLayout() const;
    QQuickItem *item() const;
private:
    qreal m_minimumWidth;
    qreal m_minimumHeight;
    qreal m_preferredWidth;
    qreal m_preferredHeight;
    qreal m_maximumWidth;
    qreal m_maximumHeight;

    qreal m_defaultMargins;
    QMarginsF m_margins;

    qreal m_fallbackWidth;
    qreal m_fallbackHeight;

    // GridLayout specific properties
    int m_row;
    int m_column;
    int m_rowSpan;
    int m_columnSpan;

    unsigned m_fillWidth : 1;
    unsigned m_fillHeight : 1;
    unsigned m_isFillWidthSet : 1;
    unsigned m_isFillHeightSet : 1;
    unsigned m_isUseDefaultSizePolicySet: 1;
    QQuickLayout::SizePolicy m_useDefaultSizePolicy;
    unsigned m_isMinimumWidthSet : 1;
    unsigned m_isMinimumHeightSet : 1;
    // preferredWidth and preferredHeight are always explicit, since
    // their implicit equivalent is implicitWidth and implicitHeight
    unsigned m_isMaximumWidthSet : 1;
    unsigned m_isMaximumHeightSet : 1;
    unsigned m_changesNotificationEnabled : 1;
    unsigned m_isMarginsSet : 1;
    unsigned m_isLeftMarginSet : 1;
    unsigned m_isTopMarginSet : 1;
    unsigned m_isRightMarginSet : 1;
    unsigned m_isBottomMarginSet : 1;
    unsigned m_isAlignmentSet : 1;
    Qt::Alignment m_alignment;
    int m_horizontalStretch;
    int m_verticalStretch;
    friend class QQuickLayout;
};

inline QQuickLayoutAttached *attachedLayoutObject(QQuickItem *item, bool create = true)
{
    return static_cast<QQuickLayoutAttached *>(qmlAttachedPropertiesObject<QQuickLayout>(item, create));
}

QT_END_NAMESPACE

#endif // QQUICKLAYOUT_P_H
