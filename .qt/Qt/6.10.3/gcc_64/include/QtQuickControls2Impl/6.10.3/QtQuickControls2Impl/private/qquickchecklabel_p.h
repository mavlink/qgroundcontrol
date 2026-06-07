// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKCHECKLABEL_P_H
#define QQUICKCHECKLABEL_P_H

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

#include <QtQuick/private/qquicktext_p.h>
#include <QtQuickControls2Impl/private/qtquickcontrols2implglobal_p.h>

QT_BEGIN_NAMESPACE

class Q_QUICKCONTROLS2IMPL_EXPORT QQuickCheckLabel : public QQuickText
{
    Q_OBJECT
    QML_NAMED_ELEMENT(CheckLabel)
    QML_ADDED_IN_VERSION(2, 3)

public:
    explicit QQuickCheckLabel(QQuickItem *parent = nullptr);
};

struct QQuickTextForeign
{
    Q_GADGET
    QML_ANONYMOUS
    QML_FOREIGN(QQuickText)
    QML_ADDED_IN_VERSION(2, 3)
};

QT_END_NAMESPACE

#endif // QQUICKCHECKLABEL_P_H
