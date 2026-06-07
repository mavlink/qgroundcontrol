// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLDOMELEMENTS_P_H
#define QQMLDOMELEMENTS_P_H

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
#include "qqmldomconstants_p.h"
#include "qqmldomcomments_p.h"
#include "qqmldomlinewriter_p.h"

#include <QtQml/private/qqmljsast_p.h>
#include <QtQml/private/qqmljsengine_p.h>
#include <QtQml/private/qqmlsignalnames_p.h>

#include <QtCore/QCborValue>
#include <QtCore/QCborMap>
#include <QtCore/QMutexLocker>

#include <memory>
#include <private/qqmljsscope_p.h>

#include <functional>
#include <limits>
#include <utility>

QT_BEGIN_NAMESPACE

namespace QQmlJS {
namespace Dom {

// namespace for utility methods building specific paths
// using a namespace one can reopen it and add more methods in other places
namespace Paths {
Path moduleIndexPath(
        const QString &uri, int majorVersion, const ErrorHandler &errorHandler = nullptr);
Path moduleScopePath(
        const QString &uri, Version version, const ErrorHandler &errorHandler = nullptr);
Path moduleScopePath(
        const QString &uri, const QString &version, const ErrorHandler &errorHandler = nullptr);
inline Path moduleScopePath(
        const QString &uri, const ErrorHandler &errorHandler = nullptr)
{
    return moduleScopePath(uri, QString(), errorHandler);
}
inline Path qmlDirInfoPath(const QString &path)
{
    return Path::fromRoot(PathRoot::Top).withField(Fields::qmldirWithPath).withKey(path);
}
inline Path qmlDirPath(const QString &path)
{
    return qmlDirInfoPath(path).withField(Fields::currentItem);
}
inline Path qmldirFileInfoPath(const QString &path)
{
    return Path::fromRoot(PathRoot::Top).withField(Fields::qmldirFileWithPath).withKey(path);
}
inline Path qmldirFilePath(const QString &path)
{
    return qmldirFileInfoPath(path).withField(Fields::currentItem);
}
inline Path qmlFileInfoPath(const QString &canonicalFilePath)
{
    return Path::fromRoot(PathRoot::Top).withField(Fields::qmlFileWithPath).withKey(canonicalFilePath);
}
inline Path qmlFilePath(const QString &canonicalFilePath)
{
    return qmlFileInfoPath(canonicalFilePath).withField(Fields::currentItem);
}
inline Path qmlFileObjectPath(const QString &canonicalFilePath)
{
    return qmlFilePath(canonicalFilePath)
            .withField(Fields::components)
            .withKey(QString())
            .withIndex(0)
            .withField(Fields::objects)
            .withIndex(0);
}
inline Path qmltypesFileInfoPath(const QString &path)
{
    return Path::fromRoot(PathRoot::Top).withField(Fields::qmltypesFileWithPath).withKey(path);
}
inline Path qmltypesFilePath(const QString &path)
{
    return qmltypesFileInfoPath(path).withField(Fields::currentItem);
}
inline Path jsFileInfoPath(const QString &path)
{
    return Path::fromRoot(PathRoot::Top).withField(Fields::jsFileWithPath).withKey(path);
}
inline Path jsFilePath(const QString &path)
{
    return jsFileInfoPath(path).withField(Fields::currentItem);
}
inline Path qmlDirectoryInfoPath(const QString &path)
{
    return Path::fromRoot(PathRoot::Top).withField(Fields::qmlDirectoryWithPath).withKey(path);
}
inline Path qmlDirectoryPath(const QString &path)
{
    return qmlDirectoryInfoPath(path).withField(Fields::currentItem);
}
inline Path globalScopeInfoPath(const QString &name)
{
    return Path::fromRoot(PathRoot::Top).withField(Fields::globalScopeWithName).withKey(name);
}
inline Path globalScopePath(const QString &name)
{
    return globalScopeInfoPath(name).withField(Fields::currentItem);
}
inline Path lookupCppTypePath(const QString &name)
{
    return Path::fromCurrent(PathCurrent::Lookup).withField(Fields::cppType).withKey(name);
}
inline Path lookupPropertyPath(const QString &name)
{
    return Path::fromCurrent(PathCurrent::Lookup).withField(Fields::propertyDef).withKey(name);
}
inline Path lookupSymbolPath(const QString &name)
{
    return Path::fromCurrent(PathCurrent::Lookup).withField(Fields::symbol).withKey(name);
}
inline Path lookupTypePath(const QString &name)
{
    return Path::fromCurrent(PathCurrent::Lookup).withField(Fields::type).withKey(name);
}
inline Path loadInfoPath(const Path &el)
{
    return Path::fromRoot(PathRoot::Env).withField(Fields::loadInfo).withKey(el.toString());
}
} // end namespace Paths

class QMLDOM_EXPORT CommentableDomElement : public DomElement
{
public:
    CommentableDomElement(const Path &pathFromOwner = Path()) : DomElement(pathFromOwner) { }
    CommentableDomElement(const CommentableDomElement &o) : DomElement(o), m_comments(o.m_comments)
    {
    }
    CommentableDomElement &operator=(const CommentableDomElement &o) = default;
    bool iterateDirectSubpaths(const DomItem &self, DirectVisitor) const override;
    RegionComments &comments() { return m_comments; }
    const RegionComments &comments() const { return m_comments; }

private:
    RegionComments m_comments;
};

class QMLDOM_EXPORT Version
{
public:
    constexpr static DomType kindValue = DomType::Version;
    constexpr static qint32 Undefined = -1;
    constexpr static qint32 Latest = -2;

    Version(qint32 majorVersion = Undefined, qint32 minorVersion = Undefined);
    static Version fromString(QStringView v);

    bool iterateDirectSubpaths(const DomItem &self, DirectVisitor) const;

    bool isLatest() const;
    bool isValid() const;
    QString stringValue() const;
    QString majorString() const
    {
        if (majorVersion >= 0 || majorVersion == Undefined)
            return QString::number(majorVersion);
        return QString();
    }
    QString majorSymbolicString() const
    {
        if (majorVersion == Version::Latest)
            return QLatin1String("Latest");
        if (majorVersion >= 0 || majorVersion == Undefined)
            return QString::number(majorVersion);
        return QString();
    }
    QString minorString() const
    {
        if (minorVersion >= 0 || minorVersion == Undefined)
            return QString::number(minorVersion);
        return QString();
    }
    int compare(const Version &o) const
    {
        int c = majorVersion - o.majorVersion;
        if (c != 0)
            return c;
        return minorVersion - o.minorVersion;
    }

