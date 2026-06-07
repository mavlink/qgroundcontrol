// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QUICKDESIGNERCUSTOMPARSEROBJECT_H
#define QUICKDESIGNERCUSTOMPARSEROBJECT_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QObject>
#include <private/qqmlcustomparser_p.h>

QT_BEGIN_NAMESPACE

class QQuickDesignerCustomParserObject : public QObject
{
    Q_OBJECT

public:
    QQuickDesignerCustomParserObject();
};

class QQuickDesignerCustomParser : public QQmlCustomParser
{
public:
    QQuickDesignerCustomParser()
        : QQmlCustomParser(AcceptsAttachedProperties | AcceptsSignalHandlers)
    {}

    void verifyBindings(
            const QQmlRefPointer<QV4::CompiledData::CompilationUnit> &compilationUnit,
            const QList<const QV4::CompiledData::Binding *> &props) override;
    void applyBindings(
            QObject *obj, const QQmlRefPointer<QV4::ExecutableCompilationUnit> &compilationUnit,
            const QList<const QV4::CompiledData::Binding *> &bindings) override;
};

QT_END_NAMESPACE

#endif // QUICKDESIGNERCUSTOMPARSEROBJECT_H
