// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWIDGETPLATFORMDIALOG_P_H
#define QWIDGETPLATFORMDIALOG_P_H

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

#include <QtCore/qnamespace.h>
#include <QtCore/private/qglobal_p.h>

QT_BEGIN_NAMESPACE

class QDialog;
class QWindow;

class QWidgetPlatformDialog
{
public:
    static bool show(QDialog *dialog, Qt::WindowFlags flags, Qt::WindowModality modality, QWindow *parent);
};

QT_END_NAMESPACE

#endif // QWIDGETPLATFORMDIALOG_P_H
