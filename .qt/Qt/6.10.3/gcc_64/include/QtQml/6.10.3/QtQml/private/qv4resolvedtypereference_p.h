// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QV4RESOLVEDTYPEREFERNCE_P_H
#define QV4RESOLVEDTYPEREFERNCE_P_H

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

#include <QtQml/private/qtqmlglobal_p.h>
#include <QtQml/private/qqmlrefcount_p.h>
#include <QtQml/private/qqmlpropertycache_p.h>
#include <QtQml/private/qqmltype_p.h>
#include <QtQml/private/qv4compileddata_p.h>

QT_BEGIN_NAMESPACE

class QCryptographicHash;
namespace QV4 {

class ResolvedTypeReference
{
    Q_DISABLE_COPY_MOVE(ResolvedTypeReference)
public:
    ResolvedTypeReference() = default;
    ~ResolvedTypeReference()
    {
        if (m_stronglyReferencesCompilationUnit && m_compilationUnit)
            m_compilationUnit->release();
    }

    QQmlPropertyCache::ConstPtr createPropertyCache();
    bool addToHash(QCryptographicHash *hash, QHash<quintptr, QByteArray> *checksums);

    void doDynamicTypeCheck();

    QQmlType type() const { return m_type; }
    void setType(QQmlType type)  {  m_type = std::move(type); }

    QQmlRefPointer<QV4::CompiledData::CompilationUnit> compilationUnit()
    {
        return m_compilationUnit;
    }

    void setCompilationUnit(QQmlRefPointer<QV4::CompiledData::CompilationUnit> unit)
    {
        if (m_compilationUnit == unit.data())
            return;
        if (m_stronglyReferencesCompilationUnit) {
            if (m_compilationUnit)
                m_compilationUnit->release();
            m_compilationUnit = unit.take();
        } else {
            m_compilationUnit = unit.data();
        }
    }

    bool referencesCompilationUnit() const { return m_stronglyReferencesCompilationUnit; }
    void setReferencesCompilationUnit(bool doReference)
    {
        if (doReference == m_stronglyReferencesCompilationUnit)
            return;
        m_stronglyReferencesCompilationUnit = doReference;
        if (!m_compilationUnit)
            return;
        if (doReference) {
            m_compilationUnit->addref();
        } else if (m_compilationUnit->count() == 1) {
            m_compilationUnit->release();
            m_compilationUnit = nullptr;
        } else {
            m_compilationUnit->release();
        }
    }

    QQmlPropertyCache::ConstPtr typePropertyCache() const { return m_typePropertyCache; }
    void setTypePropertyCache(QQmlPropertyCache::ConstPtr cache)
    {
        m_typePropertyCache = std::move(cache);
    }

    QTypeRevision version() const { return m_version; }
    void setVersion(QTypeRevision version) { m_version = version; }

    bool isFullyDynamicType() const { return m_isFullyDynamicType; }
    void setFullyDynamicType(bool fullyDynamic) { m_isFullyDynamicType = fullyDynamic; }

private:
    QQmlType m_type;
    QQmlPropertyCache::ConstPtr m_typePropertyCache;
    QV4::CompiledData::CompilationUnit *m_compilationUnit = nullptr;

    QTypeRevision m_version = QTypeRevision::zero();
    // Types such as QQmlPropertyMap can add properties dynamically at run-time and
    // therefore cannot have a property cache installed when instantiated.
    bool m_isFullyDynamicType = false;
    bool m_stronglyReferencesCompilationUnit = true;
};

} // namespace QV4

QT_END_NAMESPACE

#endif // QV4RESOLVEDTYPEREFERNCE_P_H
