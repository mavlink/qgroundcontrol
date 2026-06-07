// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLDOMEXTERNALITEMS_P_H
#define QQMLDOMEXTERNALITEMS_P_H

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

#include "qqmldomitem_p.h"
#include "qqmldomelements_p.h"
#include "qqmldommoduleindex_p.h"
#include "qqmldomcomments_p.h"

#include <QtQml/private/qqmljsast_p.h>
#include <QtQml/private/qqmljsengine_p.h>
#include <QtQml/private/qqmldirparser_p.h>
#include <QtQmlCompiler/private/qqmljstyperesolver_p.h>
#include <QtCore/QMetaType>
#include <QtCore/qregularexpression.h>

#include <limits>
#include <memory>

Q_DECLARE_METATYPE(QQmlDirParser::Plugin)

QT_BEGIN_NAMESPACE

namespace QQmlJS {
namespace Dom {

/*!
\internal
\class QQmlJS::Dom::ExternalOwningItem

\brief A OwningItem that refers to an external resource (file,...)

Every owning item has a file or directory it refers to.


*/
class QMLDOM_EXPORT ExternalOwningItem: public OwningItem {
public:
    ExternalOwningItem(
            const QString &filePath, const QDateTime &lastDataUpdateAt, const Path &pathFromTop,
            int derivedFrom = 0, const QString &code = QString());
    ExternalOwningItem(const ExternalOwningItem &o) = default;
    QString canonicalFilePath(const DomItem &) const override;
    QString canonicalFilePath() const;
    Path canonicalPath(const DomItem &) const override;
    Path canonicalPath() const;
    bool iterateDirectSubpaths(const DomItem &self, DirectVisitor visitor) const override
    {
        bool cont = OwningItem::iterateDirectSubpaths(self, visitor);
        cont = cont && self.dvValueLazyField(visitor, Fields::canonicalFilePath, [this]() {
            return canonicalFilePath();
        });
        cont = cont
                && self.dvValueLazyField(visitor, Fields::isValid, [this]() { return isValid(); });
        if (!code().isNull())
            cont = cont
                    && self.dvValueLazyField(visitor, Fields::code, [this]() { return code(); });
        return cont;
    }

    bool iterateSubOwners(const DomItem &self, function_ref<bool(const DomItem &owner)> visitor) override
    {
        bool cont = OwningItem::iterateSubOwners(self, visitor);
        cont = cont && self.field(Fields::components).visitKeys([visitor](const QString &, const DomItem &comps) {
            return comps.visitIndexes([visitor](const DomItem &comp) {
                return comp.field(Fields::objects).visitIndexes([visitor](const DomItem &qmlObj) {
                    if (const QmlObject *qmlObjPtr = qmlObj.as<QmlObject>())
                        return qmlObjPtr->iterateSubOwners(qmlObj, visitor);
                    Q_ASSERT(false);
                    return true;
                });
            });
        });
        return cont;
    }

    bool isValid() const {
        QMutexLocker l(mutex());
        return m_isValid;
    }
    void setIsValid(bool val) {
        QMutexLocker l(mutex());
        m_isValid = val;
    }
    // null code means invalid
    const QString &code() const { return m_code; }

protected:
    QString m_canonicalFilePath;
    QString m_code;
    Path m_path;
    bool m_isValid = false;
};

class QMLDOM_EXPORT QmlDirectory final : public ExternalOwningItem
{
protected:
    std::shared_ptr<OwningItem> doCopy(const DomItem &) const override
    {
        return std::make_shared<QmlDirectory>(*this);
    }

public:
    constexpr static DomType kindValue = DomType::QmlDirectory;
    DomType kind() const override { return kindValue; }
    QmlDirectory(
            const QString &filePath = QString(), const QStringList &dirList = QStringList(),
            const QDateTime &lastDataUpdateAt = QDateTime::fromMSecsSinceEpoch(0, QTimeZone::UTC),
            int derivedFrom = 0);
    QmlDirectory(const QmlDirectory &o) = default;

    std::shared_ptr<QmlDirectory> makeCopy(const DomItem &self) const
    {
        return std::static_pointer_cast<QmlDirectory>(doCopy(self));
    }

    bool iterateDirectSubpaths(const DomItem &self, DirectVisitor visitor) const override;

    const QMultiMap<QString, Export> &exports() const & { return m_exports; }

    const QMultiMap<QString, QString> &qmlFiles() const & { return m_qmlFiles; }

