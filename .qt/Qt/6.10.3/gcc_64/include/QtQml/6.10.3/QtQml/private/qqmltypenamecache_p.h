// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLTYPENAMECACHE_P_H
#define QQMLTYPENAMECACHE_P_H

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
#include "qqmlmetatype_p.h"

#include <private/qstringhash_p.h>
#include <private/qqmlimport_p.h>
#include <private/qqmltypemoduleversion_p.h>

#include <QtCore/qvector.h>

QT_BEGIN_NAMESPACE

struct QQmlImportRef {
    inline QQmlImportRef()
        : scriptIndex(-1)
    {}
    // Imported module
    QVector<QQmlTypeModuleVersion> modules;

    // Or, imported script
    int scriptIndex;

    // Or, imported compositeSingletons
    QStringHash<QUrl> compositeSingletons;

    // The qualifier of this import
    QString m_qualifier;
};

class QQmlType;
class QQmlEngine;
class Q_QML_EXPORT QQmlTypeNameCache final : public QQmlRefCounted<QQmlTypeNameCache>
{
public:
    QQmlTypeNameCache(const QQmlRefPointer<QQmlImports> &imports) : m_imports(imports) {}
    ~QQmlTypeNameCache() {}

    inline bool isEmpty() const;

    void add(const QHashedString &name, int sciptIndex = -1, const QHashedString &nameSpace = QHashedString());
    void add(const QHashedString &name, const QUrl &url, const QHashedString &nameSpace = QHashedString());

    struct Result {
        inline Result();
        inline Result(const QQmlImportRef *importNamespace);
        inline Result(const QQmlType &type);
        inline Result(int scriptIndex);

        inline bool isValid() const;

        QQmlType type;
        const QQmlImportRef *importNamespace;
        int scriptIndex;
    };

    enum class QueryNamespaced { No, Yes };

    // Restrict the types allowed for key. We don't want QV4::ScopedString, for  example.

    template<QQmlImport::RecursionRestriction recursionRestriction = QQmlImport::PreventRecursion>
    Result query(const QHashedStringRef &key, QQmlTypeLoader *typeLoader) const
    {
        return doQuery<const QHashedStringRef &, recursionRestriction>(key, typeLoader);
    }

    template<QueryNamespaced queryNamespaced = QueryNamespaced::Yes>
    Result query(const QHashedStringRef &key, const QQmlImportRef *importNamespace,
                 QQmlTypeLoader *typeLoader) const
    {
        return doQuery<const QHashedStringRef &, queryNamespaced>(key, importNamespace, typeLoader);
    }

    template<QQmlImport::RecursionRestriction recursionRestriction = QQmlImport::PreventRecursion>
    Result query(const QV4::String *key, QQmlTypeLoader *typeLoader) const
    {
        return doQuery<const QV4::String *, recursionRestriction>(key, typeLoader);
    }

    template<QueryNamespaced queryNamespaced = QueryNamespaced::Yes>
    Result query(const QV4::String *key, const QQmlImportRef *importNamespace,
                 QQmlTypeLoader *typeLoader) const
    {
        return doQuery<const QV4::String *, queryNamespaced>(key, importNamespace, typeLoader);
    }

private:
    friend class QQmlImports;

    static QHashedStringRef toHashedStringRef(const QHashedStringRef &key) { return key; }
    static QHashedStringRef toHashedStringRef(const QV4::String *key)
    {
        const QV4::Heap::String *heapString = key->d();

        // toQString() would also do simplifyString(). Therefore, we can be sure that this
        // is safe. Any other operation on the string data cannot keep references on the
        // non-simplified pieces.
        if (heapString->subtype >= QV4::Heap::String::StringType_Complex)
            heapString->simplifyString();

        // This is safe because the string data is backed by the QV4::String we got as
        // parameter. The contract about passing V4 values as parameters is that you have to
        // scope them first, so that they don't get gc'd while the callee is working on them.
        const QStringPrivate &text = heapString->text();
        return QHashedStringRef(QStringView(text.ptr, text.size));
    }

    static QString toQString(const QHashedStringRef &key) { return key.toString(); }
    static QString toQString(const QV4::String *key) { return key->toQStringNoThrow(); }

