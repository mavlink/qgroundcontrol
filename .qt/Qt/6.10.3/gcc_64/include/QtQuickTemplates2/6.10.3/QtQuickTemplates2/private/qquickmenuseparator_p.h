// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKMENUSEPARATOR_P_H
#define QQUICKMENUSEPARATOR_P_H

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

#include <QtQuickTemplates2/private/qquickcontrol_p.h>
#include <QtQuickTemplates2/private/qquicktheme_p.h>

QT_BEGIN_NAMESPACE

class QQuickMenuSeparator;

class Q_QUICKTEMPLATES2_EXPORT QQuickMenuSeparator : public QQuickControl
{
    Q_OBJECT
    QML_NAMED_ELEMENT(MenuSeparator)
    QML_ADDED_IN_VERSION(2, 1)

public:
    explicit QQuickMenuSeparator(QQuickItem *parent = nullptr);

protected:
    QFont defaultFont() const override;

#if QT_CONFIG(accessibility)
    QAccessible::Role accessibleRole() const override;
#endif

private:
    Q_DISABLE_COPY(QQuickMenuSeparator)
};

QT_END_NAMESPACE

#endif // QQUICKMENUSEPARATOR_P_H
