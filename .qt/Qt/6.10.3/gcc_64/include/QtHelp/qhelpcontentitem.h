// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QHELPCONTENTITEM_H
#define QHELPCONTENTITEM_H

#include <QtHelp/qhelp_global.h>

QT_BEGIN_NAMESPACE

class QHelpContentItemPrivate;
class QString;
class QUrl;

class QHELP_EXPORT QHelpContentItem final
{
    Q_DISABLE_COPY_MOVE(QHelpContentItem)
public:
    ~QHelpContentItem();

    QHelpContentItem *child(int row) const;
    int childCount() const;
    QString title() const;
    QUrl url() const;
    int row() const;
    QHelpContentItem *parent() const;
    int childPosition(QHelpContentItem *child) const;

private:
    QHelpContentItem(const QString &name, const QUrl &link, QHelpContentItem *parent = nullptr);

    QHelpContentItemPrivate *d;
    friend QHelpContentItem *createContentItem(const QString &, const QUrl &, QHelpContentItem *);
};

QT_END_NAMESPACE

#endif // QHELPCONTENTITEM_H
