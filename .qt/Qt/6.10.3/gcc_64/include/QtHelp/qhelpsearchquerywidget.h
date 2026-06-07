// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QHELPSEARCHQUERYWIDGET_H
#define QHELPSEARCHQUERYWIDGET_H

#include <QtHelp/qhelp_global.h>
#include <QtHelp/qhelpsearchengine.h>

#include <QtWidgets/qwidget.h>

QT_BEGIN_NAMESPACE

class QFocusEvent;
class QHelpSearchQueryWidgetPrivate;

class QHELP_EXPORT QHelpSearchQueryWidget : public QWidget
{
    Q_OBJECT

public:
    explicit QHelpSearchQueryWidget(QWidget *parent = nullptr);
    ~QHelpSearchQueryWidget() override;

    void expandExtendedSearch();
    void collapseExtendedSearch();

#if QT_DEPRECATED_SINCE(5, 9)
    QT_DEPRECATED QList<QHelpSearchQuery> query() const;
    QT_DEPRECATED void setQuery(const QList<QHelpSearchQuery> &queryList);
#endif

    QString searchInput() const;
    void setSearchInput(const QString &searchInput);

    bool isCompactMode() const;

public Q_SLOTS:
    void setCompactMode(bool on);

Q_SIGNALS:
    void search();

private:
    void focusInEvent(QFocusEvent *focusEvent) override;
    void changeEvent(QEvent *event) override;

private:
    QHelpSearchQueryWidgetPrivate *d;
};

QT_END_NAMESPACE

#endif // QHELPSEARCHQUERYWIDGET_H
