// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKPLATFORMFOLDERDIALOG_P_H
#define QQUICKPLATFORMFOLDERDIALOG_P_H

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

#include <QtGui/qpa/qplatformdialoghelper.h>

#include "qquickfolderdialogimpl_p.h"
#include "qtquickdialogs2quickimplglobal_p.h"

QT_BEGIN_NAMESPACE

class QQuickFileDialogImpl;
class QWindow;

class Q_QUICKDIALOGS2QUICKIMPL_EXPORT QQuickPlatformFolderDialog : public QPlatformFileDialogHelper
{
    Q_OBJECT

public:
    explicit QQuickPlatformFolderDialog(QObject *parent);
    ~QQuickPlatformFolderDialog() = default;

    bool isValid() const;
    bool defaultNameFilterDisables() const override;
    void setDirectory(const QUrl &directory) override;
    QUrl directory() const override;
    void selectFile(const QUrl &file) override;
    QList<QUrl> selectedFiles() const override;
    void setFilter() override;
    void selectNameFilter(const QString &filter) override;
    QString selectedNameFilter() const override;

    void exec() override;
    bool show(Qt::WindowFlags flags, Qt::WindowModality modality, QWindow *parent) override;
    void hide() override;

    QQuickFolderDialogImpl *dialog() const;

private:
    QQuickFolderDialogImpl *m_dialog = nullptr;
};

QT_END_NAMESPACE

#endif // QQUICKPLATFORMFOLDERDIALOG_P_H