    qint32 majorVersion;
    qint32 minorVersion;
};
inline bool operator==(const Version &v1, const Version &v2)
{
    return v1.compare(v2) == 0;
}
inline bool operator!=(const Version &v1, const Version &v2)
{
    return v1.compare(v2) != 0;
}
inline bool operator<(const Version &v1, const Version &v2)
{
    return v1.compare(v2) < 0;
}
inline bool operator<=(const Version &v1, const Version &v2)
{
    return v1.compare(v2) <= 0;
}
inline bool operator>(const Version &v1, const Version &v2)
{
    return v1.compare(v2) > 0;
}
inline bool operator>=(const Version &v1, const Version &v2)
{
    return v1.compare(v2) >= 0;
}

class QMLDOM_EXPORT QmlUri
{
public:
    enum class Kind { Invalid, ModuleUri, DirectoryUrl, RelativePath, AbsolutePath };
    QmlUri() = default;
    static QmlUri fromString(const QString &importStr);
    static QmlUri fromUriString(const QString &importStr);
    static QmlUri fromDirectoryString(const QString &importStr);
    bool isValid() const;
    bool isDirectory() const;
    bool isModule() const;
    QString moduleUri() const;
    QString localPath() const;
    QString absoluteLocalPath(const QString &basePath = QString()) const;
    QUrl directoryUrl() const;
    QString directoryString() const;
    QString toString() const;
    Kind kind() const;

    friend bool operator==(const QmlUri &i1, const QmlUri &i2)
    {
        return i1.m_kind == i2.m_kind && i1.m_value == i2.m_value;
    }
    friend bool operator!=(const QmlUri &i1, const QmlUri &i2) { return !(i1 == i2); }

private:
    QmlUri(const QUrl &url) : m_kind(Kind::DirectoryUrl), m_value(url) { }
    QmlUri(Kind kind, const QString &value) : m_kind(kind), m_value(value) { }
    Kind m_kind = Kind::Invalid;
    std::variant<QString, QUrl> m_value;
};

class QMLDOM_EXPORT Import
{
    Q_DECLARE_TR_FUNCTIONS(Import)
public:
    constexpr static DomType kindValue = DomType::Import;

    static Import fromUriString(
            const QString &importStr, Version v = Version(), const QString &importId = QString(),
            const ErrorHandler &handler = nullptr);
    static Import fromFileString(
            const QString &importStr, const QString &importId = QString(),
            const ErrorHandler &handler = nullptr);

    Import(const QmlUri &uri = QmlUri(), Version version = Version(),
           const QString &importId = QString())
        : uri(uri), version(version), importId(importId)
    {
    }

    bool iterateDirectSubpaths(const DomItem &self, DirectVisitor) const;
    Path importedPath() const
    {
        if (uri.isDirectory()) {
            QString path = uri.absoluteLocalPath();
            if (!path.isEmpty()) {
                return Paths::qmlDirPath(path);
            } else {
                Q_ASSERT_X(false, "Import", "url imports not supported");
                return Paths::qmldirFilePath(uri.directoryString());
            }
        } else {
            return Paths::moduleScopePath(uri.moduleUri(), version);
        }
    }
    Import baseImport() const { return Import { uri, version }; }

    friend bool operator==(const Import &i1, const Import &i2)
    {
        return i1.uri == i2.uri && i1.version == i2.version && i1.importId == i2.importId
                && i1.comments == i2.comments && i1.implicit == i2.implicit;
    }
    friend bool operator!=(const Import &i1, const Import &i2) { return !(i1 == i2); }

    void writeOut(const DomItem &self, OutWriter &ow) const;

    static QRegularExpression importRe();

    QmlUri uri;
    Version version;
    QString importId;
    RegionComments comments;
    bool implicit = false;
};

class QMLDOM_EXPORT ModuleAutoExport
{
public:
    constexpr static DomType kindValue = DomType::ModuleAutoExport;

    bool iterateDirectSubpaths(const DomItem &self, DirectVisitor visitor) const
    {
        bool cont = true;
        cont = cont && self.dvWrapField(visitor, Fields::import, import);
        cont = cont && self.dvValueField(visitor, Fields::inheritVersion, inheritVersion);
        return cont;
    }

    friend bool operator==(const ModuleAutoExport &i1, const ModuleAutoExport &i2)
    {
        return i1.import == i2.import && i1.inheritVersion == i2.inheritVersion;
    }
    friend bool operator!=(const ModuleAutoExport &i1, const ModuleAutoExport &i2)
    {
        return !(i1 == i2);
    }

    Import import;
    bool inheritVersion = false;
};

class QMLDOM_EXPORT Pragma
{
public:
    constexpr static DomType kindValue = DomType::Pragma;

    Pragma(const QString &pragmaName = QString(), const QStringList &pragmaValues = {})
        : name(pragmaName), values{ pragmaValues }
    {
    }

    bool iterateDirectSubpaths(const DomItem &self, DirectVisitor visitor) const
    {
        bool cont = self.dvValueField(visitor, Fields::name, name);
        cont = cont && self.dvValueField(visitor, Fields::values, values);
        cont = cont && self.dvWrapField(visitor, Fields::comments, comments);
        return cont;
    }

    void writeOut(const DomItem &self, OutWriter &ow) const;

    QString name;
    QStringList values;
    RegionComments comments;
};

class QMLDOM_EXPORT Id
{
public:
    constexpr static DomType kindValue = DomType::Id;

    Id(const QString &idName = QString(), const Path &referredObject = Path());

    bool iterateDirectSubpaths(const DomItem &self, DirectVisitor) const;
    void updatePathFromOwner(const Path &pathFromOwner);
    Path addAnnotation(const Path &selfPathFromOwner, const QmlObject &ann, QmlObject **aPtr = nullptr);

