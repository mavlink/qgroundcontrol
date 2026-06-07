// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKPAGE_P_H
#define QQUICKPAGE_P_H

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

class QQuickPagePrivate;

class Q_QUICKTEMPLATES2_EXPORT QQuickPage : public QQuickPane
{
    Q_OBJECT
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged RESET resetTitle FINAL)
    Q_PROPERTY(QQuickItem *header READ header WRITE setHeader NOTIFY headerChanged FINAL)
    Q_PROPERTY(QQuickItem *footer READ footer WRITE setFooter NOTIFY footerChanged FINAL)
    // 2.5 (Qt 5.12)
    Q_PROPERTY(qreal implicitHeaderWidth READ implicitHeaderWidth NOTIFY implicitHeaderWidthChanged FINAL REVISION(2, 5))
    Q_PROPERTY(qreal implicitHeaderHeight READ implicitHeaderHeight NOTIFY implicitHeaderHeightChanged FINAL REVISION(2, 5))
    Q_PROPERTY(qreal implicitFooterWidth READ implicitFooterWidth NOTIFY implicitFooterWidthChanged FINAL REVISION(2, 5))
    Q_PROPERTY(qreal implicitFooterHeight READ implicitFooterHeight NOTIFY implicitFooterHeightChanged FINAL REVISION(2, 5))
    QML_NAMED_ELEMENT(Page)
    QML_ADDED_IN_VERSION(2, 0)

public:
    explicit QQuickPage(QQuickItem *parent = nullptr);
    ~QQuickPage();

    QString title() const;
    void setTitle(const QString &title);
    void resetTitle();

    QQuickItem *header() const;
    void setHeader(QQuickItem *header);

    QQuickItem *footer() const;
    void setFooter(QQuickItem *footer);

    // 2.5 (Qt 5.12)
    qreal implicitHeaderWidth() const;
    qreal implicitHeaderHeight() const;

    qreal implicitFooterWidth() const;
    qreal implicitFooterHeight() const;

Q_SIGNALS:
    void titleChanged();
    void headerChanged();
    void footerChanged();
    // 2.5 (Qt 5.12)
    void implicitHeaderWidthChanged();
    void implicitHeaderHeightChanged();
    void implicitFooterWidthChanged();
    void implicitFooterHeightChanged();

protected:
    QQuickPage(QQuickPagePrivate &dd, QQuickItem *parent);

    void componentComplete() override;

    void spacingChange(qreal newSpacing, qreal oldSpacing) override;

#if QT_CONFIG(accessibility)
    QAccessible::Role accessibleRole() const override;
    void accessibilityActiveChanged(bool active) override;
#endif

private:
    Q_DISABLE_COPY(QQuickPage)
    Q_DECLARE_PRIVATE(QQuickPage)
};

QT_END_NAMESPACE

#endif // QQUICKPAGE_P_H
