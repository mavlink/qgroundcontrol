// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKFILEDIALOGIMPL_P_H
#define QQUICKFILEDIALOGIMPL_P_H

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

#include <QtQuick/private/qquicklistview_p.h>
#include <QtQuickTemplates2/private/qquickdialog_p.h>

#include "qtquickdialogs2quickimplglobal_p.h"

QT_BEGIN_NAMESPACE

class QQuickComboBox;
class QQuickDialogButtonBox;
class QQuickTextField;
class QQuickLabel;

class QQuickFileDialogImplAttached;
class QQuickFileDialogImplAttachedPrivate;
class QQuickFileDialogImplPrivate;
class QQuickFileNameFilter;
class QQuickFolderBreadcrumbBar;
class QQuickSideBar;

class Q_QUICKDIALOGS2QUICKIMPL_EXPORT QQuickFileDialogImpl : public QQuickDialog
{
    Q_OBJECT
    Q_PROPERTY(QUrl currentFolder READ currentFolder WRITE setCurrentFolder NOTIFY currentFolderChanged FINAL)
    Q_PROPERTY(QUrl selectedFile READ selectedFile WRITE setSelectedFile NOTIFY selectedFileChanged FINAL)
    Q_PROPERTY(QStringList nameFilters READ nameFilters NOTIFY nameFiltersChanged FINAL)
    Q_PROPERTY(QQuickFileNameFilter *selectedNameFilter READ selectedNameFilter CONSTANT)
    Q_PROPERTY(QString fileName READ fileName WRITE setFileName NOTIFY selectedFileChanged FINAL)
    Q_PROPERTY(QString currentFolderName READ currentFolderName NOTIFY selectedFileChanged FINAL)
    QML_NAMED_ELEMENT(FileDialogImpl)
    QML_ATTACHED(QQuickFileDialogImplAttached)
    QML_ADDED_IN_VERSION(6, 2)
    Q_MOC_INCLUDE(<QtQuickDialogs2Utils/private/qquickfilenamefilter_p.h>)
    Q_MOC_INCLUDE(<QtQuickDialogs2QuickImpl/private/qquickfolderbreadcrumbbar_p.h>)
    Q_MOC_INCLUDE(<QtQuickDialogs2QuickImpl/private/qquicksidebar_p.h>)

public:
    explicit QQuickFileDialogImpl(QObject *parent = nullptr);

    static QQuickFileDialogImplAttached *qmlAttachedProperties(QObject *object);

    enum class SetReason {
        // Either user interaction or e.g. a change in ListView's currentIndex after changing its model.
        External,
        // As a result of the user setting an initial selectedFile.
        Internal
    };

    QUrl currentFolder() const;
    void setCurrentFolder(const QUrl &currentFolder, SetReason setReason = SetReason::External);

    QUrl selectedFile() const;
    void setSelectedFile(const QUrl &file);
    void setInitialCurrentFolderAndSelectedFile(const QUrl &file);

    QSharedPointer<QFileDialogOptions> options() const;
    void setOptions(const QSharedPointer<QFileDialogOptions> &options);

    QStringList nameFilters() const;
    void resetNameFilters();

    QQuickFileNameFilter *selectedNameFilter() const;

    void setAcceptLabel(const QString &label);
    void setRejectLabel(const QString &label);

    QString fileName() const;
    void setFileName(const QString &fileName);

    QString currentFolderName() const;

public Q_SLOTS:
    void selectNameFilter(const QString &filter);

Q_SIGNALS:
    void currentFolderChanged(const QUrl &folderUrl);
    void selectedFileChanged(const QUrl &selectedFileUrl);
    void nameFiltersChanged();
    void fileSelected(const QUrl &fileUrl);
    void filterSelected(const QString &filter);

private:
    void componentComplete() override;
    void itemChange(QQuickItem::ItemChange change, const QQuickItem::ItemChangeData &data) override;

    Q_DISABLE_COPY(QQuickFileDialogImpl)
    Q_DECLARE_PRIVATE(QQuickFileDialogImpl)
};

