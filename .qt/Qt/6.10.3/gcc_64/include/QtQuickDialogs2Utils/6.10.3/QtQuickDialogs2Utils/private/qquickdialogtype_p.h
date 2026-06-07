// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKDIALOGTYPE_P_H
#define QQUICKDIALOGTYPE_P_H

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

// We need our own type for the extra FolderDialog value, so that we can load FolderDialog.qml,
// otherwise we would just use QPlatformTheme::DialogType and then we wouldn't need this.
enum class QQuickDialogType {
    FileDialog,
    ColorDialog,
    FontDialog,
    MessageDialog,
    FolderDialog
};

QT_END_NAMESPACE

#endif // QQUICKDIALOGTYPE_P_H
