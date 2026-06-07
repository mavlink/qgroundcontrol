// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKPLATFORMFONTDIALOG_P_H
#define QQUICKPLATFORMFONTDIALOG_P_H

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

class QQuickFontDialogImpl;
class QWindow;

class Q_QUICKDIALOGS2QUICKIMPL_EXPORT QQuickPlatformFontDialog
    : public QPlatformFontDialogHelper
{
    Q_OBJECT

public:
    explicit QQuickPlatformFontDialog(QObject *parent);
    ~QQuickPlatformFontDialog() = default;

    bool isValid() const;

    virtual void setCurrentFont(const QFont &font) override;
    virtual QFont currentFont() const override;

    void exec() override;
    bool show(Qt::WindowFlags flags, Qt::WindowModality modality, QWindow *parent) override;
    void hide() override;

    QQuickFontDialogImpl *dialog() const;

private:
    QQuickFontDialogImpl *m_dialog = nullptr;
};

QT_END_NAMESPACE

#endif // QQUICKPLATFORMFONTDIALOG_P_H
