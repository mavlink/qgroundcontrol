// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKLABEL_P_P_H
#define QQUICKLABEL_P_P_H

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

#include <QtQml/private/qlazilyallocated_p.h>
#include <QtQuick/private/qquicktext_p_p.h>
#include <QtQuick/private/qquickitemchangelistener_p.h>
#include <QtQuickTemplates2/private/qquickdeferredpointer_p_p.h>
#include <QtQuickTemplates2/private/qquicktheme_p.h>

#if QT_CONFIG(accessibility)
#include <QtGui/qaccessible.h>
#endif

QT_BEGIN_NAMESPACE

class Q_AUTOTEST_EXPORT QQuickLabelPrivate : public QQuickTextPrivate,
                                             public QSafeQuickItemChangeListener<QQuickLabelPrivate>
#if QT_CONFIG(accessibility)
    , public QAccessible::ActivationObserver
#endif
{
public:
    Q_DECLARE_PUBLIC(QQuickLabel)

    QQuickLabelPrivate();
    ~QQuickLabelPrivate();

    static QQuickLabelPrivate *get(QQuickLabel *item)
    {
        return static_cast<QQuickLabelPrivate *>(QObjectPrivate::get(item));
    }

    inline QMarginsF getInset() const { return QMarginsF(getLeftInset(), getTopInset(), getRightInset(), getBottomInset()); }
    inline qreal getTopInset() const { return extra.isAllocated() ? extra->topInset : 0; }
    inline qreal getLeftInset() const { return extra.isAllocated() ? extra->leftInset : 0; }
    inline qreal getRightInset() const { return extra.isAllocated() ? extra->rightInset : 0; }
    inline qreal getBottomInset() const { return extra.isAllocated() ? extra->bottomInset : 0; }

    void setTopInset(qreal value, bool reset = false);
    void setLeftInset(qreal value, bool reset = false);
    void setRightInset(qreal value, bool reset = false);
    void setBottomInset(qreal value, bool reset = false);

    void resizeBackground();

    void resolveFont();
    void inheritFont(const QFont &font);
    void updateFont(const QFont &font);
    inline void setFont_helper(const QFont &font) {
        if (sourceFont.resolveMask() == font.resolveMask() && sourceFont == font)
            return;
        updateFont(font);
    }

    void textChanged(const QString &text);

#if QT_CONFIG(accessibility)
    void accessibilityActiveChanged(bool active) override;
    QAccessible::Role accessibleRole() const override;
    void maybeSetAccessibleName(const QString &name);
#endif

    void cancelBackground();
    void executeBackground(bool complete = false);

    void itemGeometryChanged(QQuickItem *item, QQuickGeometryChange change, const QRectF &diff) override;
    void itemImplicitWidthChanged(QQuickItem *item) override;
    void itemImplicitHeightChanged(QQuickItem *item) override;
    void itemDestroyed(QQuickItem *item) override;

    QPalette defaultPalette() const override;

    struct ExtraData {
        bool hasTopInset = false;
        bool hasLeftInset = false;
        bool hasRightInset = false;
        bool hasBottomInset = false;
        bool hasBackgroundWidth = false;
        bool hasBackgroundHeight = false;
        qreal topInset = 0;
        qreal leftInset = 0;
        qreal rightInset = 0;
        qreal bottomInset = 0;
        QFont requestedFont;
    };
    QLazilyAllocated<ExtraData> extra;

    bool resizingBackground = false;
    QPalette resolvedPalette;
    QQuickDeferredPointer<QQuickItem> background;
};

QT_END_NAMESPACE

#endif // QQUICKLABEL_P_P_H
