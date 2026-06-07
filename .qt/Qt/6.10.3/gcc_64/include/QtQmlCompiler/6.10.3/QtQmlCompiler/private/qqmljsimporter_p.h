// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:significant

#ifndef QQMLJSIMPORTER_P_H
#define QQMLJSIMPORTER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#include <qtqmlcompilerexports.h>

#include "qqmljscontextualtypes_p.h"
#include "qqmljsscope_p.h"
#include "qqmljsresourcefilemapper_p.h"
#include <QtQml/private/qqmldirparser_p.h>
#include <QtQml/private/qqmljsast_p.h>

QT_BEGIN_NAMESPACE

namespace QQmlJS {
class Import
{
public:
    Import() = default;
    Import(QString prefix, QString name, QTypeRevision version, bool isFile, bool isDependency);

    bool isValid() const;

    QString prefix() const { return m_prefix; }
    QString name() const { return m_name; }
    QTypeRevision version() const { return m_version; }
    bool isFile() const { return m_isFile; }
    bool isDependency() const { return m_isDependency; }

private:
    QString m_prefix;
    QString m_name;
    QTypeRevision m_version;
    bool m_isFile = false;
    bool m_isDependency = false;

    friend inline size_t qHash(const Import &key, size_t seed = 0) noexcept
    {
        return qHashMulti(seed, key.m_prefix, key.m_name, key.m_version,
                          key.m_isFile, key.m_isDependency);
    }

    friend inline bool operator==(const Import &a, const Import &b)
    {
        return a.m_prefix == b.m_prefix && a.m_name == b.m_name && a.m_version == b.m_version
                && a.m_isFile == b.m_isFile && a.m_isDependency == b.m_isDependency;
    }
};
}

enum QQmlJSImporterFlag {
    UseOptionalImports = 0x1,
    PreferQmlFilesFromSourceFolder = 0x2,
    TolerateFileSelectors = 0x4, // if we find a type "twice", check if one looks like it's from a file selector and use the other
};
Q_DECLARE_FLAGS(QQmlJSImporterFlags, QQmlJSImporterFlag)

class QQmlJSImportVisitor;
class QQmlJSLogger;
class Q_QMLCOMPILER_EXPORT QQmlJSImporter
{
public:
    struct ImportedTypes {
        ImportedTypes(QQmlJS::ContextualTypes &&types, QList<QQmlJS::DiagnosticMessage> &&warnings)
            : m_types(std::move(types)), m_warnings(std::move(warnings))
        {}

        ImportedTypes(const ImportedTypes &) = default;
        ImportedTypes(ImportedTypes &&) = default;
        ImportedTypes &operator=(const ImportedTypes &) = default;
        ImportedTypes &operator=(ImportedTypes &&) = default;
        ~ImportedTypes() = default;

        void clear()
        {
            m_types.clearTypes();
            m_warnings.clear();
        }

        const QQmlJS::ContextualTypes &contextualTypes() const { return m_types; }
        const QList<QQmlJS::DiagnosticMessage> &warnings() const { return m_warnings; };

        bool isEmpty() const { return m_types.types().isEmpty(); }

        bool hasType(const QString &name) const { return m_types.hasType(name); }
        QQmlJS::ImportedScope<QQmlJSScope::ConstPtr> type(const QString &name) const
        {
            return m_types.type(name);
        }
        QString name(const QQmlJSScope::ConstPtr &type) const { return m_types.name(type); }
        void setType(const QString &name, const QQmlJS::ImportedScope<QQmlJSScope::ConstPtr> &type)
        {
            m_types.setType(name, type);
        }
        bool isNullType(const QString &name) const { return m_types.isNullType(name); }
        const QHash<QString, QQmlJS::ImportedScope<QQmlJSScope::ConstPtr>> &types() const
        {
            return m_types.types();
        }

        void add(ImportedTypes &&other)
        {
            m_types.addTypes(std::move(other.m_types));
            m_warnings.append(std::move(other.m_warnings));
        }

        void addWarnings(QList<QQmlJS::DiagnosticMessage> &&warnings)
        {
            m_warnings.append(std::move(warnings));
        }

    private:
        QQmlJS::ContextualTypes m_types;
        QList<QQmlJS::DiagnosticMessage> m_warnings;
    };

    QQmlJSImporter(const QStringList &importPaths, QQmlJSResourceFileMapper *mapper,
                   QQmlJSImporterFlags flags = QQmlJSImporterFlags{});

    QQmlJSResourceFileMapper *resourceFileMapper() const { return m_mapper; }
    void setResourceFileMapper(QQmlJSResourceFileMapper *mapper) { m_mapper = mapper; }

    QQmlJSResourceFileMapper *metaDataMapper() const { return m_metaDataMapper; }
    void setMetaDataMapper(QQmlJSResourceFileMapper *mapper) { m_metaDataMapper = mapper; }

    ImportedTypes importHardCodedBuiltins();
    QList<QQmlJS::DiagnosticMessage> importQmldirs(const QStringList &qmltypesFiles);

    bool registerScope(const QQmlJSScope::Ptr &scope);
    QQmlJSScope::Ptr importFile(const QString &file);
    ImportedTypes importDirectory(const QString &directory, const QString &prefix = QString());