    bool addQmlFilePath(const QString &relativePath);

private:
    QMultiMap<QString, Export> m_exports;
    QMultiMap<QString, QString> m_qmlFiles;
};

class QMLDOM_EXPORT QmldirFile final : public ExternalOwningItem
{
    Q_DECLARE_TR_FUNCTIONS(QmldirFile)
protected:
    std::shared_ptr<OwningItem> doCopy(const DomItem &) const override
    {
        auto copy = std::make_shared<QmldirFile>(*this);
        return copy;
    }

public:
    constexpr static DomType kindValue = DomType::QmldirFile;
    DomType kind() const override { return kindValue; }

    static ErrorGroups myParsingErrors();

    QmldirFile(
            const QString &filePath = QString(), const QString &code = QString(),
            const QDateTime &lastDataUpdateAt = QDateTime::fromMSecsSinceEpoch(0, QTimeZone::UTC),
            int derivedFrom = 0)
        : ExternalOwningItem(filePath, lastDataUpdateAt, Paths::qmldirFilePath(filePath),
                             derivedFrom, code)
    {
    }
    QmldirFile(const QmldirFile &o) = default;

    static std::shared_ptr<QmldirFile> fromPathAndCode(const QString &path, const QString &code);

    std::shared_ptr<QmldirFile> makeCopy(const DomItem &self) const
    {
        return std::static_pointer_cast<QmldirFile>(doCopy(self));
    }

    bool iterateDirectSubpaths(const DomItem &self, DirectVisitor visitor) const override;

    QmlUri uri() const { return m_uri; }

    const QSet<int> &majorVersions() const & { return m_majorVersions; }

    const QMultiMap<QString, Export> &exports() const & { return m_exports; }

    const QList<Import> &imports() const & { return m_imports; }

    const QList<Path> &qmltypesFilePaths() const & { return m_qmltypesFilePaths; }

    QMap<QString, QString> qmlFiles() const;

    bool designerSupported() const { return m_qmldir.designerSupported(); }

    QStringList classNames() const { return m_qmldir.classNames(); }

    QList<ModuleAutoExport> autoExports() const;
    void setAutoExports(const QList<ModuleAutoExport> &autoExport);

    void ensureInModuleIndex(const DomItem &self, const QString &uri) const;

private:
    void parse();
    void setFromQmldir();

    QmlUri m_uri;
    QSet<int> m_majorVersions;
    QQmlDirParser m_qmldir;
    QList<QQmlDirParser::Plugin> m_plugins;
    QList<Import> m_imports;
    QList<ModuleAutoExport> m_autoExports;
    QMultiMap<QString, Export> m_exports;
    QList<Path> m_qmltypesFilePaths;
};

class QMLDOM_EXPORT JsFile final : public ExternalOwningItem
{
protected:
    std::shared_ptr<OwningItem> doCopy(const DomItem &) const override
    {
        auto copy = std::make_shared<JsFile>(*this);
        return copy;
    }

public:
    constexpr static DomType kindValue = DomType::JsFile;
    DomType kind() const override { return kindValue; }
    JsFile(const QString &filePath = QString(),
           const QDateTime &lastDataUpdateAt = QDateTime::fromMSecsSinceEpoch(0, QTimeZone::UTC),
           const Path &pathFromTop = Path(), int derivedFrom = 0)
        : ExternalOwningItem(filePath, lastDataUpdateAt, pathFromTop, derivedFrom)
    {
    }
    JsFile(const QString &filePath = QString(), const QString &code = QString(),
           const QDateTime &lastDataUpdateAt = QDateTime::fromMSecsSinceEpoch(0, QTimeZone::UTC),
           int derivedFrom = 0);
    JsFile(const JsFile &o) = default;

    std::shared_ptr<JsFile> makeCopy(const DomItem &self) const
    {
        return std::static_pointer_cast<JsFile>(doCopy(self));
    }

    bool iterateDirectSubpaths(const DomItem &self, DirectVisitor) const
            override; // iterates the *direct* subpaths, returns false if a quick end was requested

    std::shared_ptr<QQmlJS::Engine> engine() const { return m_engine; }
    JsResource rootComponent() const { return m_rootComponent; }
    void setFileLocationsTree(const FileLocations::Tree &v) { m_fileLocationsTree = std::move(v); }

    static ErrorGroups myParsingErrors();

    void writeOut(const DomItem &self, OutWriter &lw) const override;
    void setExpression(const std::shared_ptr<ScriptExpression> &script) { m_script = script; }