    QString name;
    Path referredObjectPath;
    RegionComments comments;
    QList<QmlObject> annotations;
    std::shared_ptr<ScriptExpression> value;
};

// TODO: rename? it may contain statements and stuff, not only expressions
// TODO QTBUG-121933
class QMLDOM_EXPORT ScriptExpression final : public OwningItem
{
    Q_GADGET
    Q_DECLARE_TR_FUNCTIONS(ScriptExpression)
public:
    enum class ExpressionType {
        BindingExpression,
        FunctionBody,
        ArgInitializer,
        ArgumentStructure,
        ReturnType,
        JSCode, // Used for storing the content of the whole .js file as "one" Expression
        ESMCode, // Used for storing the content of the whole ECMAScript module (.mjs) as "one"
                 // Expression
    };
    Q_ENUM(ExpressionType);
    constexpr static DomType kindValue = DomType::ScriptExpression;
    DomType kind() const override { return kindValue; }

    explicit ScriptExpression(
            QStringView code, const std::shared_ptr<QQmlJS::Engine> &engine, AST::Node *ast,
            const std::shared_ptr<AstComments> &comments, ExpressionType expressionType,
            SourceLocation localOffset = SourceLocation(), int derivedFrom = 0,
            QStringView preCode = QStringView(), QStringView postCode = QStringView());

    ScriptExpression()
        : ScriptExpression(QStringView(), std::shared_ptr<QQmlJS::Engine>(), nullptr,
                           std::shared_ptr<AstComments>(), ExpressionType::BindingExpression,
                           SourceLocation(), 0)
    {
    }

    explicit ScriptExpression(
            const QString &code, ExpressionType expressionType, int derivedFrom = 0,
            const QString &preCode = QString(), const QString &postCode = QString())
        : OwningItem(derivedFrom), m_expressionType(expressionType)
    {
        setCode(code, preCode, postCode);
    }

    ScriptExpression(const ScriptExpression &e);

    std::shared_ptr<ScriptExpression> makeCopy(const DomItem &self) const
    {
        return std::static_pointer_cast<ScriptExpression>(doCopy(self));
    }

    std::shared_ptr<ScriptExpression> copyWithUpdatedCode(const DomItem &self, const QString &code) const;

    bool iterateDirectSubpaths(const DomItem &self, DirectVisitor visitor) const override;

    Path canonicalPath(const DomItem &self) const override { return self.m_ownerPath; }
    // parsed and created if not available
    AST::Node *ast() const { return m_ast; }
    // dump of the ast (without locations)
    void astDumper(const Sink &s, AstDumperOptions options) const;
    QString astRelocatableDump() const;

    // definedSymbols name, value, from
    // usedSymbols name, locations
    QStringView code() const
    {
        QMutexLocker l(mutex());
        return m_code;
    }

    ExpressionType expressionType() const
    {
        QMutexLocker l(mutex());
        return m_expressionType;
    }

    bool isNull() const
    {
        QMutexLocker l(mutex());
        return m_code.isNull();
    }
    std::shared_ptr<QQmlJS::Engine> engine() const
    {
        QMutexLocker l(mutex());
        return m_engine;
    }
    QStringView loc2Str(SourceLocation) const;
    std::shared_ptr<AstComments> astComments() const { return m_astComments; }
    void writeOut(const DomItem &self, OutWriter &lw) const override;
    SourceLocation globalLocation(const DomItem &self) const;
    SourceLocation localOffset() const { return m_localOffset; }
    QStringView preCode() const { return m_preCode; }
    QStringView postCode() const { return m_postCode; }
    void setScriptElement(const ScriptElementVariant &p);
    ScriptElementVariant scriptElement() { return m_element; }

protected:
    std::shared_ptr<OwningItem> doCopy(const DomItem &) const override
    {
        return std::make_shared<ScriptExpression>(*this);
    }

    std::function<SourceLocation(SourceLocation)> locationToGlobalF(const DomItem &self) const
    {
        SourceLocation loc = globalLocation(self);
        return [loc, this](SourceLocation x) {
            return SourceLocation(x.offset - m_localOffset.offset + loc.offset, x.length,
                                  x.startLine - m_localOffset.startLine + loc.startLine,
                                  ((x.startLine == m_localOffset.startLine) ? x.startColumn
                                                   - m_localOffset.startColumn + loc.startColumn
                                                                            : x.startColumn));
        };
    }

    SourceLocation locationToLocal(SourceLocation x) const
    {
        return SourceLocation(
                x.offset - m_localOffset.offset, x.length, x.startLine - m_localOffset.startLine,
                ((x.startLine == m_localOffset.startLine)
                         ? x.startColumn - m_localOffset.startColumn
                         : x.startColumn)); // are line and column 1 based? then we should + 1
    }

    std::function<SourceLocation(SourceLocation)> locationToLocalF(const DomItem &) const
    {
        return [this](SourceLocation x) { return locationToLocal(x); };
    }

private:
    enum class ParseMode {
        QML,
        JS,
        ESM, // ECMAScript module
    };

    inline ParseMode resolveParseMode()
    {
        switch (m_expressionType) {
        case ExpressionType::BindingExpression:
            // unfortunately there are no documentation explaining this resolution
            // this was just moved from the original implementation
            return ParseMode::QML;
        case ExpressionType::ESMCode:
            return ParseMode::ESM;
        default:
            return ParseMode::JS;
        }
    }
    void setCode(const QString &code, const QString &preCode, const QString &postCode);
    [[nodiscard]] AST::Node *parse(ParseMode mode);

    ExpressionType m_expressionType;
    QString m_codeStr;
    QStringView m_code;
    QStringView m_preCode;
    QStringView m_postCode;
    mutable std::shared_ptr<QQmlJS::Engine> m_engine;
    mutable AST::Node *m_ast;
    std::shared_ptr<AstComments> m_astComments;
    SourceLocation m_localOffset;
    ScriptElementVariant m_element;
};

class BindingValue;

class QMLDOM_EXPORT Binding
{
public:
    constexpr static DomType kindValue = DomType::Binding;