    template<typename Key, QQmlImport::RecursionRestriction recursionRestriction>
    Result doQuery(Key name, QQmlTypeLoader *typeLoader) const
    {
        Result result = doQuery(m_namedImports, name);

        if (!result.isValid())
            result = typeSearch(m_anonymousImports, name);

        if (!result.isValid())
            result = doQuery(m_anonymousCompositeSingletons, name);

        if (!result.isValid()) {
            // Look up anonymous types from the imports of this document
            // ### it would be nice if QQmlImports allowed us to resolve a namespace
            // first, and then types on it.
            QQmlImportNamespace *typeNamespace = nullptr;
            QList<QQmlError> errors;
            QQmlType t;
            bool typeRecursionDetected = false;
            const bool typeFound = m_imports->resolveType(
                    typeLoader, toHashedStringRef(name), &t, nullptr, &typeNamespace, &errors,
                    QQmlType::AnyRegistrationType,
                    recursionRestriction == QQmlImport::AllowRecursion
                            ? &typeRecursionDetected
                            : nullptr);
            if (typeFound)
                return Result(t);

        }

        return result;
    }

    template<typename Key, QueryNamespaced queryNamespaced>
    Result doQuery(Key name, const QQmlImportRef *importNamespace, QQmlTypeLoader *typeLoader) const
    {
        Q_ASSERT(importNamespace && importNamespace->scriptIndex == -1);

        if constexpr (queryNamespaced == QueryNamespaced::Yes) {
            QMap<const QQmlImportRef *, QStringHash<QQmlImportRef> >::const_iterator it
                    = m_namespacedImports.constFind(importNamespace);
            if (it != m_namespacedImports.constEnd()) {
                Result r = doQuery(*it, name);
                if (r.isValid())
                    return r;
            }
        }

        Result result = typeSearch(importNamespace->modules, name);

        if (!result.isValid())
            result = doQuery(importNamespace->compositeSingletons, name);

        if (!result.isValid()) {
            // Look up types from the imports of this document
            // ### it would be nice if QQmlImports allowed us to resolve a namespace
            // first, and then types on it.
            const QString qualifiedTypeName = importNamespace->m_qualifier + u'.' + toQString(name);
            QQmlImportNamespace *typeNamespace = nullptr;
            QList<QQmlError> errors;
            QQmlType t;
            bool typeFound = m_imports->resolveType(
                        typeLoader, qualifiedTypeName, &t, nullptr, &typeNamespace, &errors);
            if (typeFound)
                return Result(t);
        }

        return result;
    }

    template<typename Key>
    Result doQuery(const QStringHash<QQmlImportRef> &imports, Key key) const
    {
        QQmlImportRef *i = imports.value(key);
        if (i) {
            Q_ASSERT(!i->m_qualifier.isEmpty());
            if (i->scriptIndex != -1) {
                return Result(i->scriptIndex);
            } else {
                return Result(i);
            }
        }

        return Result();
    }

    template<typename Key>
    Result doQuery(const QStringHash<QUrl> &urls, Key key) const
    {
        QUrl *url = urls.value(key);
        if (url) {
            QQmlType type = QQmlMetaType::qmlType(*url);
            return Result(type);
        }

        return Result();
    }

    template<typename Key>
    Result typeSearch(const QVector<QQmlTypeModuleVersion> &modules, Key key) const
    {
        for (auto it = modules.crbegin(), end = modules.crend(); it != end; ++it) {
            QQmlType type = it->type(key);
            if (type.isValid())
                return Result(type);
        }

        return Result();
    }

    QStringHash<QQmlImportRef> m_namedImports;
    QMap<const QQmlImportRef *, QStringHash<QQmlImportRef> > m_namespacedImports;
    QVector<QQmlTypeModuleVersion> m_anonymousImports;
    QStringHash<QUrl> m_anonymousCompositeSingletons;
    QQmlRefPointer<QQmlImports> m_imports;
};

QQmlTypeNameCache::Result::Result()
: importNamespace(nullptr), scriptIndex(-1)
{
}

QQmlTypeNameCache::Result::Result(const QQmlImportRef *importNamespace)
: importNamespace(importNamespace), scriptIndex(-1)
{
}

QQmlTypeNameCache::Result::Result(const QQmlType &type)
: type(type), importNamespace(nullptr), scriptIndex(-1)
{
}

QQmlTypeNameCache::Result::Result(int scriptIndex)
: importNamespace(nullptr), scriptIndex(scriptIndex)
{
}

bool QQmlTypeNameCache::Result::isValid() const
{
    return type.isValid() || importNamespace || scriptIndex != -1;
}

bool QQmlTypeNameCache::isEmpty() const
{
    return m_namedImports.isEmpty() && m_anonymousImports.isEmpty()
        && m_anonymousCompositeSingletons.isEmpty();
}

QT_END_NAMESPACE

#endif // QQMLTYPENAMECACHE_P_H