    // ### qmltc needs this. once re-written, we no longer need to expose this
    QHash<QString, QQmlJSScope::Ptr> importedFiles() const { return m_importedFiles; }

    ImportedTypes importModule(const QString &module, const QString &prefix = QString(),
                               QTypeRevision version = QTypeRevision(),
                               QStringList *staticModuleList = nullptr);

    ImportedTypes builtinInternalNames();

    QList<QQmlJS::DiagnosticMessage> takeGlobalWarnings()
    {
        const auto result = std::move(m_globalWarnings);
        m_globalWarnings.clear();
        return result;
    }

    QStringList importPaths() const { return m_importPaths; }
    void setImportPaths(const QStringList &importPaths);

    void clearCache();

    QQmlJSScope::ConstPtr jsGlobalObject();

    struct ImportVisitorPrerequisites
    {
        ImportVisitorPrerequisites(QQmlJSScope::Ptr target, QQmlJSLogger *logger,
                                   const QString &implicitImportDirectory = {},
                                   const QStringList &qmldirFiles = {})
            : m_target(target),
              m_logger(logger),
              m_implicitImportDirectory(implicitImportDirectory),
              m_qmldirFiles(qmldirFiles)
        {
            Q_ASSERT(target && logger);
        }

        QQmlJSScope::Ptr m_target;
        QQmlJSLogger *m_logger;
        QString m_implicitImportDirectory;
        QStringList m_qmldirFiles;
    };
    void runImportVisitor(QQmlJS::AST::Node *rootNode,
                          const ImportVisitorPrerequisites &prerequisites);

    /*!
    \internal
     When a qml file gets lazily loaded, it will be lexed and parsed and finally be constructed
    via an ImportVisitor. By default, this is done via the QQmlJSImportVisitor, but can also be done
    via other import visitors like QmltcVisitor, which is used by qmltc to compile a QML file, or
    QQmlDomAstCreatorWithQQmlJSScope, which is used to construct the Dom of lazily loaded QML files.
    */
    using ImportVisitor = std::function<void(QQmlJS::AST::Node *rootNode, QQmlJSImporter *self,
                                             const ImportVisitorPrerequisites &prerequisites)>;

    void setImportVisitor(ImportVisitor visitor) { m_importVisitor = visitor; }

private:
    friend class QDeferredFactory<QQmlJSScope>;

    struct AvailableTypes
    {
        AvailableTypes(QQmlJS::ContextualTypes builtins)
            : cppNames(std::move(builtins))
            , qmlNames(QQmlJS::ContextualTypes::QML, {}, {}, cppNames.arrayType())
        {
        }

        // C++ names used in qmltypes files for non-composite types
        QQmlJS::ContextualTypes cppNames;

        // Names the importing component sees, including any prefixes
        QQmlJS::ContextualTypes qmlNames;

        // Static modules included here
        QStringList staticModules;

        // Warnings produced when importing
        QList<QQmlJS::DiagnosticMessage> warnings;

        // Whether a system module has been imported
        bool hasSystemModule = false;
    };

    struct Import {
        QString name;
        bool isStaticModule = false;
        bool isSystemModule = false;

        QList<QQmlJSExportedScope> objects;
        QHash<QString, QQmlJSExportedScope> scripts;
        QList<QQmlDirParser::Import> imports;
        QList<QQmlDirParser::Import> dependencies;

        // Warnings produced when importing
        QList<QQmlJS::DiagnosticMessage> warnings;
    };

    AvailableTypes builtinImportHelper();
    bool importHelper(const QString &module, AvailableTypes *types,
                      const QString &prefix = QString(), QTypeRevision version = QTypeRevision(),
                      bool isDependency = false, bool isFile = false);
    void processImport(
            const QQmlJS::Import &importDescription, const Import &import, AvailableTypes *types);
    void importDependencies(
            const Import &import, AvailableTypes *types, const QString &prefix = QString(),
            QTypeRevision version = QTypeRevision(), bool isDependency = false);
    QQmlDirParser createQmldirParserForFile(const QString &filename, Import *import);
    void readQmltypes(const QString &filename, Import *result);
    Import readQmldir(const QString &dirname);
    Import readDirectory(const QString &directory);

    QQmlJSScope::Ptr localFile2ScopeTree(const QString &filePath);
    static void setQualifiedNamesOn(const Import &import);

    QStringList m_importPaths;

    QHash<std::pair<QString, QTypeRevision>, QString> m_seenImports;
    QHash<QQmlJS::Import, QSharedPointer<AvailableTypes>> m_cachedImportTypes;
    QHash<QString, Import> m_seenQmldirFiles;

    QHash<QString, QQmlJSScope::Ptr> m_importedFiles;
    QList<QQmlJS::DiagnosticMessage> m_globalWarnings;
    std::optional<AvailableTypes> m_builtins;

    QQmlJSResourceFileMapper *m_mapper = nullptr;
    QQmlJSResourceFileMapper *m_metaDataMapper = nullptr;
    QQmlJSImporterFlags m_flags;
    bool useOptionalImports() const { return m_flags.testFlag(UseOptionalImports); };
    bool preferQmlFilesFromSourceFolder() const
    {
        return m_flags.testFlag(PreferQmlFilesFromSourceFolder);
    };

    ImportVisitor m_importVisitor;
};

QT_END_NAMESPACE

#endif // QQMLJSIMPORTER_P_H