    Binding(const QString &m_name = QString());
    Binding(const QString &m_name, std::unique_ptr<BindingValue> value,
            BindingType bindingType = BindingType::Normal);
    Binding(const QString &m_name, const std::shared_ptr<ScriptExpression> &value,
            BindingType bindingType = BindingType::Normal);
    Binding(const QString &m_name, const QString &scriptCode,
            BindingType bindingType = BindingType::Normal);
    Binding(const QString &m_name, const QmlObject &value,
            BindingType bindingType = BindingType::Normal);
    Binding(const QString &m_name, const QList<QmlObject> &value,
            BindingType bindingType = BindingType::Normal);
    Binding(const Binding &o);
    Binding(Binding &&o) = default;
    ~Binding();
    Binding &operator=(const Binding &);
    Binding &operator=(Binding &&) = default;

    bool iterateDirectSubpaths(const DomItem &self, DirectVisitor) const;
    DomItem valueItem(const DomItem &self) const; //  ### REVISIT: consider replacing return value with variant
    BindingValueKind valueKind() const;
    QString name() const { return m_name; }
    BindingType bindingType() const { return m_bindingType; }
    QmlObject const *objectValue() const;
    QList<QmlObject> const *arrayValue() const;
    std::shared_ptr<ScriptExpression> scriptExpressionValue() const;
    QmlObject *objectValue();
    QList<QmlObject> *arrayValue();
    std::shared_ptr<ScriptExpression> scriptExpressionValue();
    QList<QmlObject> annotations() const { return m_annotations; }
    void setAnnotations(const QList<QmlObject> &annotations) { m_annotations = annotations; }
    void setValue(std::unique_ptr<BindingValue> &&value);
    Path addAnnotation(const Path &selfPathFromOwner, const QmlObject &a, QmlObject **aPtr = nullptr);
    const RegionComments &comments() const { return m_comments; }
    RegionComments &comments() { return m_comments; }
    void updatePathFromOwner(const Path &newPath);
    void writeOut(const DomItem &self, OutWriter &lw) const;
    void writeOutValue(const DomItem &self, OutWriter &lw) const;
    bool isSignalHandler() const
    {
        QString baseName = m_name.split(QLatin1Char('.')).last();
        return QQmlSignalNames::isHandlerName(baseName);
    }
    static QString preCodeForName(QStringView n)
    {
        return QStringLiteral(u"QtObject{\n  %1: ").arg(n.split(u'.').last());
    }
    static QString postCodeForName(QStringView) { return QStringLiteral(u"\n}\n"); }
    QString preCode() const { return preCodeForName(m_name); }
    QString postCode() const { return postCodeForName(m_name); }

    ScriptElementVariant bindingIdentifiers() const { return m_bindingIdentifiers; }
    void setBindingIdentifiers(const ScriptElementVariant &bindingIdentifiers) { m_bindingIdentifiers = bindingIdentifiers; }

private:
    friend class QQmlDomAstCreator;
    BindingType m_bindingType;
    QString m_name;
    std::unique_ptr<BindingValue> m_value;
    QList<QmlObject> m_annotations;
    RegionComments m_comments;
    ScriptElementVariant m_bindingIdentifiers;
};

class QMLDOM_EXPORT AttributeInfo
{
public:
    enum Access { Private, Protected, Public };

    bool iterateDirectSubpaths(const DomItem &self, DirectVisitor visitor) const;

    Path addAnnotation(const Path &selfPathFromOwner, const QmlObject &annotation,
                       QmlObject **aPtr = nullptr);
    void updatePathFromOwner(const Path &newPath);

    QQmlJSScope::ConstPtr semanticScope() const { return m_semanticScope; }
    void setSemanticScope(const QQmlJSScope::ConstPtr &scope) { m_semanticScope = scope; }

    QString name;
    Access access = Access::Public;
    QString typeName;
    bool isReadonly = false;
    bool isList = false;
    QList<QmlObject> annotations;
    RegionComments comments;
    QQmlJSScope::ConstPtr m_semanticScope;
};

struct QMLDOM_EXPORT LocallyResolvedAlias
{
    enum class Status { Invalid, ResolvedProperty, ResolvedObject, Loop, TooDeep };
    bool valid()
    {
        switch (status) {
        case Status::ResolvedProperty:
        case Status::ResolvedObject:
            return true;
        default:
            return false;
        }
    }
    DomItem baseObject;
    DomItem localPropertyDef;
    QString typeName;
    QStringList accessedPath;
    Status status = Status::Invalid;
    int nAliases = 0;
};

class QMLDOM_EXPORT PropertyDefinition : public AttributeInfo
{
public:
    constexpr static DomType kindValue = DomType::PropertyDefinition;

    bool iterateDirectSubpaths(const DomItem &self, DirectVisitor visitor) const
    {
        bool cont = AttributeInfo::iterateDirectSubpaths(self, visitor);
        cont = cont && self.dvValueField(visitor, Fields::isPointer, isPointer);
        cont = cont && self.dvValueField(visitor, Fields::isFinal, isFinal);
        cont = cont && self.dvValueField(visitor, Fields::isAlias, isAlias());
        cont = cont && self.dvValueField(visitor, Fields::isDefaultMember, isDefaultMember);
        cont = cont && self.dvValueField(visitor, Fields::isRequired, isRequired);
        cont = cont && self.dvValueField(visitor, Fields::read, read);
        cont = cont && self.dvValueField(visitor, Fields::write, write);
        cont = cont && self.dvValueField(visitor, Fields::bindable, bindable);
        cont = cont && self.dvValueField(visitor, Fields::notify, notify);
        cont = cont && self.dvReferenceField(visitor, Fields::type, typePath());
        if (m_nameIdentifiers) {
            cont = cont && self.dvItemField(visitor, Fields::nameIdentifiers, [this, &self]() {
                return self.subScriptElementWrapperItem(m_nameIdentifiers);
            });
        }
        return cont;
    }

    Path typePath() const { return Paths::lookupTypePath(typeName); }

    bool isAlias() const { return typeName == u"alias"; }
    bool isParametricType() const;
    void writeOut(const DomItem &self, OutWriter &lw) const;
    ScriptElementVariant nameIdentifiers() const { return m_nameIdentifiers; }
    void setNameIdentifiers(const ScriptElementVariant &name) { m_nameIdentifiers = name; }

    QString read;
    QString write;
    QString bindable;
    QString notify;
    bool isFinal = false;
    bool isPointer = false;
    bool isDefaultMember = false;
    bool isRequired = false;
    ScriptElementVariant m_nameIdentifiers;
};

class QMLDOM_EXPORT PropertyInfo
{
public:
    constexpr static DomType kindValue = DomType::PropertyInfo; // used to get the correct kind in ObjectWrapper

