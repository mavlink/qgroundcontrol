// Copyright (C) 2015 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Pelagicore AG
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QEGLFSKMSGBMWINDOW_H
#define QEGLFSKMSGBMWINDOW_H

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

#include "private/qeglfswindow_p.h"

QT_BEGIN_NAMESPACE

class QEglFSKmsGbmIntegration;

class Q_EGLFS_EXPORT QEglFSKmsGbmWindow : public QEglFSWindow
{
public:
    QEglFSKmsGbmWindow(QWindow *w, const QEglFSKmsGbmIntegration *integration)
        : QEglFSWindow(w),
          m_integration(integration)
    { }

    ~QEglFSKmsGbmWindow() { destroy(); }

    void resetSurface() override;
    void invalidateSurface() override;

private:
    const QEglFSKmsGbmIntegration *m_integration;
};

QT_END_NAMESPACE

#endif // QEGLFSKMSGBMWINDOW_H
