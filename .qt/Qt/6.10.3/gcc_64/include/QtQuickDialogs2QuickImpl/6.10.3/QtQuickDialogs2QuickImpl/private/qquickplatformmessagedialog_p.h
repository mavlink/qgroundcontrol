// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKPLATFORMMESSAGEDIALOG_P_H
#define QQUICKPLATFORMMESSAGEDIALOG_P_H

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

#include <QtGui/qpa/qplatformdialoghelper.h>

#include "qtquickdialogs2quickimplglobal_p.h"

QT_BEGIN_NAMESPACE

class QQuickMessageDialogImpl;

class Q_QUICKDIALOGS2QUICKIMPL_EXPORT QQuickPlatformMessageDialog
    : public QPlatformMessageDialogHelper
{
    Q_OBJECT

public:
    explicit QQuickPlatformMessageDialog(QObject *parent);
    ~QQuickPlatformMessageDialog() = default;

    bool isValid() const;

    void exec() override;
    bool show(Qt::WindowFlags flags, Qt::WindowModality modality, QWindow *parent) override;
    void hide() override;

    QQuickMessageDialogImpl *dialog() const;

private:
    QQuickMessageDialogImpl *m_dialog = nullptr;
};

QT_END_NAMESPACE

#endif // QQUICKPLATFORMMESSAGEDIALOG_P_H
