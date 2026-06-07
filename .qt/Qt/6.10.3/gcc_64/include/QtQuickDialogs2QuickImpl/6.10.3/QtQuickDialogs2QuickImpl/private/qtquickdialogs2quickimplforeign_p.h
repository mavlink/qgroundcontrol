// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QTQUICKDIALOGS2QUICKIMPLFOREIGN_P_H
#define QTQUICKDIALOGS2QUICKIMPLFOREIGN_P_H

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

#include <QtQml/qqml.h>
#include <QtQuickDialogs2Utils/private/qquickfilenamefilter_p.h>
#include <QtQuickTemplates2/private/qquickabstractbutton_p.h>
#include <QtQuickTemplates2/private/qquickcontrol_p.h>
#include <QtQuickTemplates2/private/qquickdialog_p.h>
#include <QtQuickTemplates2/private/qquickicon_p.h>
#include <QtQuickTemplates2/private/qquickpopup_p.h>

QT_BEGIN_NAMESPACE

struct QQuickFileNameFilterQuickDialogs2QuickImplForeign
{
    Q_GADGET
    QML_ANONYMOUS
    QML_FOREIGN(QQuickFileNameFilter)
    QML_ADDED_IN_VERSION(6, 2)
};

// TODO: remove these ones when not needed (QTBUG-88179)

// verticalPadding, etc.
struct QQuickControlForeign
{
    Q_GADGET
    QML_ANONYMOUS
    QML_FOREIGN(QQuickControl)
    QML_ADDED_IN_VERSION(2, 0)
};

struct QQuickAbstractButtonForeign
{
    Q_GADGET
    QML_ANONYMOUS
    QML_FOREIGN(QQuickAbstractButton)
    QML_ADDED_IN_VERSION(2, 0)
};

struct QQuickIconForeign
{
    Q_GADGET
    QML_ANONYMOUS
    QML_FOREIGN(QQuickIcon)
    QML_ADDED_IN_VERSION(6, 2)
};

// For leftInset, etc.
struct QQuickPopupForeign
{
    Q_GADGET
    QML_ANONYMOUS
    QML_FOREIGN(QQuickPopup)
    QML_ADDED_IN_VERSION(2, 0)
};

struct QQuickDialogForeign
{
    Q_GADGET
    QML_ANONYMOUS
    QML_FOREIGN(QQuickDialog)
    QML_ADDED_IN_VERSION(2, 1)
};

QT_END_NAMESPACE

#endif // QTQUICKDIALOGS2QUICKIMPLFOREIGN_P_H
