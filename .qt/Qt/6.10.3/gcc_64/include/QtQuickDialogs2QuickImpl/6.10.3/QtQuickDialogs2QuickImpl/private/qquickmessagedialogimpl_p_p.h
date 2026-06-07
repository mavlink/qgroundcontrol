// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKMESSAGEDIALOGIMPL_P_P_H
#define QQUICKMESSAGEDIALOGIMPL_P_P_H

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

#include "qquickmessagedialogimpl_p.h"

#include <QtCore/qpointer.h>

QT_BEGIN_NAMESPACE

class QQuickMessageDialogImplPrivate : public QQuickDialogPrivate
{
    Q_DECLARE_PUBLIC(QQuickMessageDialogImpl)

public:
    QQuickMessageDialogImplPrivate();

    static QQuickMessageDialogImplPrivate *get(QQuickMessageDialogImpl *dialog)
    {
        return dialog->d_func();
    }

    QQuickMessageDialogImplAttached *attachedOrWarn();

    void handleClick(QQuickAbstractButton *button) override;

    QSharedPointer<QMessageDialogOptions> options;
    bool m_showDetailedText = false;
};

class QQuickMessageDialogImplAttachedPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QQuickMessageDialogImplAttached)
public:
    QPointer<QQuickDialogButtonBox> buttonBox;
    QPointer<QQuickButton> detailedTextButton;
};

QT_END_NAMESPACE

#endif // QQUICKMESSAGEDIALOGIMPL_P_P_H
