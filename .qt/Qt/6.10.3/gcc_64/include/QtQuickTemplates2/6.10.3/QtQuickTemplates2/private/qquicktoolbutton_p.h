// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKTOOLBUTTON_P_H
#define QQUICKTOOLBUTTON_P_H

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

#include <QtQuickTemplates2/private/qquickbutton_p.h>

QT_BEGIN_NAMESPACE

class QQuickToolButtonPrivate;

class Q_QUICKTEMPLATES2_EXPORT QQuickToolButton : public QQuickButton
{
    Q_OBJECT
    QML_NAMED_ELEMENT(ToolButton)
    QML_ADDED_IN_VERSION(2, 0)

public:
    explicit QQuickToolButton(QQuickItem *parent = nullptr);

protected:
    QFont defaultFont() const override;

private:
    Q_DECLARE_PRIVATE(QQuickToolButton)
};

QT_END_NAMESPACE

#endif // QQUICKTOOLBUTTON_P_H
