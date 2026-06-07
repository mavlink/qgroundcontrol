// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef ABSTRACTACTIONEDITOR_H
#define ABSTRACTACTIONEDITOR_H

#include <QtDesigner/sdk_global.h>

#include <QtWidgets/qwidget.h>

QT_BEGIN_NAMESPACE

class QDesignerFormEditorInterface;
class QDesignerFormWindowInterface;

class QDESIGNER_SDK_EXPORT QDesignerActionEditorInterface: public QWidget
{
    Q_OBJECT
public:
    explicit QDesignerActionEditorInterface(QWidget *parent, Qt::WindowFlags flags = {});
    virtual ~QDesignerActionEditorInterface();

    virtual QDesignerFormEditorInterface *core() const;

    virtual void manageAction(QAction *action) = 0;
    virtual void unmanageAction(QAction *action) = 0;

public Q_SLOTS:
    virtual void setFormWindow(QDesignerFormWindowInterface *formWindow) = 0;
};

QT_END_NAMESPACE

#endif // ABSTRACTACTIONEDITOR_H
