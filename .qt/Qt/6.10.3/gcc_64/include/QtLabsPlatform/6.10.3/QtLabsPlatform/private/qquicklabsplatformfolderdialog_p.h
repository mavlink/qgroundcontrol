// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKLABSPLATFORMFOLDERDIALOG_P_H
#define QQUICKLABSPLATFORMFOLDERDIALOG_P_H

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

#include "qquicklabsplatformdialog_p.h"
#include <QtCore/qurl.h>
#include <QtQml/qqml.h>

#if QT_DEPRECATED_SINCE(6, 9)

QT_BEGIN_NAMESPACE

class QQuickLabsPlatformFolderDialog : public QQuickLabsPlatformDialog
{
    Q_OBJECT
    QML_NAMED_ELEMENT(FolderDialog)
    QML_EXTENDED_NAMESPACE(QFileDialogOptions)
    Q_PROPERTY(QUrl folder READ folder WRITE setFolder NOTIFY folderChanged FINAL)
    Q_PROPERTY(QUrl currentFolder READ currentFolder WRITE setCurrentFolder NOTIFY currentFolderChanged FINAL)
    Q_PROPERTY(QFileDialogOptions::FileDialogOptions options READ options WRITE setOptions RESET resetOptions NOTIFY optionsChanged FINAL)
    Q_PROPERTY(QString acceptLabel READ acceptLabel WRITE setAcceptLabel RESET resetAcceptLabel NOTIFY acceptLabelChanged FINAL)
    Q_PROPERTY(QString rejectLabel READ rejectLabel WRITE setRejectLabel RESET resetRejectLabel NOTIFY rejectLabelChanged FINAL)

public:
    explicit QQuickLabsPlatformFolderDialog(QObject *parent = nullptr);

    QUrl folder() const;
    void setFolder(const QUrl &folder);

    QUrl currentFolder() const;
    void setCurrentFolder(const QUrl &folder);

    QFileDialogOptions::FileDialogOptions options() const;
    void setOptions(QFileDialogOptions::FileDialogOptions options);
    void resetOptions();

    QString acceptLabel() const;
    void setAcceptLabel(const QString &label);
    void resetAcceptLabel();

    QString rejectLabel() const;
    void setRejectLabel(const QString &label);
    void resetRejectLabel();

Q_SIGNALS:
    void folderChanged();
    void currentFolderChanged();
    void optionsChanged();
    void acceptLabelChanged();
    void rejectLabelChanged();

protected:
    bool useNativeDialog() const override;
    void onCreate(QPlatformDialogHelper *dialog) override;
    void onShow(QPlatformDialogHelper *dialog) override;
    void accept() override;

private:
    QUrl m_folder;
    QSharedPointer<QFileDialogOptions> m_options;
};

QT_END_NAMESPACE

#endif // QT_DEPRECATED_SINCE(6, 9)

#endif // QQUICKLABSPLATFORMFOLDERDIALOG_P_H