    bool iterateDirectSubpaths(const DomItem &self, DirectVisitor visitor) const;

    QList<DomItem> propertyDefs;
    QList<DomItem> bindings;
};

class QMLDOM_EXPORT MethodParameter
{
public:
    constexpr static DomType kindValue = DomType::MethodParameter;
    enum class TypeAnnotationStyle {
        Prefix, // a(int x)
        Suffix, // a(x : int)
    };
    bool iterateDirectSubpaths(const DomItem &self, DirectVisitor visitor) const;

    void writeOut(const DomItem &self, OutWriter &ow) const;
    void writeOutSignal(const DomItem &self, OutWriter &ow) const;

    QString name;
    QString typeName;
    bool isPointer = false;
    bool isReadonly = false;
    bool isList = false;
    bool isRestElement = false;
    std::shared_ptr<ScriptExpression> defaultValue;
    /*!
        \internal
        Contains the scriptElement representing this argument, inclusive default value,
        deconstruction, etc.
     */
    std::shared_ptr<ScriptExpression> value;
    QList<QmlObject> annotations;
    RegionComments comments;
    TypeAnnotationStyle typeAnnotationStyle = TypeAnnotationStyle::Suffix;
};

// TODO (QTBUG-128423)
// Refactor to differentiate between Signals and Methods easily,
// considering their distinct handling and formatting needs.
// Explore separating Signal functionality or unifying shared methods.
class QMLDOM_EXPORT MethodInfo : public AttributeInfo
{
    Q_GADGET
public:
    enum MethodType { Signal, Method };
    Q_ENUM(MethodType)

    constexpr static DomType kindValue = DomType::MethodInfo;

    Path typePath(const DomItem &) const
    {
        return (typeName.isEmpty() ? Path() : Paths::lookupTypePath(typeName));
    }

    bool iterateDirectSubpaths(const DomItem &self, DirectVisitor visitor) const;
    QString preCode(const DomItem &) const; // ### REVISIT, might be simplified by using different toplevel production rules at usage site
    QString postCode(const DomItem &) const;
    void writePre(const DomItem &self, OutWriter &ow) const;
    void writeOut(const DomItem &self, OutWriter &ow) const;
    QString signature(const DomItem &self) const;

    void setCode(const QString &code)
    {
        body = std::make_shared<ScriptExpression>(
                code, ScriptExpression::ExpressionType::FunctionBody, 0,
                                     QLatin1String("function foo(){\n"), QLatin1String("\n}\n"));
    }
    MethodInfo() = default;

    // TODO: make private + add getters/setters
    QList<MethodParameter> parameters;
    MethodType methodType = Method;
    std::shared_ptr<ScriptExpression> body;
    std::shared_ptr<ScriptExpression> returnType;
    bool isConstructor = false;

private:
    void writeOutArguments(const DomItem &self, OutWriter &ow) const;
    void writeOutReturnType(OutWriter &ow) const;
    void writeOutBody(const DomItem &self, OutWriter &ow) const;
};

class QMLDOM_EXPORT EnumItem
{
public:
    constexpr static DomType kindValue = DomType::EnumItem;
    enum class ValueKind : quint8 {
        ImplicitValue,
        ExplicitValue
    };
    EnumItem(const QString &name = QString(), int value = 0, ValueKind valueKind = ValueKind::ImplicitValue)
        : m_name(name), m_value(value), m_valueKind(valueKind)
    {
    }

    bool iterateDirectSubpaths(const DomItem &self, DirectVisitor visitor) const;

    QString name() const { return m_name; }
    double value() const { return m_value; }
    RegionComments &comments() { return m_comments; }
    const RegionComments &comments() const { return m_comments; }
    void writeOut(const DomItem &self, OutWriter &lw) const;

private:
    QString m_name;
    double m_value;
    ValueKind m_valueKind;
    RegionComments m_comments;
};

class QMLDOM_EXPORT EnumDecl final : public CommentableDomElement
{
public:
    constexpr static DomType kindValue = DomType::EnumDecl;
    DomType kind() const override { return kindValue; }

    EnumDecl(const QString &name = QString(), QList<EnumItem> values = QList<EnumItem>(),
             Path pathFromOwner = Path())
        : CommentableDomElement(pathFromOwner), m_name(name), m_values(values)
    {
    }

    bool iterateDirectSubpaths(const DomItem &self, DirectVisitor visitor) const override;

    QString name() const { return m_name; }
    void setName(const QString &name) { m_name = name; }
    const QList<EnumItem> &values() const & { return m_values; }
    bool isFlag() const { return m_isFlag; }
    void setIsFlag(bool flag) { m_isFlag = flag; }
    QString alias() const { return m_alias; }
    void setAlias(const QString &aliasName) { m_alias = aliasName; }
    void setValues(QList<EnumItem> values) { m_values = values; }
    Path addValue(EnumItem value)
    {
        m_values.append(value);
        return Path::fromField(Fields::values).withIndex(index_type(m_values.size() - 1));
    }
    void updatePathFromOwner(const Path &newP) override;

    const QList<QmlObject> &annotations() const & { return m_annotations; }
    void setAnnotations(const QList<QmlObject> &annotations);
    Path addAnnotation(const QmlObject &child, QmlObject **cPtr = nullptr);
    void writeOut(const DomItem &self, OutWriter &lw) const override;

private:
    QString m_name;
    bool m_isFlag = false;
    QString m_alias;
    QList<EnumItem> m_values;
    QList<QmlObject> m_annotations;
};

class QMLDOM_EXPORT QmlObject final : public CommentableDomElement
{
    Q_DECLARE_TR_FUNCTIONS(QmlObject)
public:
    constexpr static DomType kindValue = DomType::QmlObject;
    DomType kind() const override { return kindValue; }

    QmlObject(const Path &pathFromOwner = Path());
    bool iterateDirectSubpaths(const DomItem &self, DirectVisitor) const override;
    bool iterateBaseDirectSubpaths(const DomItem &self, DirectVisitor) const;
    QList<QString> fields() const;
    QList<QString> fields(const DomItem &) const override { return fields(); }
    DomItem field(const DomItem &self, QStringView name) const override;
    void updatePathFromOwner(const Path &newPath) override;
    QString localDefaultPropertyName() const;
    QString defaultPropertyName(const DomItem &self) const;
    bool iterateSubOwners(const DomItem &self,
                          function_ref<bool(const DomItem &owner)> visitor) const;

