// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKCONTROL_P_H
#define QQUICKCONTROL_P_H

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

#include <QtCore/qlocale.h>
#include <QtGui/qpalette.h>
#if QT_CONFIG(accessibility)
#include <QtGui/qaccessible_base.h>
#endif
#include <QtQuick/qquickitem.h>
#include <QtQuick/private/qquickpalette_p.h>
#include <QtQuickTemplates2/private/qtquicktemplates2global_p.h>

QT_BEGIN_NAMESPACE

class QQuickControlPrivate;

class Q_QUICKTEMPLATES2_EXPORT QQuickControl : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QFont font READ font WRITE setFont RESET resetFont NOTIFY fontChanged FINAL)
    Q_PROPERTY(qreal availableWidth READ availableWidth NOTIFY availableWidthChanged FINAL)
    Q_PROPERTY(qreal availableHeight READ availableHeight NOTIFY availableHeightChanged FINAL)
    Q_PROPERTY(qreal padding READ padding WRITE setPadding RESET resetPadding NOTIFY paddingChanged FINAL)
    Q_PROPERTY(qreal topPadding READ topPadding WRITE setTopPadding RESET resetTopPadding NOTIFY topPaddingChanged FINAL)
    Q_PROPERTY(qreal leftPadding READ leftPadding WRITE setLeftPadding RESET resetLeftPadding NOTIFY leftPaddingChanged FINAL)
    Q_PROPERTY(qreal rightPadding READ rightPadding WRITE setRightPadding RESET resetRightPadding NOTIFY rightPaddingChanged FINAL)
    Q_PROPERTY(qreal bottomPadding READ bottomPadding WRITE setBottomPadding RESET resetBottomPadding NOTIFY bottomPaddingChanged FINAL)
    Q_PROPERTY(qreal spacing READ spacing WRITE setSpacing RESET resetSpacing NOTIFY spacingChanged FINAL)
    Q_PROPERTY(QLocale locale READ locale WRITE setLocale RESET resetLocale NOTIFY localeChanged FINAL)
    Q_PROPERTY(bool mirrored READ isMirrored NOTIFY mirroredChanged FINAL)
    QT6_ONLY(Q_PROPERTY(Qt::FocusPolicy focusPolicy READ focusPolicy WRITE setFocusPolicy NOTIFY focusPolicyChanged FINAL))
    Q_PROPERTY(Qt::FocusReason focusReason READ focusReason WRITE setFocusReason NOTIFY focusReasonChanged FINAL)
    Q_PROPERTY(bool visualFocus READ hasVisualFocus NOTIFY visualFocusChanged FINAL)
    Q_PROPERTY(bool hovered READ isHovered NOTIFY hoveredChanged FINAL)
    Q_PROPERTY(bool hoverEnabled READ isHoverEnabled WRITE setHoverEnabled RESET resetHoverEnabled NOTIFY hoverEnabledChanged FINAL)
    Q_PROPERTY(bool wheelEnabled READ isWheelEnabled WRITE setWheelEnabled NOTIFY wheelEnabledChanged FINAL)
    Q_PROPERTY(QQuickItem *background READ background WRITE setBackground NOTIFY backgroundChanged FINAL)
    Q_PROPERTY(QQuickItem *contentItem READ contentItem WRITE setContentItem NOTIFY contentItemChanged FINAL)
    Q_PROPERTY(qreal baselineOffset READ baselineOffset WRITE setBaselineOffset RESET resetBaselineOffset NOTIFY baselineOffsetChanged FINAL)
    // 2.5 (Qt 5.12)
    Q_PROPERTY(qreal horizontalPadding READ horizontalPadding WRITE setHorizontalPadding RESET resetHorizontalPadding NOTIFY horizontalPaddingChanged FINAL REVISION(2, 5))
    Q_PROPERTY(qreal verticalPadding READ verticalPadding WRITE setVerticalPadding RESET resetVerticalPadding NOTIFY verticalPaddingChanged FINAL REVISION(2, 5))
    Q_PROPERTY(qreal implicitContentWidth READ implicitContentWidth NOTIFY implicitContentWidthChanged FINAL REVISION(2, 5))
    Q_PROPERTY(qreal implicitContentHeight READ implicitContentHeight NOTIFY implicitContentHeightChanged FINAL REVISION(2, 5))
    Q_PROPERTY(qreal implicitBackgroundWidth READ implicitBackgroundWidth NOTIFY implicitBackgroundWidthChanged FINAL REVISION(2, 5))
    Q_PROPERTY(qreal implicitBackgroundHeight READ implicitBackgroundHeight NOTIFY implicitBackgroundHeightChanged FINAL REVISION(2, 5))
    Q_PROPERTY(qreal topInset READ topInset WRITE setTopInset RESET resetTopInset NOTIFY topInsetChanged FINAL REVISION(2, 5))
    Q_PROPERTY(qreal leftInset READ leftInset WRITE setLeftInset RESET resetLeftInset NOTIFY leftInsetChanged FINAL REVISION(2, 5))
    Q_PROPERTY(qreal rightInset READ rightInset WRITE setRightInset RESET resetRightInset NOTIFY rightInsetChanged FINAL REVISION(2, 5))
    Q_PROPERTY(qreal bottomInset READ bottomInset WRITE setBottomInset RESET resetBottomInset NOTIFY bottomInsetChanged FINAL REVISION(2, 5))
    Q_CLASSINFO("DeferredPropertyNames", "background,contentItem")
    QML_NAMED_ELEMENT(Control)
    QML_ADDED_IN_VERSION(2, 0)

