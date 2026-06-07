// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QHELPCONTENTWIDGET_H
#define QHELPCONTENTWIDGET_H

#include <QtHelp/qhelp_global.h>
#include <QtHelp/qhelpcontentitem.h>
#include <QtWidgets/qtreeview.h>

QT_BEGIN_NAMESPACE

class QHelpContentModelPrivate;
class QHelpEngine;
class QHelpEngineCore;
class QUrl;

class QHELP_EXPORT QHelpContentModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    ~QHelpContentModel() override;

    void createContentsForCurrentFilter();
    void createContents(const QString &customFilterName);
    QHelpContentItem *contentItemAt(const QModelIndex &index) const;

    QVariant data(const QModelIndex &index, int role) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = {}) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = {}) const override;
    int columnCount(const QModelIndex &parent = {}) const override;
    bool isCreatingContents() const;

Q_SIGNALS:
    void contentsCreationStarted();
    void contentsCreated();

private Q_SLOTS:
    void insertContents();

private:
    QHelpContentModel(QHelpEngineCore *helpEngine);
    QHelpContentModelPrivate *d;
    friend class QHelpEnginePrivate;
    friend class QHelpContentModelPrivate;
};

class QHELP_EXPORT QHelpContentWidget : public QTreeView
{
    Q_OBJECT

public:
    QModelIndex indexOf(const QUrl &link);

Q_SIGNALS:
    void linkActivated(const QUrl &link);

private Q_SLOTS:
    void showLink(const QModelIndex &index);

private:
    bool searchContentItem(QHelpContentModel *model, const QModelIndex &parent,
                           const QString &path);
    QModelIndex m_syncIndex;

private:
    QHelpContentWidget();
    friend class QHelpEngine;
};

QT_END_NAMESPACE

#endif // QHELPCONTENTWIDGET_H