    QString idStr() const { return m_idStr; }
    QString name() const { return m_name; }
    const QList<Path> &prototypePaths() const & { return m_prototypePaths; }
    Path nextScopePath() const { return m_nextScopePath; }
    const QMultiMap<QString, PropertyDefinition> &propertyDefs() const & { return m_propertyDefs; }
    const QMultiMap<QString, Binding> &bindings() const & { return m_bindings; }
    const QMultiMap<QString, MethodInfo> &methods() const & { return m_methods; }
    QList<QmlObject> children() const { return m_children; }
    QList<QmlObject> annotations() const { return m_annotations; }

    void setIdStr(const QString &id) { m_idStr = id; }
    void setName(const QString &name) { m_name = name; }
    void setDefaultPropertyName(const QString &name) { m_defaultPropertyName = name; }
    void setPrototypePaths(QList<Path> prototypePaths) { m_prototypePaths = prototypePaths; }
    Path addPrototypePath(const Path &prototypePath)
    {
        index_type idx = index_type(m_prototypePaths.indexOf(prototypePath));
        if (idx == -1) {
            idx = index_type(m_prototypePaths.size());
            m_prototypePaths.append(prototypePath);
        }
        return Path::fromField(Fields::prototypes).withIndex(idx);
    }
    void setNextScopePath(const Path &nextScopePath) { m_nextScopePath = nextScopePath; }
    void setPropertyDefs(QMultiMap<QString, PropertyDefinition> propertyDefs)
    {
        m_propertyDefs = propertyDefs;
    }
    void setBindings(QMultiMap<QString, Binding> bindings) { m_bindings = bindings; }
    void setMethods(QMultiMap<QString, MethodInfo> functionDefs) { m_methods = functionDefs; }
    void setChildren(const QList<QmlObject> &children)
    {
        m_children = children;
        if (pathFromOwner())
            updatePathFromOwner(pathFromOwner());
    }
    void setAnnotations(const QList<QmlObject> &annotations)
    {
        m_annotations = annotations;
        if (pathFromOwner())
            updatePathFromOwner(pathFromOwner());
    }
    Path addPropertyDef(const PropertyDefinition &propertyDef, AddOption option,
                        PropertyDefinition **pDef = nullptr)
    {
        return insertUpdatableElementInMultiMap(pathFromOwner().withField(Fields::propertyDefs),
                                                m_propertyDefs, propertyDef.name, propertyDef,
                                                option, pDef);
    }
    MutableDomItem addPropertyDef(MutableDomItem &self, const PropertyDefinition &propertyDef,
                                  AddOption option);

    Path addBinding(Binding binding, AddOption option, Binding **bPtr = nullptr)
    {
        return insertUpdatableElementInMultiMap(pathFromOwner().withField(Fields::bindings), m_bindings,
                                                binding.name(), binding, option, bPtr);
    }
    MutableDomItem addBinding(MutableDomItem &self, Binding binding, AddOption option);
    Path addMethod(const MethodInfo &functionDef, AddOption option, MethodInfo **mPtr = nullptr)
    {
        return insertUpdatableElementInMultiMap(pathFromOwner().withField(Fields::methods), m_methods,
                                                functionDef.name, functionDef, option, mPtr);
    }
    MutableDomItem addMethod(MutableDomItem &self, const MethodInfo &functionDef, AddOption option);
    Path addChild(QmlObject child, QmlObject **cPtr = nullptr)
    {
        return appendUpdatableElementInQList(pathFromOwner().withField(Fields::children), m_children,
                                             child, cPtr);
    }
    MutableDomItem addChild(MutableDomItem &self, QmlObject child)
    {
        Path p = addChild(child);
        return MutableDomItem(self.owner().item(), p);
    }
    Path addAnnotation(const QmlObject &annotation, QmlObject **aPtr = nullptr)
    {
        return appendUpdatableElementInQList(pathFromOwner().withField(Fields::annotations),
                                             m_annotations, annotation, aPtr);
    }

    QList<std::pair<SourceLocation, DomItem>> orderOfAttributes(const DomItem &self,
                                                            const DomItem &component) const;
    using Attributes = QList<std::pair<SourceLocation, DomItem>>;
    void writeOutAttributes(OutWriter &ow, const Attributes &attribs, const QString &code) const;

    void writeOutSortedEnumerations(const DomItem &component, OutWriter &ow) const;
    void writeOutSortedAttributes(const DomItem &self, OutWriter &ow, const DomItem &component) const;
    void writeOutSortedPropertyDefinition(const DomItem &self, OutWriter &ow,
                                          QSet<QString> &mergedDefBinding) const;

    void writeOutId(const DomItem &self, OutWriter &ow) const;
    void writeOut(const DomItem &self, OutWriter &ow, const QString &onTarget) const;
    void writeOut(const DomItem &self, OutWriter &lw) const override { writeOut(self, lw, QString()); }

    LocallyResolvedAlias resolveAlias(const DomItem &self,
                                      std::shared_ptr<ScriptExpression> accessSequence) const;
    LocallyResolvedAlias resolveAlias(const DomItem &self, const QStringList &accessSequence) const;

    QQmlJSScope::ConstPtr semanticScope() const { return m_scope; }
    void setSemanticScope(const QQmlJSScope::ConstPtr &scope) { m_scope = scope; }

    ScriptElementVariant nameIdentifiers() const { return m_nameIdentifiers; }
    void setNameIdentifiers(const ScriptElementVariant &name) { m_nameIdentifiers = name; }

private:
    friend class QQmlDomAstCreator;
    QString m_idStr;
    QString m_name;
    QList<Path> m_prototypePaths;
    Path m_nextScopePath;
    QString m_defaultPropertyName;
    QMultiMap<QString, PropertyDefinition> m_propertyDefs;
    QMultiMap<QString, Binding> m_bindings;
    QMultiMap<QString, MethodInfo> m_methods;
    QList<QmlObject> m_children;
    QList<QmlObject> m_annotations;
    QQmlJSScope::ConstPtr m_scope;
    ScriptElementVariant m_nameIdentifiers;