public:
    explicit QQuickControl(QQuickItem *parent = nullptr);
    ~QQuickControl();

    QFont font() const;
    void setFont(const QFont &font);
    void resetFont();

    qreal availableWidth() const;
    qreal availableHeight() const;

    qreal padding() const;
    void setPadding(qreal padding);
    void resetPadding();

    qreal topPadding() const;
    void setTopPadding(qreal padding);
    void resetTopPadding();

    qreal leftPadding() const;
    void setLeftPadding(qreal padding);
    void resetLeftPadding();

    qreal rightPadding() const;
    void setRightPadding(qreal padding);
    void resetRightPadding();

    qreal bottomPadding() const;
    void setBottomPadding(qreal padding);
    void resetBottomPadding();

    qreal spacing() const;
    void setSpacing(qreal spacing);
    void resetSpacing();

    QLocale locale() const;
    void setLocale(const QLocale &locale);
    void resetLocale();

    bool isMirrored() const;

    Qt::FocusReason focusReason() const;
    void setFocusReason(Qt::FocusReason reason);

    bool hasVisualFocus() const;

    bool isHovered() const;
    void setHovered(bool hovered);

    bool isHoverEnabled() const;
    void setHoverEnabled(bool enabled);
    void resetHoverEnabled();

    bool isWheelEnabled() const;
    void setWheelEnabled(bool enabled);

    QQuickItem *background() const;
    void setBackground(QQuickItem *background);

    QQuickItem *contentItem() const;
    void setContentItem(QQuickItem *item);

    qreal baselineOffset() const;
    void setBaselineOffset(qreal offset);
    void resetBaselineOffset();

     // 2.5 (Qt 5.12)
    qreal horizontalPadding() const;
    void setHorizontalPadding(qreal padding);
    void resetHorizontalPadding();

    qreal verticalPadding() const;
    void setVerticalPadding(qreal padding);
    void resetVerticalPadding();

    qreal implicitContentWidth() const;
    qreal implicitContentHeight() const;

    qreal implicitBackgroundWidth() const;
    qreal implicitBackgroundHeight() const;

    qreal topInset() const;
    void setTopInset(qreal inset);
    void resetTopInset();

    qreal leftInset() const;
    void setLeftInset(qreal inset);
    void resetLeftInset();

    qreal rightInset() const;
    void setRightInset(qreal inset);
    void resetRightInset();

    qreal bottomInset() const;
    void setBottomInset(qreal inset);
    void resetBottomInset();

Q_SIGNALS:
    void fontChanged();
    void availableWidthChanged();
    void availableHeightChanged();
    void paddingChanged();
    void topPaddingChanged();
    void leftPaddingChanged();
    void rightPaddingChanged();
    void bottomPaddingChanged();
    void spacingChanged();
    void localeChanged();
    void focusReasonChanged();
    void mirroredChanged();
    void visualFocusChanged();
    void hoveredChanged();
    void hoverEnabledChanged();
    void wheelEnabledChanged();
    void backgroundChanged();
    void contentItemChanged();
    void baselineOffsetChanged();
    // 2.5 (Qt 5.12)
    Q_REVISION(2, 5) void horizontalPaddingChanged();
    Q_REVISION(2, 5) void verticalPaddingChanged();
    Q_REVISION(2, 5) void implicitContentWidthChanged();
    Q_REVISION(2, 5) void implicitContentHeightChanged();
    Q_REVISION(2, 5) void implicitBackgroundWidthChanged();
    Q_REVISION(2, 5) void implicitBackgroundHeightChanged();
    Q_REVISION(2, 5) void topInsetChanged();
    Q_REVISION(2, 5) void leftInsetChanged();
    Q_REVISION(2, 5) void rightInsetChanged();
    Q_REVISION(2, 5) void bottomInsetChanged();

protected:
    virtual QFont defaultFont() const;

    QQuickControl(QQuickControlPrivate &dd, QQuickItem *parent);

    void classBegin() override;
    void componentComplete() override;

    void itemChange(ItemChange change, const ItemChangeData &value) override;

    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
#if QT_CONFIG(quicktemplates2_hover)
    void hoverEnterEvent(QHoverEvent *event) override;
    void hoverMoveEvent(QHoverEvent *event) override;
    void hoverLeaveEvent(QHoverEvent *event) override;
#endif
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseUngrabEvent() override;
#if QT_CONFIG(quicktemplates2_multitouch)
    void touchEvent(QTouchEvent *event) override;
    void touchUngrabEvent() override;
#endif
#if QT_CONFIG(wheelevent)
    void wheelEvent(QWheelEvent *event) override;
#endif

    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;

    virtual void fontChange(const QFont &newFont, const QFont &oldFont);
#if QT_CONFIG(quicktemplates2_hover)
    virtual void hoverChange();
#endif
    virtual void mirrorChange();
    virtual void spacingChange(qreal newSpacing, qreal oldSpacing);
    virtual void paddingChange(const QMarginsF &newPadding, const QMarginsF &oldPadding);
    virtual void contentItemChange(QQuickItem *newItem, QQuickItem *oldItem);
    virtual void localeChange(const QLocale &newLocale, const QLocale &oldLocale);
    virtual void insetChange(const QMarginsF &newInset, const QMarginsF &oldInset);
    virtual void enabledChange();

#if QT_CONFIG(accessibility)
    virtual QAccessible::Role accessibleRole() const;
    virtual void accessibilityActiveChanged(bool active);
#endif

    // helper functions which avoid to check QT_CONFIG(accessibility)
    QString accessibleName() const;
    void maybeSetAccessibleName(const QString &name);

    QVariant accessibleProperty(const char *propertyName);
    bool setAccessibleProperty(const char *propertyName, const QVariant &value);

private:
    Q_DISABLE_COPY(QQuickControl)
    Q_DECLARE_PRIVATE(QQuickControl)
};

QT_END_NAMESPACE

#endif // QQUICKCONTROL_P_H
