// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKPLATFORMCOLORDIALOG_P_H
#define QQUICKPLATFORMCOLORDIALOG_P_H

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

class QQuickColorDialogImpl;
class QWindow;

class Q_QUICKDIALOGS2QUICKIMPL_EXPORT QQuickPlatformColorDialog
    : public QPlatformColorDialogHelper
{
    Q_OBJECT

public:
    explicit QQuickPlatformColorDialog(QObject *parent);
    ~QQuickPlatformColorDialog() = default;

    bool isValid() const;

    virtual void setCurrentColor(const QColor &color) override;
    virtual QColor currentColor() const override;

    void exec() override;
    bool show(Qt::WindowFlags flags, Qt::WindowModality modality, QWindow *parent) override;
    void hide() override;

    QQuickColorDialogImpl *dialog() const;

private:
    QQuickColorDialogImpl *m_dialog = nullptr;
};

QT_END_NAMESPACE

#endif // QQUICKPLATFORMCOLORDIALOG_P_H
