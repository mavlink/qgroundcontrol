// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKCONTROL_P_P_H
#define QQUICKCONTROL_P_P_H

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

#include <QtQuickTemplates2/private/qquickcontrol_p.h>
#include <QtQuickTemplates2/private/qquickdeferredpointer_p_p.h>
#include <QtQuickTemplates2/private/qquicktheme_p.h>

#include <QtQuick/private/qquickitem_p.h>
#include <QtQuick/private/qquickitemchangelistener_p.h>
#include <QtQml/private/qlazilyallocated_p.h>

#if QT_CONFIG(accessibility)
#include <QtGui/qaccessible.h>
#endif

#include <QtCore/qloggingcategory.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcItemManagement)

class QQuickAccessibleAttached;

class Q_QUICKTEMPLATES2_EXPORT QQuickControlPrivate : public QQuickItemPrivate,
                                                      public QSafeQuickItemChangeListener<QQuickControlPrivate>
#if QT_CONFIG(accessibility)
    , public QAccessible::ActivationObserver
#endif
{
public:
    Q_DECLARE_PUBLIC(QQuickControl)

    QQuickControlPrivate();
    ~QQuickControlPrivate();

    static QQuickControlPrivate *get(QQuickControl *control)
    {
        return control->d_func();
    }

    void init();

#if QT_CONFIG(quicktemplates2_multitouch)
    virtual bool acceptTouch(const QTouchEvent::TouchPoint &point);
#endif
    virtual bool handlePress(const QPointF &point, ulong timestamp);
    virtual bool handleMove(const QPointF &point, ulong timestamp);
    virtual bool handleRelease(const QPointF &point, ulong timestamp);
    virtual void handleUngrab();

    void mirrorChange() override;

    inline QMarginsF getPadding() const { return QMarginsF(getLeftPadding(), getTopPadding(), getRightPadding(), getBottomPadding()); }
    inline qreal getTopPadding() const { return extra.isAllocated() && extra->hasTopPadding ? extra->topPadding : getVerticalPadding(); }
    inline qreal getLeftPadding() const { return extra.isAllocated() && extra->hasLeftPadding ? extra->leftPadding : getHorizontalPadding(); }
    inline qreal getRightPadding() const { return extra.isAllocated() && extra->hasRightPadding ? extra->rightPadding : getHorizontalPadding(); }
    inline qreal getBottomPadding() const { return extra.isAllocated() && extra->hasBottomPadding ? extra->bottomPadding : getVerticalPadding(); }
    inline qreal getHorizontalPadding() const { return hasHorizontalPadding ? horizontalPadding : padding; }
    inline qreal getVerticalPadding() const { return hasVerticalPadding ? verticalPadding : padding; }

    void setTopPadding(qreal value, bool reset = false);
    void setLeftPadding(qreal value, bool reset = false);
    void setRightPadding(qreal value, bool reset = false);
    void setBottomPadding(qreal value, bool reset = false);
    void setHorizontalPadding(qreal value, bool reset = false);
    void setVerticalPadding(qreal value, bool reset = false);

    inline QMarginsF getInset() const { return QMarginsF(getLeftInset(), getTopInset(), getRightInset(), getBottomInset()); }
    inline qreal getTopInset() const { return extra.isAllocated() ? extra->topInset : 0; }
    inline qreal getLeftInset() const { return extra.isAllocated() ? extra->leftInset : 0; }
    inline qreal getRightInset() const { return extra.isAllocated() ? extra->rightInset : 0; }
    inline qreal getBottomInset() const { return extra.isAllocated() ? extra->bottomInset : 0; }

    void setTopInset(qreal value, bool reset = false);
    void setLeftInset(qreal value, bool reset = false);
    void setRightInset(qreal value, bool reset = false);
    void setBottomInset(qreal value, bool reset = false);

    virtual void resizeBackground();
    virtual void resizeContent();

    virtual QQuickItem *getContentItem();
    void setContentItem_helper(QQuickItem *item, bool notify = true);

#if QT_CONFIG(accessibility)
    void accessibilityActiveChanged(bool active) override;
    QAccessible::Role accessibleRole() const override;
    static QQuickAccessibleAttached *accessibleAttached(const QObject *object);
#endif

    virtual void resolveFont();
    void inheritFont(const QFont &font);
    void updateFont(const QFont &font);
    static void updateFontRecur(QQuickItem *item, const QFont &font);
    inline void setFont_helper(const QFont &font) {
        if (resolvedFont.resolveMask() == font.resolveMask() && resolvedFont == font)
            return;
        updateFont(font);
    }
    static QFont parentFont(const QQuickItem *item);

    void updateLocale(const QLocale &l, bool e);
    static void updateLocaleRecur(QQuickItem *item, const QLocale &l);
    static QLocale calcLocale(const QQuickItem *item);

#if QT_CONFIG(quicktemplates2_hover)
    void updateHoverEnabled(bool enabled, bool xplicit);
    static void updateHoverEnabledRecur(QQuickItem *item, bool enabled);
    static bool calcHoverEnabled(const QQuickItem *item);
#endif

    static void warnIfCustomizationNotSupported(QObject *control, QQuickItem *item, const QString &propertyName);

    virtual void cancelContentItem();
    virtual void executeContentItem(bool complete = false);

    virtual void cancelBackground();
    virtual void executeBackground(bool complete = false);

    enum class UnhideVisibility {
        Show,
        Hide
    };

    static void hideOldItem(QQuickItem *item);
    static void unhideOldItem(QQuickControl *control, QQuickItem *item,
        UnhideVisibility visibility = UnhideVisibility::Show);

    void updateBaselineOffset();

    static const ChangeTypes ImplicitSizeChanges;

    void addImplicitSizeListener(QQuickItem *item, ChangeTypes changes = ImplicitSizeChanges);
    void removeImplicitSizeListener(QQuickItem *item, ChangeTypes changes = ImplicitSizeChanges);

    static void addImplicitSizeListener(QQuickItem *item, QQuickItemChangeListener *listener, ChangeTypes changes = ImplicitSizeChanges);
    static void removeImplicitSizeListener(QQuickItem *item, QQuickItemChangeListener *listener, ChangeTypes changes = ImplicitSizeChanges);

    void itemImplicitWidthChanged(QQuickItem *item) override;
    void itemImplicitHeightChanged(QQuickItem *item) override;
    void itemGeometryChanged(QQuickItem *item, QQuickGeometryChange change, const QRectF &diff) override;
    void itemDestroyed(QQuickItem *item) override;
    void itemFocusChanged(QQuickItem *item, Qt::FocusReason reason) override;

    bool setLastFocusChangeReason(Qt::FocusReason) override;

    virtual qreal getContentWidth() const;
    virtual qreal getContentHeight() const;

    void updateImplicitContentWidth();
    void updateImplicitContentHeight();
    void updateImplicitContentSize();

    QPalette defaultPalette() const override;

    struct ExtraData {
        bool hasTopPadding = false;
        bool hasLeftPadding = false;
        bool hasRightPadding = false;
        bool hasBottomPadding = false;
        bool hasBaselineOffset = false;
        bool hasTopInset = false;
        bool hasLeftInset = false;
        bool hasRightInset = false;
        bool hasBottomInset = false;
        bool hasBackgroundWidth = false;
        bool hasBackgroundHeight = false;
        qreal topPadding = 0;
        qreal leftPadding = 0;
        qreal rightPadding = 0;
        qreal bottomPadding = 0;
        qreal topInset = 0;
        qreal leftInset = 0;
        qreal rightInset = 0;
        qreal bottomInset = 0;
        QFont requestedFont;
    };
    QLazilyAllocated<ExtraData> extra;

    bool hasHorizontalPadding = false;
    bool hasVerticalPadding = false;
    bool hasLocale = false;
    bool wheelEnabled = false;
#if QT_CONFIG(quicktemplates2_hover)
    bool hovered = false;
    bool explicitHoverEnabled = false;
#endif
    bool resizingBackground = false;
    int touchId = -1;
    qreal padding = 0;
    qreal horizontalPadding = 0;
    qreal verticalPadding = 0;
    qreal implicitContentWidth = 0;
    qreal implicitContentHeight = 0;
    qreal spacing = 0;
    QLocale locale;
    QFont resolvedFont;
    QQuickDeferredPointer<QQuickItem> background;
    QQuickDeferredPointer<QQuickItem> contentItem;
};

QT_END_NAMESPACE

#endif // QQUICKCONTROL_P_P_H
