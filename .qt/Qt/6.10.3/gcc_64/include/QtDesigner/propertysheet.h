// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef PROPERTYSHEET_H
#define PROPERTYSHEET_H

#include <QtDesigner/extension.h>

QT_BEGIN_NAMESPACE

class QVariant;

class QDesignerPropertySheetExtension
{
public:
    Q_DISABLE_COPY_MOVE(QDesignerPropertySheetExtension)

    QDesignerPropertySheetExtension() = default;
    virtual ~QDesignerPropertySheetExtension() = default;

    virtual int count() const = 0;

    virtual int indexOf(const QString &name) const = 0;

    virtual QString propertyName(int index) const = 0;
    virtual QString propertyGroup(int index) const = 0;
    virtual void setPropertyGroup(int index, const QString &group) = 0;

    virtual bool hasReset(int index) const = 0;
    virtual bool reset(int index) = 0;

    virtual bool isVisible(int index) const = 0;
    virtual void setVisible(int index, bool b) = 0;

    virtual bool isAttribute(int index) const = 0;
    virtual void setAttribute(int index, bool b) = 0;

    virtual QVariant property(int index) const = 0;
    virtual void setProperty(int index, const QVariant &value) = 0;

    virtual bool isChanged(int index) const = 0;
    virtual void setChanged(int index, bool changed) = 0;

    virtual bool isEnabled(int index) const = 0;
};

Q_DECLARE_EXTENSION_INTERFACE(QDesignerPropertySheetExtension,
    "org.qt-project.Qt.Designer.PropertySheet")

QT_END_NAMESPACE

#endif // PROPERTYSHEET_H
