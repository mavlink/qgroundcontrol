// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef TASKMENU_H
#define TASKMENU_H

#include <QtDesigner/sdk_global.h>
#include <QtDesigner/extension.h>

QT_BEGIN_NAMESPACE

class QAction;

class QDESIGNER_SDK_EXPORT QDesignerTaskMenuExtension
{
public:
    Q_DISABLE_COPY_MOVE(QDesignerTaskMenuExtension)

    QDesignerTaskMenuExtension() = default;
    virtual ~QDesignerTaskMenuExtension();

    virtual QAction *preferredEditAction() const;

    virtual QList<QAction*> taskActions() const = 0;
};
Q_DECLARE_EXTENSION_INTERFACE(QDesignerTaskMenuExtension, "org.qt-project.Qt.Designer.TaskMenu")

QT_END_NAMESPACE

#endif // TASKMENU_H
