// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLSCRIPTDATA_P_H
#define QQMLSCRIPTDATA_P_H

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

#include <private/qqmlrefcount_p.h>
#include <private/qqmlscriptblob_p.h>
#include <private/qv4value_p.h>
#include <private/qv4persistent_p.h>
#include <private/qv4compileddata_p.h>
#include <private/qv4scopedvalue_p.h>

#include <QtCore/qurl.h>

QT_BEGIN_NAMESPACE

class QQmlTypeNameCache;
class QQmlContextData;

class Q_AUTOTEST_EXPORT QQmlScriptData final : public QQmlRefCounted<QQmlScriptData>
{
private:
    friend class QQmlTypeLoader;

    QQmlScriptData() = default;

public:
    QUrl url;
    QString urlString;
    QQmlRefPointer<QQmlTypeNameCache> typeNameCache;
    QVector<QQmlRefPointer<QQmlScriptData>> scripts;

    QV4::ReturnedValue scriptValueForContext(const QQmlRefPointer<QQmlContextData> &parentCtxt);

    QQmlRefPointer<QV4::CompiledData::CompilationUnit> compilationUnit() const
    {
        return m_precompiledScript;
    }

private:
    friend class QQmlScriptBlob;

    QQmlRefPointer<QQmlContextData> qmlContextDataForContext(
            const QQmlRefPointer<QQmlContextData> &parentQmlContextData);

    template<typename WithExecutableCU>
    QV4::ReturnedValue handleOwnScriptValueOrExecutableCU(
            QV4::ExecutionEngine *v4,
            WithExecutableCU &&withExecutableCU) const
    {
        QV4::Scope scope(v4);

        if (!m_precompiledScript)
            return QV4::Value::emptyValue().asReturnedValue();

        return withExecutableCU(v4->executableCompilationUnit(
                QQmlRefPointer<QV4::CompiledData::CompilationUnit>(m_precompiledScript)));
    }

    QQmlRefPointer<QV4::CompiledData::CompilationUnit> m_precompiledScript;
};

QT_END_NAMESPACE

#endif // QQMLSCRIPTDATA_P_H
