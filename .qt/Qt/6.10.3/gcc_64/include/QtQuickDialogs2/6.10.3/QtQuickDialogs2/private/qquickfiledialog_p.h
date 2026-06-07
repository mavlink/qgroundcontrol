// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKFILEDIALOG_P_H
#define QQUICKFILEDIALOG_P_H

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

#include <QtCore/qurl.h>
#include <QtQml/qqml.h>

#include "qquickabstractdialog_p.h"

QT_BEGIN_NAMESPACE

class QQuickFileNameFilter;

class Q_QUICKDIALOGS2_EXPORT QQuickFileDialog : public QQuickAbstractDialog
{
    Q_OBJECT
    Q_PROPERTY(FileMode fileMode READ fileMode WRITE setFileMode NOTIFY fileModeChanged FINAL)
    Q_PROPERTY(QUrl selectedFile READ selectedFile WRITE setSelectedFile NOTIFY selectedFileChanged FINAL)
    Q_PROPERTY(QList<QUrl> selectedFiles READ selectedFiles NOTIFY selectedFilesChanged FINAL)
    Q_PROPERTY(QUrl currentFile READ currentFile WRITE setCurrentFile NOTIFY currentFileChanged FINAL)
    Q_PROPERTY(QList<QUrl> currentFiles READ currentFiles WRITE setCurrentFiles NOTIFY currentFilesChanged FINAL)
    Q_PROPERTY(QUrl currentFolder READ currentFolder WRITE setCurrentFolder NOTIFY currentFolderChanged FINAL)
    Q_PROPERTY(QFileDialogOptions::FileDialogOptions options READ options WRITE setOptions RESET resetOptions NOTIFY optionsChanged FINAL)
    Q_PROPERTY(QStringList nameFilters READ nameFilters WRITE setNameFilters RESET resetNameFilters NOTIFY nameFiltersChanged FINAL)
    Q_PROPERTY(QQuickFileNameFilter *selectedNameFilter READ selectedNameFilter CONSTANT)
    Q_PROPERTY(QString defaultSuffix READ defaultSuffix WRITE setDefaultSuffix RESET resetDefaultSuffix NOTIFY defaultSuffixChanged FINAL)
    Q_PROPERTY(QString acceptLabel READ acceptLabel WRITE setAcceptLabel RESET resetAcceptLabel NOTIFY acceptLabelChanged FINAL)
    Q_PROPERTY(QString rejectLabel READ rejectLabel WRITE setRejectLabel RESET resetRejectLabel NOTIFY rejectLabelChanged FINAL)
    QML_NAMED_ELEMENT(FileDialog)
    QML_EXTENDED_NAMESPACE(QFileDialogOptions)
    QML_ADDED_IN_VERSION(6, 2)
    Q_MOC_INCLUDE(<QtQuickDialogs2Utils/private/qquickfilenamefilter_p.h>)

public:
    explicit QQuickFileDialog(QObject *parent = nullptr);

    enum FileMode {
        OpenFile,
        OpenFiles,
        SaveFile
    };
    Q_ENUM(FileMode)

    FileMode fileMode() const;
    void setFileMode(FileMode fileMode);

    QUrl selectedFile() const;
    void setSelectedFile(const QUrl &selectedFile);

    QList<QUrl> selectedFiles() const;

    QUrl currentFile() const;
    void setCurrentFile(const QUrl &file);

    QList<QUrl> currentFiles() const;
    void setCurrentFiles(const QList<QUrl> &currentFiles);

    QUrl currentFolder() const;
    void setCurrentFolder(const QUrl &currentFolder);

    QFileDialogOptions::FileDialogOptions options() const;
    void setOptions(QFileDialogOptions::FileDialogOptions options);
    void resetOptions();

    QStringList nameFilters() const;
    void setNameFilters(const QStringList &filters);
    void resetNameFilters();

    QQuickFileNameFilter *selectedNameFilter() const;

    QString defaultSuffix() const;
    void setDefaultSuffix(const QString &suffix);
    void resetDefaultSuffix();

    QString acceptLabel() const;
    void setAcceptLabel(const QString &label);
    void resetAcceptLabel();

    QString rejectLabel() const;
    void setRejectLabel(const QString &label);
    void resetRejectLabel();

Q_SIGNALS:
    void fileModeChanged();
    void selectedFileChanged();
    void selectedFilesChanged();
    void currentFileChanged();
    void currentFilesChanged();
    void currentFolderChanged();
    void optionsChanged();
    void nameFiltersChanged();
    void defaultSuffixChanged();
    void acceptLabelChanged();
    void rejectLabelChanged();

protected:
    bool useNativeDialog() const override;
    void onCreate(QPlatformDialogHelper *dialog) override;
    void onShow(QPlatformDialogHelper *dialog) override;
    void onHide(QPlatformDialogHelper *dialog) override;
    void accept() override;

private:
    QUrl addDefaultSuffix(const QUrl &file) const;
    QList<QUrl> addDefaultSuffixes(const QList<QUrl> &files) const;

    void setSelectedFiles(const QList<QUrl> &selectedFiles);

    FileMode m_fileMode;
    QList<QUrl> m_selectedFiles;
    QSharedPointer<QFileDialogOptions> m_options;
    mutable QQuickFileNameFilter *m_selectedNameFilter;
};

QT_END_NAMESPACE

#endif // QQUICKFILEDIALOG_P_H
