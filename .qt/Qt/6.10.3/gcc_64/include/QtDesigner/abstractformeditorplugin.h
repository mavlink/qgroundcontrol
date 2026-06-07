// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef ABSTRACTFORMEDITORPLUGIN_H
#define ABSTRACTFORMEDITORPLUGIN_H

#include <QtDesigner/sdk_global.h>

#include <QtCore/qobject.h>

QT_BEGIN_NAMESPACE

class QDesignerFormEditorInterface;
class QAction;

class QDESIGNER_SDK_EXPORT QDesignerFormEditorPluginInterface
{
public:
    Q_DISABLE_COPY_MOVE(QDesignerFormEditorPluginInterface)

    QDesignerFormEditorPluginInterface() = default;
    virtual ~QDesignerFormEditorPluginInterface() = default;

    virtual bool isInitialized() const = 0;
    virtual void initialize(QDesignerFormEditorInterface *core) = 0;
    virtual QAction *action() const = 0;

    virtual QDesignerFormEditorInterface *core() const = 0;
};
Q_DECLARE_INTERFACE(QDesignerFormEditorPluginInterface, "org.qt-project.Qt.Designer.QDesignerFormEditorPluginInterface")

QT_END_NAMESPACE

#endif // ABSTRACTFORMEDITORPLUGIN_H
