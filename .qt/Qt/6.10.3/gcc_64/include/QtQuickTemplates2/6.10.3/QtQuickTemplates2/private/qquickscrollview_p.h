// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKSCROLLVIEW_P_H
#define QQUICKSCROLLVIEW_P_H

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

#include <QtQuickTemplates2/private/qquickpane_p.h>
#include <QtQml/qqmllist.h>

QT_BEGIN_NAMESPACE

class QQuickScrollViewPrivate;

class Q_QUICKTEMPLATES2_EXPORT QQuickScrollView : public QQuickPane
{
    Q_OBJECT
    QML_NAMED_ELEMENT(ScrollView)
    QML_ADDED_IN_VERSION(2, 2)
    Q_PROPERTY(qreal effectiveScrollBarWidth READ effectiveScrollBarWidth NOTIFY effectiveScrollBarWidthChanged FINAL REVISION(6, 6))
    Q_PROPERTY(qreal effectiveScrollBarHeight READ effectiveScrollBarHeight NOTIFY effectiveScrollBarHeightChanged FINAL REVISION(6, 6))

public:
    explicit QQuickScrollView(QQuickItem *parent = nullptr);
    ~QQuickScrollView();
    qreal effectiveScrollBarWidth();
    qreal effectiveScrollBarHeight();

protected:
    bool childMouseEventFilter(QQuickItem *item, QEvent *event) override;
    bool eventFilter(QObject *object, QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

    void componentComplete() override;
    void contentItemChange(QQuickItem *newItem, QQuickItem *oldItem) override;
    void contentSizeChange(const QSizeF &newSize, const QSizeF &oldSize) override;

#if QT_CONFIG(accessibility)
    QAccessible::Role accessibleRole() const override;
#endif

Q_SIGNALS:
    Q_REVISION(6, 6) void effectiveScrollBarWidthChanged();
    Q_REVISION(6, 6) void effectiveScrollBarHeightChanged();

private:
    Q_DISABLE_COPY(QQuickScrollView)
    Q_DECLARE_PRIVATE(QQuickScrollView)
};

QT_END_NAMESPACE

#endif // QQUICKSCROLLVIEW_P_H