    void initPragmaLibrary() { m_pragmaLibrary = LegacyPragmaLibrary{}; };
    void addFileImport(const QString &jsfile, const QString &module);
    void addModuleImport(const QString &uri, const QString &version, const QString &module);

private:
    void writeOutDirectives(OutWriter &lw) const;

    /*
    Entities with Legacy prefix are here to support formatting of the discouraged
    .import, .pragma directives in .js files.
    Taking into account that usage of these directives is discouraged and
    the fact that current usecase is limited to the formatting of .js, it's arguably should not
    be exposed and kept private.

    LegacyPragma corresponds to the only one existing .pragma library

    LegacyImport is capable of representing the following import statements:
    .import T_STRING_LITERAL as T_IDENTIFIER
    .import T_IDENTIFIER (. T_IDENTIFIER)* (T_VERSION_NUMBER (. T_VERSION_NUMBER)?)? as T_IDENTIFIER

    LegacyDirectivesCollector is a workaround for collecting those directives.
    At the moment of writing .import, .pragma in .js files do not have corresponding
    representative AST::Node-s. Collecting of those is happening during the lexing
    */

    struct LegacyPragmaLibrary
    {
        void writeOut(OutWriter &lw) const;
    };

    struct LegacyImport
    {
        QString fileName; // file import
        QString uri; // module import
        QString version; // used for module import
        QString asIdentifier; // .import ... as T_Identifier

        void writeOut(OutWriter &lw) const;
    };

    class LegacyDirectivesCollector : public QQmlJS::Directives
    {
    public:
        LegacyDirectivesCollector(JsFile &file) : m_file(file){};

        void pragmaLibrary() override { m_file.initPragmaLibrary(); };
        void importFile(const QString &jsfile, const QString &module, int, int) override
        {
            m_file.addFileImport(jsfile, module);
        };
        void importModule(const QString &uri, const QString &version, const QString &module, int,
                          int) override
        {
            m_file.addModuleImport(uri, version, module);
        };

    private:
        JsFile &m_file;
    };

private:
    std::shared_ptr<QQmlJS::Engine> m_engine;
    std::optional<LegacyPragmaLibrary> m_pragmaLibrary = std::nullopt;
    QList<LegacyImport> m_imports;
    std::shared_ptr<ScriptExpression> m_script;
    JsResource m_rootComponent;
    FileLocations::Tree m_fileLocationsTree;
};

class QMLDOM_EXPORT QmlFile final : public ExternalOwningItem
{
protected:
    std::shared_ptr<OwningItem> doCopy(const DomItem &self) const override;

public:
    constexpr static DomType kindValue = DomType::QmlFile;
    DomType kind() const override { return kindValue; }

    enum RecoveryOption { DisableParserRecovery, EnableParserRecovery };

    QmlFile(const QString &filePath = QString(), const QString &code = QString(),
            const QDateTime &lastDataUpdate = QDateTime::fromMSecsSinceEpoch(0, QTimeZone::UTC),
            int derivedFrom = 0, RecoveryOption option = DisableParserRecovery);
    static ErrorGroups myParsingErrors();
    bool iterateDirectSubpaths(const DomItem &self, DirectVisitor) const
            override; // iterates the *direct* subpaths, returns false if a quick end was requested
    DomItem field(const DomItem &self, QStringView name) const override;
    std::shared_ptr<QmlFile> makeCopy(const DomItem &self) const
    {
        return std::static_pointer_cast<QmlFile>(doCopy(self));
    }
    void addError(const DomItem &self, ErrorMessage &&msg) override;

    const QMultiMap<QString, QmlComponent> &components() const &
    {
        return lazyMembers().m_components;
    }
    void setComponents(const QMultiMap<QString, QmlComponent> &components)
    {
        lazyMembers().m_components = components;
    }
    Path addComponent(const QmlComponent &component, AddOption option = AddOption::Overwrite,
                      QmlComponent **cPtr = nullptr)
    {
        QStringList nameEls = component.name().split(QChar::fromLatin1('.'));
        QString key = nameEls.mid(1).join(QChar::fromLatin1('.'));
        return insertUpdatableElementInMultiMap(Path::fromField(Fields::components), lazyMembers().m_components,
                                                key, component, option, cPtr);
    }

    void writeOut(const DomItem &self, OutWriter &lw) const override;

