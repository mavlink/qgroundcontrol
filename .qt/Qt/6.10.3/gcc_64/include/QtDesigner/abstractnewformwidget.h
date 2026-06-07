// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef ABSTRACTNEWFORMWIDGET_H
#define ABSTRACTNEWFORMWIDGET_H

#include <QtDesigner/sdk_global.h>

#include <QtWidgets/qwidget.h>

QT_BEGIN_NAMESPACE

class QDesignerFormEditorInterface;

class QDESIGNER_SDK_EXPORT QDesignerNewFormWidgetInterface : public QWidget
{
    Q_OBJECT
public:
    explicit QDesignerNewFormWidgetInterface(QWidget *parent = nullptr);
    virtual ~QDesignerNewFormWidgetInterface();

    virtual bool hasCurrentTemplate() const = 0;
    virtual QString currentTemplate(QString *errorMessage = nullptr) = 0;

    static QDesignerNewFormWidgetInterface *createNewFormWidget(QDesignerFormEditorInterface *core, QWidget *parent = nullptr);

Q_SIGNALS:
    void templateActivated();
    void currentTemplateChanged(bool templateSelected);
};

QT_END_NAMESPACE

#endif // ABSTRACTNEWFORMWIDGET_H
