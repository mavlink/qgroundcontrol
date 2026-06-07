// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKFOLDERDIALOG_P_H
#define QQUICKFOLDERDIALOG_P_H

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

class Q_QUICKDIALOGS2_EXPORT QQuickFolderDialog : public QQuickAbstractDialog
{
    Q_OBJECT
    Q_PROPERTY(QUrl currentFolder READ currentFolder WRITE setCurrentFolder NOTIFY currentFolderChanged FINAL)
    Q_PROPERTY(QUrl selectedFolder READ selectedFolder WRITE setSelectedFolder NOTIFY selectedFolderChanged FINAL)
    Q_PROPERTY(QFileDialogOptions::FileDialogOptions options READ options WRITE setOptions RESET resetOptions NOTIFY optionsChanged FINAL)
    Q_PROPERTY(QString acceptLabel READ acceptLabel WRITE setAcceptLabel RESET resetAcceptLabel NOTIFY acceptLabelChanged FINAL)
    Q_PROPERTY(QString rejectLabel READ rejectLabel WRITE setRejectLabel RESET resetRejectLabel NOTIFY rejectLabelChanged FINAL)
    QML_EXTENDED_NAMESPACE(QFileDialogOptions)
    QML_NAMED_ELEMENT(FolderDialog)
    QML_ADDED_IN_VERSION(6, 3)

public:
    explicit QQuickFolderDialog(QObject *parent = nullptr);

    QUrl currentFolder() const;
    void setCurrentFolder(const QUrl &folder);

    QUrl selectedFolder() const;
    void setSelectedFolder(const QUrl &folder);

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
    void currentFolderChanged();
    void selectedFolderChanged();
    void optionsChanged();
    void acceptLabelChanged();
    void rejectLabelChanged();

protected:
    bool useNativeDialog() const override;
    void onCreate(QPlatformDialogHelper *dialog) override;
    void onShow(QPlatformDialogHelper *dialog) override;

private:
    QSharedPointer<QFileDialogOptions> m_options;
};

QT_END_NAMESPACE

#endif // QQUICKFOLDERDIALOG_P_H
