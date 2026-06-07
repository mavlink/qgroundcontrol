// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:significant

#ifndef QQMLJSLITERALBINDINGCHECK_P_H
#define QQMLJSLITERALBINDINGCHECK_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#include <QtCore/qglobal.h>
#include <QtQmlCompiler/qqmlsa.h>

#include <qtqmlcompilerexports.h>
#include "qqmljsvaluetypefromstringcheck_p.h"

QT_BEGIN_NAMESPACE

class QQmlJSImportVisitor;
class QQmlJSTypeResolver;

class Q_QMLCOMPILER_EXPORT LiteralBindingCheckBase : public QQmlSA::PropertyPass
{
public:
    using QQmlSA::PropertyPass::PropertyPass;

protected:
    virtual QQmlJSStructuredTypeError check(const QString &typeName, const QString &value) const = 0;

    QQmlSA::Property getProperty(const QString &propertyName, const QQmlSA::Binding &binding,
                                 const QQmlSA::Element &bindingScope) const;

    void warnOnCheckedBinding(const QQmlSA::Binding &binding, const QQmlSA::Element &propertyType);
};

class Q_QMLCOMPILER_EXPORT QQmlJSLiteralBindingCheck: public LiteralBindingCheckBase
{
public:
    QQmlJSLiteralBindingCheck(QQmlSA::PassManager *manager);

    void onBinding(const QQmlSA::Element &element, const QString &propertyName,
                   const QQmlSA::Binding &binding, const QQmlSA::Element &bindingScope,
                   const QQmlSA::Element &value) override;

private:
    QQmlJSTypeResolver *m_resolver;

protected:
    QQmlJSStructuredTypeError check(const QString &typeName, const QString &value) const override;
};

QT_END_NAMESPACE

#endif // QQMLJSLITERALBINDINGCHECK_P_H
