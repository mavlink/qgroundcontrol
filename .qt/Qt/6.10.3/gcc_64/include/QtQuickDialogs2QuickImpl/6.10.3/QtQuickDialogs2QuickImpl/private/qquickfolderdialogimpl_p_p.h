// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKFOLDERDIALOG_P_P_H
#define QQUICKFOLDERDIALOG_P_P_H

//
//  W A R N I N G
//  -------------
//
// This Folder is not part of the Qt API.  It exists purely as an
// implementation detail.  This header Folder may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtQuickTemplates2/private/qquickdialog_p_p.h>
#include <QtQuickTemplates2/private/qquickdialogbuttonbox_p.h>

#include "qquickfolderdialogimpl_p.h"

#include <QtCore/qpointer.h>

QT_BEGIN_NAMESPACE

class QQuickFolderDialogImplPrivate : public QQuickDialogPrivate
{
    Q_DECLARE_PUBLIC(QQuickFolderDialogImpl)

public:
    QQuickFolderDialogImplPrivate();

    static QQuickFolderDialogImplPrivate *get(QQuickFolderDialogImpl *dialog)
    {
        return dialog->d_func();
    }

    QQuickFolderDialogImplAttached *attachedOrWarn();

    void updateEnabled();
    void updateSelectedFolder(const QString &oldFolderPath);

    void handleAccept() override;
    void handleClick(QQuickAbstractButton *button) override;

    QSharedPointer<QFileDialogOptions> options;
    QUrl currentFolder;
    QUrl selectedFolder;
    QString acceptLabel;
    QString rejectLabel;
};

class QQuickFolderDialogImplAttachedPrivate : public QObjectPrivate
{
public:
    Q_DECLARE_PUBLIC(QQuickFolderDialogImplAttached)

    void folderDialogListViewCurrentIndexChanged();

    QPointer<QQuickDialogButtonBox> buttonBox;
    QPointer<QQuickListView> folderDialogListView;
    QPointer<QQuickFolderBreadcrumbBar> breadcrumbBar;
};

QT_END_NAMESPACE

#endif // QQUICKFOLDERDIALOG_P_P_H