    static constexpr quint32 posOfNewElements = std::numeric_limits<quint32>::max();
};

class Export
{
    Q_DECLARE_TR_FUNCTIONS(Export)
public:
    constexpr static DomType kindValue = DomType::Export;
    static Export fromString(
            const Path &source, QStringView exp, const Path &typePath, const ErrorHandler &h);
    bool iterateDirectSubpaths(const DomItem &self, DirectVisitor visitor) const
    {
        bool cont = true;
        cont = cont && self.dvValueField(visitor, Fields::uri, uri);
        cont = cont && self.dvValueField(visitor, Fields::typeName, typeName);
        cont = cont && self.dvWrapField(visitor, Fields::version, version);
        if (typePath)
            cont = cont && self.dvReferenceField(visitor, Fields::type, typePath);
        cont = cont && self.dvValueField(visitor, Fields::isInternal, isInternal);
        cont = cont && self.dvValueField(visitor, Fields::isSingleton, isSingleton);
        if (exportSourcePath)
            cont = cont && self.dvReferenceField(visitor, Fields::exportSource, exportSourcePath);
        return cont;
    }

    Path exportSourcePath;
    QString uri;
    QString typeName;
    Version version;
    Path typePath;
    bool isInternal = false;
    bool isSingleton = false;
};

class QMLDOM_EXPORT Component : public CommentableDomElement
{
public:
    Component(const QString &name);
    Component(const Path &pathFromOwner = Path());
    Component(const Component &o) = default;
    Component &operator=(const Component &) = default;

    bool iterateDirectSubpaths(const DomItem &, DirectVisitor) const override;
    void updatePathFromOwner(const Path &newPath) override;
    DomItem field(const DomItem &self, QStringView name) const override;

    QString name() const { return m_name; }
    const QMultiMap<QString, EnumDecl> &enumerations() const & { return m_enumerations; }
    const QList<QmlObject> &objects() const & { return m_objects; }
    bool isSingleton() const { return m_isSingleton; }
    bool isCreatable() const { return m_isCreatable; }
    bool isComposite() const { return m_isComposite; }
    QString attachedTypeName() const { return m_attachedTypeName; }
    Path attachedTypePath(const DomItem &) const { return m_attachedTypePath; }

    void setName(const QString &name) { m_name = name; }
    void setEnumerations(QMultiMap<QString, EnumDecl> enumerations)
    {
        m_enumerations = enumerations;
    }
    Path addEnumeration(const EnumDecl &enumeration, AddOption option = AddOption::Overwrite,
                        EnumDecl **ePtr = nullptr)
    {
        return insertUpdatableElementInMultiMap(pathFromOwner().withField(Fields::enumerations),
                                                m_enumerations, enumeration.name(), enumeration,
                                                option, ePtr);
    }
    void setObjects(const QList<QmlObject> &objects) { m_objects = objects; }
    Path addObject(const QmlObject &object, QmlObject **oPtr = nullptr);
    void setIsSingleton(bool isSingleton) { m_isSingleton = isSingleton; }
    void setIsCreatable(bool isCreatable) { m_isCreatable = isCreatable; }
    void setIsComposite(bool isComposite) { m_isComposite = isComposite; }
    void setAttachedTypeName(const QString &name) { m_attachedTypeName = name; }
    void setAttachedTypePath(const Path &p) { m_attachedTypePath = p; }

private:
    friend class QQmlDomAstCreator;
    QString m_name;
    QMultiMap<QString, EnumDecl> m_enumerations;
    QList<QmlObject> m_objects;
    bool m_isSingleton = false;
    bool m_isCreatable = true;
    bool m_isComposite = true;
    QString m_attachedTypeName;
    Path m_attachedTypePath;
};

class QMLDOM_EXPORT JsResource final : public Component
{
public:
    constexpr static DomType kindValue = DomType::JsResource;
    DomType kind() const override { return kindValue; }

    JsResource(const Path &pathFromOwner = Path()) : Component(pathFromOwner) { }
    bool iterateDirectSubpaths(const DomItem &, DirectVisitor) const override
    { // to do: complete
        return true;
    }
    // globalSymbols defined/exported, required/used
};

class QMLDOM_EXPORT QmltypesComponent final : public Component
{
public:
    constexpr static DomType kindValue = DomType::QmltypesComponent;
    DomType kind() const override { return kindValue; }

    QmltypesComponent(const Path &pathFromOwner = Path()) : Component(pathFromOwner) { }
    bool iterateDirectSubpaths(const DomItem &, DirectVisitor) const override;
    const QList<Export> &exports() const & { return m_exports; }
    QString fileName() const { return m_fileName; }
    void setExports(QList<Export> exports) { m_exports = exports; }
    void addExport(const Export &exportedEntry) { m_exports.append(exportedEntry); }
    void setFileName(const QString &fileName) { m_fileName = fileName; }
    const QList<int> &metaRevisions() const & { return m_metaRevisions; }
    void setMetaRevisions(QList<int> metaRevisions) { m_metaRevisions = metaRevisions; }
    void setInterfaceNames(const QStringList& interfaces) { m_interfaceNames = interfaces; }
    const QStringList &interfaceNames() const & { return m_interfaceNames; }
    QString extensionTypeName() const { return m_extensionTypeName; }
    void setExtensionTypeName(const QString &name) { m_extensionTypeName =  name; }
    QString valueTypeName() const { return m_valueTypeName; }
    void setValueTypeName(const QString &name) { m_valueTypeName = name; }
    bool hasCustomParser() const { return m_hasCustomParser; }
    void setHasCustomParser(bool v) { m_hasCustomParser = v; }
    bool extensionIsJavaScript() const { return m_extensionIsJavaScript; }
    void setExtensionIsJavaScript(bool v) { m_extensionIsJavaScript = v; }
    bool extensionIsNamespace() const { return m_extensionIsNamespace; }
    void setExtensionIsNamespace(bool v) { m_extensionIsNamespace = v; }
    QQmlJSScope::AccessSemantics accessSemantics() const { return m_accessSemantics; }
    void setAccessSemantics(QQmlJSScope::AccessSemantics v) { m_accessSemantics = v; }

