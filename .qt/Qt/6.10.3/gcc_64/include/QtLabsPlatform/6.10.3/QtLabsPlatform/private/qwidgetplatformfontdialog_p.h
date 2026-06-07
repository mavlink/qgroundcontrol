// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWIDGETPLATFORMFONTDIALOG_P_H
#define QWIDGETPLATFORMFONTDIALOG_P_H

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
#include <QtCore/private/qglobal_p.h>

QT_BEGIN_NAMESPACE

class QFontDialog;

class QWidgetPlatformFontDialog : public QPlatformFontDialogHelper
{
    Q_OBJECT

public:
    explicit QWidgetPlatformFontDialog(QObject *parent = nullptr);
    ~QWidgetPlatformFontDialog();

    QFont currentFont() const override;
    void setCurrentFont(const QFont &font) override;

    void exec() override;
    bool show(Qt::WindowFlags windowFlags, Qt::WindowModality windowModality, QWindow *parent) override;
    void hide() override;

private:
    QScopedPointer<QFontDialog> m_dialog;
};

QT_END_NAMESPACE

#endif // QWIDGETPLATFORMFONTDIALOG_P_H
