// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKFILEDIALOG_P_P_H
#define QQUICKFILEDIALOG_P_P_H

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

#include <QtQuickTemplates2/private/qquickcombobox_p.h>
#include <QtQuickTemplates2/private/qquickdialog_p_p.h>
#include <QtQuickTemplates2/private/qquickdialogbuttonbox_p.h>
#include <QtQuickTemplates2/private/qquicklabel_p.h>
#include <QtQuickTemplates2/private/qquicktextfield_p.h>

#include "qquickfiledialogimpl_p.h"

#include <QtCore/qpointer.h>

QT_BEGIN_NAMESPACE

class QQuickFileNameFilter;

class Q_QUICKDIALOGS2QUICKIMPL_EXPORT QQuickFileDialogImplPrivate : public QQuickDialogPrivate
{
public:
    Q_DECLARE_PUBLIC(QQuickFileDialogImpl)

    QQuickFileDialogImplPrivate();

    static QQuickFileDialogImplPrivate *get(QQuickFileDialogImpl *dialog)
    {
        return dialog->d_func();
    }

    QQuickFileDialogImplAttached *attachedOrWarn();

    void setNameFilters(const QStringList &filters);

    void updateEnabled();
    void updateSelectedFile(const QString &oldFolderPath);
    void updateFileNameTextEdit();
    static QDir::SortFlags fileListSortFlags();
    static QFileInfoList fileList(const QDir &dir);
    void setFileDialogListViewCurrentIndex(int newCurrentIndex);
    void tryUpdateFileDialogListViewCurrentIndex(int newCurrentIndex);
    void fileDialogListViewCountChanged();

    void handleAccept() override;
    void handleClick(QQuickAbstractButton *button) override;
    void selectFile();

    QSharedPointer<QFileDialogOptions> options;
    QUrl currentFolder;
    QUrl selectedFile;
    QQuickAbstractButton *lastButtonClicked = nullptr;
    QStringList nameFilters;
    mutable QQuickFileNameFilter *selectedNameFilter = nullptr;
    QString acceptLabel;
    QString rejectLabel;
    bool setCurrentIndexToInitiallySelectedFile = false;
    QFileInfoList cachedFileList;
    int pendingCurrentIndexToSet = -1;
};

class QQuickFileDialogImplAttachedPrivate : public QObjectPrivate
{
    void nameFiltersComboBoxItemActivated(int index);
    void fileDialogListViewCurrentIndexChanged();
    void fileNameEditedByUser();
    void fileNameEditingByUserFinished();

public:
    Q_DECLARE_PUBLIC(QQuickFileDialogImplAttached)

    QPointer<QQuickDialogButtonBox> buttonBox;
    QPointer<QQuickComboBox> nameFiltersComboBox;
    QPointer<QQuickLabel> filterLabel;
    QPointer<QQuickListView> fileDialogListView;
    QPointer<QQuickFolderBreadcrumbBar> breadcrumbBar;
    QPointer<QQuickLabel> fileNameLabel;
    QPointer<QQuickTextField> fileNameTextField;
    QPointer<QQuickDialog> overwriteConfirmationDialog;
    QPointer<QQuickSideBar> sideBar;
};

QT_END_NAMESPACE

#endif // QQUICKFILEDIALOG_P_P_H
