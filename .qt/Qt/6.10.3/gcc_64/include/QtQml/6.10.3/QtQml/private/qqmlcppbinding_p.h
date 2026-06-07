// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLCPPBINDING_P_H
#define QQMLCPPBINDING_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qobject.h>
#include <QtCore/qproperty.h>
#include <QtCore/qurl.h>

#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcontext.h>
#include <QtCore/qmetaobject.h>

#include <private/qqmltypedata_p.h>
#include <private/qqmlpropertybinding_p.h>
#include <private/qqmlbinding_p.h>
#include <private/qv4qmlcontext_p.h>
#include <private/qqmlproperty_p.h>
#include <private/qqmlbinding_p.h>

QT_BEGIN_NAMESPACE

struct Q_QML_EXPORT QQmlCppBinding
{
    // TODO: this might instead be put into the QQmlEngine or QQmlAnyBinding?
    static QUntypedPropertyBinding
    createBindingForBindable(const QV4::ExecutableCompilationUnit *unit, QObject *thisObject,
                             qsizetype functionIndex, QObject *bindingTarget, int metaPropertyIndex,
                             int valueTypePropertyIndex, const QString &propertyName);

    static void createBindingForNonBindable(const QV4::ExecutableCompilationUnit *unit,
                                            QObject *thisObject, qsizetype functionIndex,
                                            QObject *bindingTarget, int metaPropertyIndex,
                                            int valueTypePropertyIndex,
                                            const QString &propertyName);

    static QUntypedPropertyBinding
    createTranslationBindingForBindable(const QQmlRefPointer<QV4::ExecutableCompilationUnit> &unit,
                                        QObject *bindingTarget, int metaPropertyIndex,
                                        const QQmlTranslation &translationData,
                                        const QString &propertyName);

    static void createTranslationBindingForNonBindable(
            const QQmlRefPointer<QV4::ExecutableCompilationUnit> &unit,
            const QQmlSourceLocation &location, const QQmlTranslation &translationData,
            QObject *thisObject, QObject *bindingTarget, int metaPropertyIndex,
            const QString &propertyName, int valueTypePropertyIndex);
};

QT_END_NAMESPACE

#endif // QQMLCPPBINDING_P_H