    AST::UiProgram *ast() const
    {
        return m_ast; // avoid making it public? would make moving away from it easier
    }
    const QList<Import> &imports() const &
    {
        return lazyMembers().m_imports;
    }
    void setImports(const QList<Import> &imports) { lazyMembers().m_imports = imports; }
    Path addImport(const Import &i)
    {
        auto &members = lazyMembers();
        index_type idx = index_type(members.m_imports.size());
        members.m_imports.append(i);
        if (i.uri.isModule()) {
            members.m_importScope.addImport((i.importId.isEmpty()
                                                     ? QStringList()
                                                     : i.importId.split(QChar::fromLatin1('.'))),
                                            i.importedPath());
        } else {
            QString path = i.uri.absoluteLocalPath(canonicalFilePath());
            if (!path.isEmpty())
                members.m_importScope.addImport(
                        (i.importId.isEmpty() ? QStringList()
                                              : i.importId.split(QChar::fromLatin1('.'))),
                        Paths::qmlDirPath(path));
        }
        return Path::fromField(Fields::imports).withIndex(idx);
    }
    std::shared_ptr<QQmlJS::Engine> engine() const { return m_engine; }
    RegionComments &comments() { return lazyMembers().m_comments; }
    std::shared_ptr<AstComments> astComments() const { return lazyMembers().m_astComments; }
    void setAstComments(const std::shared_ptr<AstComments> &comm) { lazyMembers().m_astComments = comm; }
    FileLocations::Tree fileLocationsTree() const { return lazyMembers().m_fileLocationsTree; }
    void setFileLocationsTree(const FileLocations::Tree &v) { lazyMembers().m_fileLocationsTree = v; }
    const QList<Pragma> &pragmas() const & { return lazyMembers().m_pragmas; }
    void setPragmas(QList<Pragma> pragmas) { lazyMembers().m_pragmas = pragmas; }
    Path addPragma(const Pragma &pragma)
    {
        auto &members = lazyMembers();
        int idx = members.m_pragmas.size();
        members.m_pragmas.append(pragma);
        return Path::fromField(Fields::pragmas).withIndex(idx);
    }
    ImportScope &importScope() { return lazyMembers().m_importScope; }
    const ImportScope &importScope() const { return lazyMembers().m_importScope; }

    std::shared_ptr<QQmlJSTypeResolver> typeResolver() const
    {
        return lazyMembers().m_typeResolver;
    }
    void setTypeResolverWithDependencies(const std::shared_ptr<QQmlJSTypeResolver> &typeResolver,
                                         const QQmlJSTypeResolverDependencies &dependencies)
    {
        auto &members = lazyMembers();
        members.m_typeResolver = typeResolver;
        members.m_typeResolverDependencies = dependencies;
    }

    DomCreationOption creationOption() const { return lazyMembers().m_creationOption; }

    QQmlJSScope::ConstPtr handleForPopulation() const
    {
        return m_handleForPopulation;
    }

    void setHandleForPopulation(const QQmlJSScope::ConstPtr &scope)
    {
        m_handleForPopulation = scope;
    }


private:
    // The lazy parts of QmlFile are inside of QmlFileLazy.
    struct QmlFileLazy
    {
        QmlFileLazy(FileLocations::Tree fileLocationsTree, AstComments *astComments)
            : m_fileLocationsTree(fileLocationsTree), m_astComments(astComments)
        {
        }
        RegionComments m_comments;
        QMultiMap<QString, QmlComponent> m_components;
        QList<Pragma> m_pragmas;
        QList<Import> m_imports;
        ImportScope m_importScope;
        FileLocations::Tree m_fileLocationsTree;
        std::shared_ptr<AstComments> m_astComments;
        DomCreationOption m_creationOption;
        std::shared_ptr<QQmlJSTypeResolver> m_typeResolver;
        QQmlJSTypeResolverDependencies m_typeResolverDependencies;
    };
    friend class QQmlDomAstCreator;
    AST::UiProgram *m_ast; // avoid? would make moving away from it easier
    std::shared_ptr<Engine> m_engine;
    QQmlJSScope::ConstPtr m_handleForPopulation;
    mutable std::optional<QmlFileLazy> m_lazyMembers;

    void ensurePopulated() const
    {
        if (m_lazyMembers)
            return;

        m_lazyMembers.emplace(FileLocations::createTree(canonicalPath()), new AstComments(m_engine));

        // populate via the QQmlJSScope by accessing the (lazy) pointer
        if (m_handleForPopulation.factory()) {
            // silence no-discard attribute:
            Q_UNUSED(m_handleForPopulation.data());
        }
    }
    const QmlFileLazy &lazyMembers() const
    {
        ensurePopulated();
        return *m_lazyMembers;
    }
    QmlFileLazy &lazyMembers()
    {
        ensurePopulated();
        return *m_lazyMembers;
    }
};

class QMLDOM_EXPORT QmltypesFile final : public ExternalOwningItem
{
protected:
    std::shared_ptr<OwningItem> doCopy(const DomItem &) const override
    {
        auto res = std::make_shared<QmltypesFile>(*this);
        return res;
    }

public:
    constexpr static DomType kindValue = DomType::QmltypesFile;
    DomType kind() const override { return kindValue; }

