// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef ABSTRACTPROPERTYEDITOR_H
#define ABSTRACTPROPERTYEDITOR_H

#include <QtDesigner/sdk_global.h>

#include <QtWidgets/qwidget.h>

QT_BEGIN_NAMESPACE

class QDesignerFormEditorInterface;
class QString;
class QVariant;

class QDESIGNER_SDK_EXPORT QDesignerPropertyEditorInterface: public QWidget
{
    Q_OBJECT
public:
    explicit QDesignerPropertyEditorInterface(QWidget *parent, Qt::WindowFlags flags = {});
    virtual ~QDesignerPropertyEditorInterface();

    virtual QDesignerFormEditorInterface *core() const;

    virtual bool isReadOnly() const = 0;
    virtual QObject *object() const = 0;

    virtual QString currentPropertyName() const = 0;

Q_SIGNALS:
    void propertyChanged(const QString &name, const QVariant &value);

public Q_SLOTS:
    virtual void setObject(QObject *object) = 0;
    virtual void setPropertyValue(const QString &name, const QVariant &value, bool changed = true) = 0;
    virtual void setReadOnly(bool readOnly) = 0;
};

QT_END_NAMESPACE

#endif // ABSTRACTPROPERTYEDITOR_H
