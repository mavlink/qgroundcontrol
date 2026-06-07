// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLTYPEDATA_P_H
#define QQMLTYPEDATA_P_H

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

#include <private/qqmlsourcecoordinate_p.h>
#include <private/qqmltypeloader_p.h>
#include <private/qv4executablecompilationunit_p.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcCycle)

class Q_AUTOTEST_EXPORT QQmlTypeData : public QQmlTypeLoader::Blob
{
    Q_DECLARE_TR_FUNCTIONS(QQmlTypeData)
public:

    struct TypeReference
    {
        TypeReference() : version(QTypeRevision::zero()), needsCreation(true) {}

        QV4::CompiledData::Location location;
        QQmlType type;
        QTypeRevision version;
        QQmlRefPointer<QQmlTypeData> typeData;
        bool selfReference = false;
        QString prefix; // used by CompositeSingleton types
        QString qualifiedName() const;
        bool needsCreation;
    };

    struct ScriptReference
    {
        QV4::CompiledData::Location location;
        QString qualifier;
        QQmlRefPointer<QQmlScriptBlob> script;
    };

private:
    friend class QQmlTypeLoader;

    template<typename Container>
    void setCompileUnit(const Container &container);

public:
    QQmlTypeData(const QUrl &, QQmlTypeLoader *);
    ~QQmlTypeData() override;

    QV4::CompiledData::CompilationUnit *compilationUnit() const;

    // Used by QQmlComponent to get notifications
    struct TypeDataCallback {
        virtual ~TypeDataCallback();
        virtual void typeDataProgress(QQmlTypeData *, qreal) {}
        virtual void typeDataReady(QQmlTypeData *) {}
    };
    void registerCallback(TypeDataCallback *);
    void unregisterCallback(TypeDataCallback *);

    QQmlType qmlType(const QString &inlineComponentName = QString()) const;
    QByteArray typeClassName() const { return m_typeClassName; }
    SourceCodeData backupSourceCode() const { return m_backupSourceCode; }

protected:
    void done() override;
    void completed() override;
    void dataReceived(const SourceCodeData &) override;
    void initializeFromCachedUnit(const QQmlPrivate::CachedQmlUnit *unit) override;
    void allDependenciesDone() override;
    void downloadProgressChanged(qreal) override;

    QString stringAt(int index) const override;

private:
    using InlineComponentData = QV4::CompiledData::InlineComponentData;

    bool tryLoadFromDiskCache();
    bool loadFromDiskCache(const QQmlRefPointer<QV4::CompiledData::CompilationUnit> &unit);
    bool loadFromSource();
    void restoreIR(const QQmlRefPointer<QV4::CompiledData::CompilationUnit> &unit);
    void continueLoadFromIR();
    bool resolveTypes();
    QQmlError buildTypeResolutionCaches(
            QQmlRefPointer<QQmlTypeNameCache> *typeNameCache,
            QV4::CompiledData::ResolvedTypeReferenceMap *resolvedTypeCache
            ) const;
    void compile(const QQmlRefPointer<QQmlTypeNameCache> &typeNameCache,
                 QV4::CompiledData::ResolvedTypeReferenceMap *resolvedTypeCache,
                 const QV4::CompiledData::DependentTypesHasher &dependencyHasher);
    QQmlError createTypeAndPropertyCaches(
            const QQmlRefPointer<QQmlTypeNameCache> &typeNameCache,
            const QV4::CompiledData::ResolvedTypeReferenceMap &resolvedTypeCache);
    bool resolveType(const QString &typeName, QTypeRevision &version,
                     TypeReference &ref, int lineNumber = -1, int columnNumber = -1,
                     bool reportErrors = true,
                     QQmlType::RegistrationType registrationType = QQmlType::AnyRegistrationType,
                     bool *typeRecursionDetected = nullptr);

    void scriptImported(
            const QQmlRefPointer<QQmlScriptBlob> &blob, const QV4::CompiledData::Location &location,
            const QString &nameSpace, const QString &qualifier) override;

    SourceCodeData m_backupSourceCode; // used when cache verification fails.
    QScopedPointer<QmlIR::Document> m_document;
    QV4::CompiledData::TypeReferenceMap m_typeReferences;

    QList<ScriptReference> m_scripts;

    QSet<QString> m_namespaces;
    QList<TypeReference> m_compositeSingletons;

    // map from name index to resolved type
    // While this could be a hash, a map is chosen here to provide a stable
    // order, which is used to calculating a check-sum on dependent meta-objects.
    QMap<int, TypeReference> m_resolvedTypes;
    bool m_typesResolved:1;

    // Used for self-referencing types, otherwise invalid.
    QQmlType m_qmlType;
    QByteArray m_typeClassName; // used for meta-object later

    using CompilationUnitPtr = QQmlRefPointer<QV4::CompiledData::CompilationUnit>;

    QHash<QString, InlineComponentData> m_inlineComponentData;

    CompilationUnitPtr m_compiledData;

    QList<TypeDataCallback *> m_callbacks;

    bool m_implicitImportLoaded;
    bool loadImplicitImport();
    bool checkScripts();
    bool checkDependencies();

    template<typename Reference>
    void createError(const Reference &ref, const QString &message, QList<QQmlError> errors)
    {
        QQmlError error;
        error.setUrl(url());
        error.setLine(qmlConvertSourceCoordinate<quint32, int>(ref.location.line()));
        error.setColumn(qmlConvertSourceCoordinate<quint32, int>(ref.location.column()));
        error.setDescription(message);
        errors.prepend(std::move(error));
        setError(errors);
    }

    void createError(const TypeReference &type, const QString &message);
    void createError(const ScriptReference &script, const QString &message);

    bool checkCompositeSingletons();
    void createQQmlType();
    bool rebuildFromSource();
};

QT_END_NAMESPACE

#endif // QQMLTYPEDATA_P_H
