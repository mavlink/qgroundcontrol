// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKDIALOG_P_P_H
#define QQUICKDIALOG_P_P_H

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

#include <QtQuickTemplates2/private/qquickdialog_p.h>
#include <QtQuickTemplates2/private/qquickpopup_p_p.h>
#include <QtGui/qpa/qplatformdialoghelper.h>

QT_BEGIN_NAMESPACE

class QQuickAbstractButton;
class QQuickDialogButtonBox;

class Q_QUICKTEMPLATES2_EXPORT QQuickDialogPrivate : public QQuickPopupPrivate
{
public:
    Q_DECLARE_PUBLIC(QQuickDialog)
    QQuickDialogPrivate() { windowFlags = Qt::Dialog;}

    static QQuickDialogPrivate *get(QQuickDialog *dialog)
    {
        return dialog->d_func();
    }

    static QPlatformDialogHelper::ButtonRole buttonRole(QQuickAbstractButton *button);

    virtual void handleAccept();
    virtual void handleReject();
    virtual void handleClick(QQuickAbstractButton *button);

    int result = 0;
    QQuickDialogButtonBox *buttonBox = nullptr;
    QPlatformDialogHelper::StandardButtons standardButtons = QPlatformDialogHelper::NoButton;
};

QT_END_NAMESPACE

#endif // QQUICKDIALOG_P_P_H
