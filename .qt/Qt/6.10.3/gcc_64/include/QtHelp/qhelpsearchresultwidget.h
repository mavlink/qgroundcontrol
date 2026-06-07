// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QHELPSEARCHRESULTWIDGET_H
#define QHELPSEARCHRESULTWIDGET_H

#include <QtHelp/qhelp_global.h>
#include <QtHelp/qhelpsearchengine.h>

#include <QtWidgets/qwidget.h>

QT_BEGIN_NAMESPACE

class QHelpSearchResultWidgetPrivate;
class QPoint;
class QUrl;

class QHELP_EXPORT QHelpSearchResultWidget : public QWidget
{
    Q_OBJECT

public:
    ~QHelpSearchResultWidget() override;
    QUrl linkAt(const QPoint &point);

Q_SIGNALS:
    void requestShowLink(const QUrl &url);

private:
    friend class QHelpSearchEngine;

    QHelpSearchResultWidgetPrivate *d;
    QHelpSearchResultWidget(QHelpSearchEngine *engine);
    void changeEvent(QEvent *event) override;
};

QT_END_NAMESPACE

#endif // QHELPSEARCHRESULTWIDGET_H
