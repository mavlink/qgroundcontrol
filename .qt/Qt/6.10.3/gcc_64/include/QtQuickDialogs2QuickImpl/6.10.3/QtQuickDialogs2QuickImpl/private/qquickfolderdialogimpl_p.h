// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKFOLDERDIALOGIMPL_P_H
#define QQUICKFOLDERDIALOGIMPL_P_H

//
//  W A R N I N G
//  -------------
//
// This folder is not part of the Qt API.  It exists purely as an
// implementation detail.  This header folder may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtQuick/private/qquicklistview_p.h>
#include <QtQuickTemplates2/private/qquickdialog_p.h>

#include "qtquickdialogs2quickimplglobal_p.h"

QT_BEGIN_NAMESPACE

class QQuickDialogButtonBox;

class QQuickFolderDialogImplAttached;
class QQuickFolderDialogImplAttachedPrivate;
class QQuickFolderDialogImplPrivate;
class QQuickFolderBreadcrumbBar;

class Q_QUICKDIALOGS2QUICKIMPL_EXPORT QQuickFolderDialogImpl : public QQuickDialog
{
    Q_OBJECT
    Q_PROPERTY(QUrl currentFolder READ currentFolder WRITE setCurrentFolder NOTIFY currentFolderChanged FINAL)
    Q_PROPERTY(QUrl selectedFolder READ selectedFolder WRITE setSelectedFolder NOTIFY selectedFolderChanged FINAL)
    QML_NAMED_ELEMENT(FolderDialogImpl)
    QML_ATTACHED(QQuickFolderDialogImplAttached)
    QML_ADDED_IN_VERSION(6, 3)

public:
    explicit QQuickFolderDialogImpl(QObject *parent = nullptr);

    static QQuickFolderDialogImplAttached *qmlAttachedProperties(QObject *object);

    QUrl currentFolder() const;
    void setCurrentFolder(const QUrl &folder);

    QUrl selectedFolder() const;
    void setSelectedFolder(const QUrl &selectedFolder);

    QSharedPointer<QFileDialogOptions> options() const;
    void setOptions(const QSharedPointer<QFileDialogOptions> &options);

    void setAcceptLabel(const QString &label);
    void setRejectLabel(const QString &label);

Q_SIGNALS:
    void currentFolderChanged(const QUrl &folderUrl);
    void selectedFolderChanged(const QUrl &folderUrl);
    void nameFiltersChanged();

private:
    void componentComplete() override;
    void itemChange(QQuickItem::ItemChange change, const QQuickItem::ItemChangeData &data) override;

    Q_DISABLE_COPY(QQuickFolderDialogImpl)
    Q_DECLARE_PRIVATE(QQuickFolderDialogImpl)
};

class Q_QUICKDIALOGS2QUICKIMPL_EXPORT QQuickFolderDialogImplAttached : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQuickListView *folderDialogListView READ folderDialogListView WRITE setFolderDialogListView NOTIFY folderDialogListViewChanged)
    Q_PROPERTY(QQuickFolderBreadcrumbBar *breadcrumbBar READ breadcrumbBar WRITE setBreadcrumbBar NOTIFY breadcrumbBarChanged)
    Q_MOC_INCLUDE(<QtQuickTemplates2/private/qquickdialogbuttonbox_p.h>)

public:
    explicit QQuickFolderDialogImplAttached(QObject *parent = nullptr);

    QQuickListView *folderDialogListView() const;
    void setFolderDialogListView(QQuickListView *folderDialogListView);

    QQuickFolderBreadcrumbBar *breadcrumbBar() const;
    void setBreadcrumbBar(QQuickFolderBreadcrumbBar *breadcrumbBar);

Q_SIGNALS:
    void folderDialogListViewChanged();
    void breadcrumbBarChanged();

private:
    Q_DISABLE_COPY(QQuickFolderDialogImplAttached)
    Q_DECLARE_PRIVATE(QQuickFolderDialogImplAttached)
};

QT_END_NAMESPACE

#endif // QQUICKFOLDERDIALOGIMPL_P_H
