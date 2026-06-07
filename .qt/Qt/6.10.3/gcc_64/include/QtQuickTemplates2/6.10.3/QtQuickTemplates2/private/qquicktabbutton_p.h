// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKTABBUTTON_P_H
#define QQUICKTABBUTTON_P_H

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

#include <QtQuickTemplates2/private/qquickabstractbutton_p.h>

QT_BEGIN_NAMESPACE

class QQuickTabButtonPrivate;

class Q_QUICKTEMPLATES2_EXPORT QQuickTabButton : public QQuickAbstractButton
{
    Q_OBJECT
    QML_NAMED_ELEMENT(TabButton)
    QML_ADDED_IN_VERSION(2, 0)

public:
    explicit QQuickTabButton(QQuickItem *parent = nullptr);

protected:
    QFont defaultFont() const override;

#if QT_CONFIG(accessibility)
    QAccessible::Role accessibleRole() const override;
#endif

private:
    Q_DECLARE_PRIVATE(QQuickTabButton)
};

QT_END_NAMESPACE

#endif // QQUICKTABBUTTON_P_H