class Q_QUICKDIALOGS2QUICKIMPL_EXPORT QQuickFileDialogImplAttached : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQuickDialogButtonBox *buttonBox READ buttonBox WRITE setButtonBox NOTIFY buttonBoxChanged FINAL)
    Q_PROPERTY(QQuickComboBox *nameFiltersComboBox READ nameFiltersComboBox WRITE setNameFiltersComboBox NOTIFY nameFiltersComboBoxChanged FINAL)
    Q_PROPERTY(QQuickLabel *filterLabel READ filterLabel WRITE setFilterLabel NOTIFY filterLabelChanged FINAL)
    Q_PROPERTY(QQuickListView *fileDialogListView READ fileDialogListView WRITE setFileDialogListView NOTIFY fileDialogListViewChanged FINAL)
    Q_PROPERTY(QQuickFolderBreadcrumbBar *breadcrumbBar READ breadcrumbBar WRITE setBreadcrumbBar NOTIFY breadcrumbBarChanged FINAL)
    Q_PROPERTY(QQuickLabel *fileNameLabel READ fileNameLabel WRITE setFileNameLabel NOTIFY fileNameLabelChanged FINAL)
    Q_PROPERTY(QQuickTextField *fileNameTextField READ fileNameTextField WRITE setFileNameTextField NOTIFY fileNameTextFieldChanged FINAL)
    Q_PROPERTY(QQuickDialog *overwriteConfirmationDialog READ overwriteConfirmationDialog WRITE setOverwriteConfirmationDialog NOTIFY overwriteConfirmationDialogChanged FINAL)
    Q_PROPERTY(QQuickSideBar *sideBar READ sideBar WRITE setSideBar NOTIFY sideBarChanged FINAL)
    Q_MOC_INCLUDE(<QtQuickTemplates2/private/qquickdialogbuttonbox_p.h>)
    Q_MOC_INCLUDE(<QtQuickTemplates2/private/qquickcombobox_p.h>)
    Q_MOC_INCLUDE(<QtQuickTemplates2/private/qquicktextfield_p.h>)
    Q_MOC_INCLUDE(<QtQuickTemplates2/private/qquicklabel_p.h>)

public:
    explicit QQuickFileDialogImplAttached(QObject *parent = nullptr);

    QQuickDialogButtonBox *buttonBox() const;
    void setButtonBox(QQuickDialogButtonBox *buttonBox);

    QQuickComboBox *nameFiltersComboBox() const;
    void setNameFiltersComboBox(QQuickComboBox *nameFiltersComboBox);

    QQuickLabel *filterLabel() const;
    void setFilterLabel(QQuickLabel *label);

    QString selectedNameFilter() const;
    void selectNameFilter(const QString &filter);

    QQuickListView *fileDialogListView() const;
    void setFileDialogListView(QQuickListView *fileDialogListView);

    QQuickFolderBreadcrumbBar *breadcrumbBar() const;
    void setBreadcrumbBar(QQuickFolderBreadcrumbBar *breadcrumbBar);

    QQuickLabel *fileNameLabel() const;
    void setFileNameLabel(QQuickLabel *fileNameLabel);

    QQuickTextField *fileNameTextField() const;
    void setFileNameTextField(QQuickTextField *fileNameTextField);

    QQuickDialog *overwriteConfirmationDialog() const;
    void setOverwriteConfirmationDialog(QQuickDialog *dialog);

    QQuickSideBar *sideBar() const;
    void setSideBar(QQuickSideBar *sideBar);

Q_SIGNALS:
    void buttonBoxChanged();
    void nameFiltersComboBoxChanged();
    void filterLabelChanged();
    void fileDialogListViewChanged();
    void breadcrumbBarChanged();
    void fileNameLabelChanged();
    void fileNameTextFieldChanged();
    void overwriteConfirmationDialogChanged();
    Q_REVISION(6, 9) void sideBarChanged();

private:
    Q_DISABLE_COPY(QQuickFileDialogImplAttached)
    Q_DECLARE_PRIVATE(QQuickFileDialogImplAttached)
};

QT_END_NAMESPACE

#endif // QQUICKFILEDIALOGIMPL_P_H
