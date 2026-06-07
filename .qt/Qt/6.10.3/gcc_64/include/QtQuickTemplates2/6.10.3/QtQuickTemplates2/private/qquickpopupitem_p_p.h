// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKPOPUPITEM_P_P_H
#define QQUICKPOPUPITEM_P_P_H

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

#include <QtQuickTemplates2/private/qquickpage_p.h>
#include <QtQuickTemplates2/private/qquickpage_p_p.h>

QT_BEGIN_NAMESPACE

class QQuickPopup;
class QQuickPopupItemPrivate;

class Q_QUICKTEMPLATES2_EXPORT QQuickPopupItem : public QQuickPage
{
    Q_OBJECT

public:
    explicit QQuickPopupItem(QQuickPopup *popup);

protected:
    void updatePolish() override;

    bool childMouseEventFilter(QQuickItem *child, QEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mouseUngrabEvent() override;
#if QT_CONFIG(quicktemplates2_multitouch)
    void touchEvent(QTouchEvent *event) override;
    void touchUngrabEvent() override;
#endif
#if QT_CONFIG(wheelevent)
    void wheelEvent(QWheelEvent *event) override;
#endif

    void contentItemChange(QQuickItem *newItem, QQuickItem *oldItem) override;
    void contentSizeChange(const QSizeF &newSize, const QSizeF &oldSize) override;
    void fontChange(const QFont &newFont, const QFont &oldFont) override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    void localeChange(const QLocale &newLocale, const QLocale &oldLocale) override;
    void mirrorChange() override;
    void itemChange(ItemChange change, const ItemChangeData &data) override;
    void paddingChange(const QMarginsF &newPadding, const QMarginsF &oldPadding) override;
    void enabledChange() override;

    QFont defaultFont() const override;

#if QT_CONFIG(accessibility)
    QAccessible::Role accessibleRole() const override;
    void accessibilityActiveChanged(bool active) override;
#endif

private:
    Q_DISABLE_COPY(QQuickPopupItem)
    Q_DECLARE_PRIVATE(QQuickPopupItem)
    friend class QQuickPopup;
};

class Q_QUICKTEMPLATES2_EXPORT QQuickPopupItemPrivate : public QQuickPagePrivate
{
    Q_DECLARE_PUBLIC(QQuickPopupItem)

public:
    QQuickPopupItemPrivate(QQuickPopup *popup);

    static QQuickPopupItemPrivate *get(QQuickPopupItem *popupItem);

    void implicitWidthChanged() override;
    void implicitHeightChanged() override;

    void resolveFont() override;

    QQuickItem *getContentItem() override;

    void cancelContentItem() override;
    void executeContentItem(bool complete = false) override;

    void cancelBackground() override;
    void executeBackground(bool complete = false) override;

    QQuickPalette *palette() const override;
    void setPalette(QQuickPalette* p) override;
    void resetPalette() override;

    QPalette defaultPalette() const override;
    bool providesPalette() const override;

    QPalette parentPalette(const QPalette &fallbackPalette) const override;

    int backId = 0;
    int escapeId = 0;
    QQuickPopup *popup = nullptr;
};

QT_END_NAMESPACE

#endif // QQUICKPOPUPITEM_P_P_H