    void setSemanticScope(const QQmlJSScope::ConstPtr &scope) { m_semanticScope = scope; }
    QQmlJSScope::ConstPtr semanticScope() const { return m_semanticScope; }

private:
    QList<Export> m_exports;
    QList<int> m_metaRevisions;
    QString m_fileName; // remove?
    QStringList m_interfaceNames;
    bool m_hasCustomParser = false;
    bool m_extensionIsJavaScript = false;
    bool m_extensionIsNamespace = false;
    QString m_valueTypeName;
    QString m_extensionTypeName;
    QQmlJSScope::AccessSemantics m_accessSemantics = QQmlJSScope::AccessSemantics::None;
    QQmlJSScope::ConstPtr m_semanticScope;
};

class QMLDOM_EXPORT QmlComponent final : public Component
{
public:
    constexpr static DomType kindValue = DomType::QmlComponent;
    DomType kind() const override { return kindValue; }

    QmlComponent(const QString &name = QString()) : Component(name)
    {
        setIsComposite(true);
        setIsCreatable(true);
    }

    bool iterateDirectSubpaths(const DomItem &self, DirectVisitor) const override;

    const QMultiMap<QString, Id> &ids() const & { return m_ids; }
    Path nextComponentPath() const { return m_nextComponentPath; }
    void setIds(QMultiMap<QString, Id> ids) { m_ids = ids; }
    void setNextComponentPath(const Path &p) { m_nextComponentPath = p; }
    void updatePathFromOwner(const Path &newPath) override;
    Path addId(const Id &id, AddOption option = AddOption::Overwrite, Id **idPtr = nullptr)
    {
        // warning does nor remove old idStr when overwriting...
        return insertUpdatableElementInMultiMap(pathFromOwner().withField(Fields::ids), m_ids, id.name,
                                                id, option, idPtr);
    }
    void writeOut(const DomItem &self, OutWriter &) const override;
    QList<QString> subComponentsNames(const DomItem &self) const;
    QList<DomItem> subComponents(const DomItem &self) const;

    void setSemanticScope(const QQmlJSScope::ConstPtr &scope) { m_semanticScope = scope; }
    QQmlJSScope::ConstPtr semanticScope() const { return m_semanticScope; }
    ScriptElementVariant nameIdentifiers() const { return m_nameIdentifiers; }
    void setNameIdentifiers(const ScriptElementVariant &name) { m_nameIdentifiers = name; }

private:
    friend class QQmlDomAstCreator;
    Path m_nextComponentPath;
    QMultiMap<QString, Id> m_ids;
    QQmlJSScope::ConstPtr m_semanticScope;
    // m_nameIdentifiers contains the name of the component as FieldMemberExpression, and therefore
    // only exists in inline components!
    ScriptElementVariant m_nameIdentifiers;
};

class QMLDOM_EXPORT GlobalComponent final : public Component
{
public:
    constexpr static DomType kindValue = DomType::GlobalComponent;
    DomType kind() const override { return kindValue; }

    GlobalComponent(const Path &pathFromOwner = Path()) : Component(pathFromOwner) { }
};

static ErrorGroups importErrors = { { DomItem::domErrorGroup, NewErrorGroup("importError") } };

class QMLDOM_EXPORT ImportScope
{
    Q_DECLARE_TR_FUNCTIONS(ImportScope)
public:
    constexpr static DomType kindValue = DomType::ImportScope;

    ImportScope() = default;

    const QList<Path> &importSourcePaths() const & { return m_importSourcePaths; }

    const QMap<QString, ImportScope> &subImports() const & { return m_subImports; }

    QList<Path> allSources(const DomItem &self) const;

    QSet<QString> importedNames(const DomItem &self) const
    {
        QSet<QString> res;
        const auto sources = allSources(self);
        for (const Path &p : sources) {
            QSet<QString> ks = self.path(p.withField(Fields::exports), self.errorHandler()).keys();
            res += ks;
        }
        return res;
    }

    QList<DomItem> importedItemsWithName(const DomItem &self, const QString &name) const
    {
        QList<DomItem> res;
        const auto sources = allSources(self);
        for (const Path &p : sources) {
            DomItem source = self.path(p.withField(Fields::exports), self.errorHandler());
            DomItem els = source.key(name);
            int nEls = els.indexes();
            for (int i = 0; i < nEls; ++i)
                res.append(els.index(i));
            if (nEls == 0 && els) {
                self.addError(importErrors.warning(
                        tr("Looking up '%1' expected a list of exports, not %2")
                                .arg(name, els.toString())));
            }
        }
        return res;
    }

    QList<Export> importedExportsWithName(const DomItem &self, const QString &name) const
    {
        QList<Export> res;
        for (const DomItem &i : importedItemsWithName(self, name))
            if (const Export *e = i.as<Export>())
                res.append(*e);
            else
                self.addError(importErrors.warning(
                        tr("Expected Export looking up '%1', not %2").arg(name, i.toString())));
        return res;
    }

    bool iterateDirectSubpaths(const DomItem &self, DirectVisitor visitor) const;

    void addImport(QStringList p, const Path &targetExports)
    {
        if (!p.isEmpty()) {
            const QString current = p.takeFirst();
            m_subImports[current].addImport(std::move(p), targetExports);
        } else if (!m_importSourcePaths.contains(targetExports)) {
            m_importSourcePaths.append(targetExports);
        }
    }

private:
    QList<Path> m_importSourcePaths;
    QMap<QString, ImportScope> m_subImports;
};

class BindingValue
{
public:
    BindingValue();
    BindingValue(const QmlObject &o);
    BindingValue(const std::shared_ptr<ScriptExpression> &o);
    BindingValue(const QList<QmlObject> &l);
    ~BindingValue();
    BindingValue(const BindingValue &o);
    BindingValue &operator=(const BindingValue &o);

    DomItem value(const DomItem &binding) const;
    void updatePathFromOwner(const Path &newPath);

private:
    friend class Binding;
    void clearValue();

    BindingValueKind kind;
    union {
        int dummy;
        QmlObject object;
        std::shared_ptr<ScriptExpression> scriptExpression;
        QList<QmlObject> array;
    };
};

} // end namespace Dom
} // end namespace QQmlJS
QT_END_NAMESPACE
#endif // QQMLDOMELEMENTS_P_H
