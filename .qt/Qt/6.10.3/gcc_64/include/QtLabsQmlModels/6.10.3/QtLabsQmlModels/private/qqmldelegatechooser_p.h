// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLDELEGATECHOOSER_P_H
#define QQMLDELEGATECHOOSER_P_H

#include "qqmlmodelsglobal_p.h"

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
#include <QtQmlModels/private/qtqmlmodelsglobal_p.h>
#include <QtQmlModels/private/qqmldelegatecomponent_p.h>

QT_REQUIRE_CONFIG(qml_delegate_model);

QT_BEGIN_NAMESPACE

struct Q_LABSQMLMODELS_EXPORT QQmlDelegateChooserForeign
{
    Q_GADGET
    QML_FOREIGN(QQmlDelegateChooser)
    QML_NAMED_ELEMENT(DelegateChooser)
    QML_ADDED_IN_VERSION(1, 0)
};

struct Q_LABSQMLMODELS_EXPORT QQmlDelegateChoiceForeign
{
    Q_GADGET
    QML_FOREIGN(QQmlDelegateChoice)
    QML_NAMED_ELEMENT(DelegateChoice)
    QML_ADDED_IN_VERSION(1, 0)
};

QT_END_NAMESPACE

#endif // QQMLDELEGATECHOOSER_P_H
