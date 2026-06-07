// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of Qt Designer.  This header
// file may change from version to version without notice, or even be removed.
//
// We mean it.
//

#ifndef DYNAMICPROPERTYSHEET_H
#define DYNAMICPROPERTYSHEET_H

#include <QtDesigner/extension.h>

QT_BEGIN_NAMESPACE

class QString; // FIXME: fool syncqt

class QDesignerDynamicPropertySheetExtension
{
public:
    Q_DISABLE_COPY_MOVE(QDesignerDynamicPropertySheetExtension)

    QDesignerDynamicPropertySheetExtension() = default;
    virtual ~QDesignerDynamicPropertySheetExtension() = default;

    virtual bool dynamicPropertiesAllowed() const = 0;
    virtual int addDynamicProperty(const QString &propertyName, const QVariant &value) = 0;
    virtual bool removeDynamicProperty(int index) = 0;
    virtual bool isDynamicProperty(int index) const = 0;
    virtual bool canAddDynamicProperty(const QString &propertyName) const = 0;
};
Q_DECLARE_EXTENSION_INTERFACE(QDesignerDynamicPropertySheetExtension, "org.qt-project.Qt.Designer.DynamicPropertySheet")

QT_END_NAMESPACE

#endif // DYNAMICPROPERTYSHEET_H