    QmltypesFile(
            const QString &filePath = QString(), const QString &code = QString(),
            const QDateTime &lastDataUpdateAt = QDateTime::fromMSecsSinceEpoch(0, QTimeZone::UTC),
            int derivedFrom = 0)
        : ExternalOwningItem(filePath, lastDataUpdateAt, Paths::qmltypesFilePath(filePath),
                             derivedFrom, code)
    {
    }

    QmltypesFile(const QmltypesFile &o) = default;

    void ensureInModuleIndex(const DomItem &self) const;

    bool iterateDirectSubpaths(const DomItem &self, DirectVisitor) const override;
    std::shared_ptr<QmltypesFile> makeCopy(const DomItem &self) const
    {
        return std::static_pointer_cast<QmltypesFile>(doCopy(self));
    }

    void addImport(const Import i)
    { // builder only: not threadsafe...
        m_imports.append(i);
    }
    const QList<Import> &imports() const & { return m_imports; }
    const QMultiMap<QString, QmltypesComponent> &components() const & { return m_components; }
    void setComponents(QMultiMap<QString, QmltypesComponent> c) { m_components = std::move(c); }
    Path addComponent(const QmltypesComponent &comp, AddOption option = AddOption::Overwrite,
                      QmltypesComponent **cPtr = nullptr)
    {
        for (const Export &e : comp.exports())
            addExport(e);
        return insertUpdatableElementInMultiMap(Path::fromField(u"components"), m_components,
                                                comp.name(), comp, option, cPtr);
    }
    const QMultiMap<QString, Export> &exports() const & { return m_exports; }
    void setExports(QMultiMap<QString, Export> e) { m_exports = e; }
    Path addExport(const Export &e)
    {
        index_type i = m_exports.values(e.typeName).size();
        m_exports.insert(e.typeName, e);
        addUri(e.uri, e.version.majorVersion);
        return canonicalPath().withField(Fields::exports).withIndex(i);
    }

    const QMap<QString, QSet<int>> &uris() const & { return m_uris; }
    void addUri(const QString &uri, int majorVersion)
    {
        QSet<int> &v = m_uris[uri];
        if (!v.contains(majorVersion)) {
            v.insert(majorVersion);
        }
    }

private:
    QList<Import> m_imports;
    QMultiMap<QString, QmltypesComponent> m_components;
    QMultiMap<QString, Export> m_exports;
    QMap<QString, QSet<int>> m_uris;
};

class QMLDOM_EXPORT GlobalScope final : public ExternalOwningItem
{
protected:
    std::shared_ptr<OwningItem> doCopy(const DomItem &) const override;

public:
    constexpr static DomType kindValue = DomType::GlobalScope;
    DomType kind() const override { return kindValue; }

    GlobalScope(
            const QString &filePath = QString(),
            const QDateTime &lastDataUpdateAt = QDateTime::fromMSecsSinceEpoch(0, QTimeZone::UTC),
            int derivedFrom = 0)
        : ExternalOwningItem(filePath, lastDataUpdateAt, Paths::globalScopePath(filePath),
                             derivedFrom)
    {
        setIsValid(true);
    }

    bool iterateDirectSubpaths(const DomItem &self, DirectVisitor visitor) const override;
    std::shared_ptr<GlobalScope> makeCopy(const DomItem &self) const
    {
        return std::static_pointer_cast<GlobalScope>(doCopy(self));
    }
    QString name() const { return m_name; }
    Language language() const { return m_language; }
    GlobalComponent rootComponent() const { return m_rootComponent; }
    void setName(const QString &name) { m_name = name; }
    void setLanguage(Language language) { m_language = language; }
    void setRootComponent(const GlobalComponent &ob)
    {
        m_rootComponent = ob;
        m_rootComponent.updatePathFromOwner(Path::fromField(Fields::rootComponent));
    }

private:
    QString m_name;
    Language m_language;
    GlobalComponent m_rootComponent;
};

} // end namespace Dom
} // end namespace QQmlJS
QT_END_NAMESPACE
#endif // QQMLDOMEXTERNALITEMS_P_H
