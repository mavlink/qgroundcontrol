// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef ABSTRACTOBJECTINSPECTOR_H
#define ABSTRACTOBJECTINSPECTOR_H

#include <QtDesigner/sdk_global.h>

#include <QtWidgets/qwidget.h>

QT_BEGIN_NAMESPACE

class QDesignerFormEditorInterface;
class QDesignerFormWindowInterface;

class QDESIGNER_SDK_EXPORT QDesignerObjectInspectorInterface: public QWidget
{
    Q_OBJECT
public:
    explicit QDesignerObjectInspectorInterface(QWidget *parent, Qt::WindowFlags flags = {});
    virtual ~QDesignerObjectInspectorInterface();

    virtual QDesignerFormEditorInterface *core() const;

public Q_SLOTS:
    virtual void setFormWindow(QDesignerFormWindowInterface *formWindow) = 0;
};

QT_END_NAMESPACE

#endif // ABSTRACTOBJECTINSPECTOR_H
