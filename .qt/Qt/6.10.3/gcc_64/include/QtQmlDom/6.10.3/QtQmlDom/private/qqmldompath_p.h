// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QMLDOM_PATH_H
#define QMLDOM_PATH_H

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

#include "qqmldomconstants_p.h"
#include "qqmldomstringdumper_p.h"
#include "qqmldom_global.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QMetaEnum>
#include <QtCore/QString>
#include <QtCore/QStringView>
#include <QtCore/QStringList>
#include <QtCore/QVector>
#include <QtCore/QDebug>

#include <functional>
#include <iterator>

QT_BEGIN_NAMESPACE

namespace QQmlJS {
namespace Dom {

class ErrorGroups;
class ErrorMessage;
class DomItem;
class Path;

using ErrorHandler = std::function<void(const ErrorMessage &)> ;

using index_type = qint64;

namespace PathEls {

enum class Kind{
    Empty,
    Field,
    Index,
    Key,
    Root,
    Current,
    Any,
    Filter
};

class TestPaths;
class Empty;
class Field;
class Index;
class Key;
class Root;
class Current;
class Any;
class Filter;

class Base {
public:
    QStringView stringView() const { return QStringView(); }
    index_type index(index_type defaultValue = -1) const { return defaultValue; }
    bool hasSquareBrackets() const { return false; }

protected:
    void dump(const Sink &sink, const QString &name, bool hasSquareBrackets) const;
};

class Empty final : public Base
{
public:
    Empty() = default;
    QString name() const { return QString(); }
    bool checkName(QStringView s) const { return s.isEmpty(); }
    void dump(const Sink &sink) const { Base::dump(sink, name(), hasSquareBrackets()); }
};

class Field final : public Base
{
public:
    Field() = default;
    Field(QStringView n): fieldName(n) {}
    QString name() const { return fieldName.toString(); }
    bool checkName(QStringView s) const { return s == fieldName; }
    QStringView stringView() const { return fieldName; }
    void dump(const Sink &sink) const { sink(fieldName); }

    QStringView fieldName;
};

class Index final : public Base
{
public:
    Index() = default;
    Index(index_type i): indexValue(i) {}
    QString name() const { return QString::number(indexValue); }
    bool checkName(QStringView s) const { return s == name(); }
    index_type index(index_type = -1) const { return indexValue; }
    void dump(const Sink &sink) const { Base::dump(sink, name(), hasSquareBrackets()); }
    bool hasSquareBrackets() const { return true; }

    index_type indexValue = -1;
};

class Key final : public Base
{
public:
    Key() = default;
    Key(const QString &n) : keyValue(n) { }
    QString name() const { return keyValue; }
    bool checkName(QStringView s) const { return s == keyValue; }
    QStringView stringView() const { return keyValue; }
    void dump(const Sink &sink) const {
        sink(u"[");
        sinkEscaped(sink, keyValue);
        sink(u"]");
    }
    bool hasSquareBrackets() const { return true; }

    QString keyValue;
};

class Root final : public Base
{
public:
    Root() = default;
    Root(PathRoot r): contextKind(r), contextName() {}
    Root(QStringView n) {
        QMetaEnum metaEnum = QMetaEnum::fromType<PathRoot>();
        contextKind = PathRoot::Other;
        for (int i = 0; i < metaEnum.keyCount(); ++ i)
            if (n.compare(QString::fromUtf8(metaEnum.key(i)), Qt::CaseInsensitive) == 0)
                contextKind = PathRoot(metaEnum.value(i));
        if (contextKind == PathRoot::Other)
            contextName = n;
    }
    QString name() const {
        switch (contextKind) {
        case PathRoot::Modules:
            return QStringLiteral(u"$modules");
        case PathRoot::Cpp:
            return QStringLiteral(u"$cpp");
        case PathRoot::Libs:
            return QStringLiteral(u"$libs");
        case PathRoot::Top:
            return QStringLiteral(u"$top");
        case PathRoot::Env:
            return QStringLiteral(u"$env");
        case PathRoot::Universe:
            return QStringLiteral(u"$universe");
        case PathRoot::Other:
            return QString::fromUtf8("$").append(contextName.toString());
        }
        Q_ASSERT(false && "Unexpected contextKind in name");
        return QString();
    }
    bool checkName(QStringView s) const {
        if (contextKind != PathRoot::Other)
            return s.compare(name(), Qt::CaseInsensitive) == 0;
        return s.startsWith(QChar::fromLatin1('$')) && s.mid(1) == contextName;
    }
    QStringView stringView() const { return contextName; }
    void dump(const Sink &sink) const { sink(name()); }

    PathRoot contextKind = PathRoot::Other;
    QStringView contextName;
};

class Current final : public Base
{
public:
    Current() = default;
    Current(PathCurrent c): contextKind(c) {}
    Current(QStringView n) {
        QMetaEnum metaEnum = QMetaEnum::fromType<PathCurrent>();
        contextKind = PathCurrent::Other;
        for (int i = 0; i < metaEnum.keyCount(); ++ i)
            if (n.compare(QString::fromUtf8(metaEnum.key(i)), Qt::CaseInsensitive) == 0)
                contextKind = PathCurrent(metaEnum.value(i));
        if (contextKind == PathCurrent::Other)
            contextName = n;
    }
    QString name() const {
        switch (contextKind) {
        case PathCurrent::Other:
            return QString::fromUtf8("@").append(contextName.toString());
        case PathCurrent::Obj:
            return QStringLiteral(u"@obj");
        case PathCurrent::ObjChain:
            return QStringLiteral(u"@objChain");
        case PathCurrent::ScopeChain:
            return QStringLiteral(u"@scopeChain");
        case PathCurrent::Component:
            return QStringLiteral(u"@component");
        case PathCurrent::Module:
            return QStringLiteral(u"@module");
        case PathCurrent::Ids:
            return QStringLiteral(u"@ids");
        case PathCurrent::Types:
            return QStringLiteral(u"@types");
        case PathCurrent::LookupStrict:
            return QStringLiteral(u"@lookupStrict");
        case PathCurrent::LookupDynamic:
            return QStringLiteral(u"@lookupDynamic");
        case PathCurrent::Lookup:
            return QStringLiteral(u"@lookup");
        }
        Q_ASSERT(false && "Unexpected contextKind in Current::name");
        return QString();
    }
    bool checkName(QStringView s) const {
        if (contextKind != PathCurrent::Other)
            return s.compare(name(), Qt::CaseInsensitive) == 0;
        return s.startsWith(QChar::fromLatin1('@')) && s.mid(1) == contextName;
    }
    QStringView stringView() const { return contextName; }
    void dump(const Sink &sink) const { Base::dump(sink, name(), hasSquareBrackets()); }

    PathCurrent contextKind = PathCurrent::Other;
    QStringView contextName;
};

class Any final : public Base
{
public:
    Any() = default;
    QString name() const { return QLatin1String("*"); }
    bool checkName(QStringView s) const { return s == u"*"; }
    void dump(const Sink &sink) const { Base::dump(sink, name(), hasSquareBrackets()); }
    bool hasSquareBrackets() const { return true; }
};

class QMLDOM_EXPORT Filter final : public Base
{
public:
    Filter() = default;
    Filter(const std::function<bool(const DomItem &)> &f,
           QStringView filterDescription = u"<native code filter>");
    QString name() const;
    bool checkName(QStringView s) const;
    QStringView stringView() const { return filterDescription; }
    void dump(const Sink &sink) const { Base::dump(sink, name(), hasSquareBrackets()); }
    bool hasSquareBrackets() const { return true; }

    std::function<bool(const DomItem &)> filterFunction;
    QStringView filterDescription;
};

class QMLDOM_EXPORT PathComponent {
public:
    PathComponent() = default;
    PathComponent(const PathComponent &) = default;
    PathComponent(PathComponent &&) = default;
    PathComponent &operator=(const PathComponent &) = default;
    PathComponent &operator=(PathComponent &&) = default;
    ~PathComponent() = default;

    Kind kind() const { return Kind(m_data.index()); }

    QString name() const
    {
        return std::visit([](auto &&d) { return d.name(); }, m_data);
    }

    bool checkName(QStringView s) const
    {
        return std::visit([s](auto &&d) { return d.checkName(s); }, m_data);
    }

    QStringView stringView() const
    {
        return std::visit([](auto &&d) { return d.stringView(); }, m_data);
    }

    index_type index(index_type defaultValue=-1) const
    {
        return std::visit([defaultValue](auto &&d) { return d.index(defaultValue); }, m_data);
    }

    void dump(const Sink &sink) const
    {
        return std::visit([sink](auto &&d) { return d.dump(sink); }, m_data);
    }

    bool hasSquareBrackets() const
    {
        return std::visit([](auto &&d) { return d.hasSquareBrackets(); }, m_data);
    }

    const Empty   *asEmpty()   const { return std::get_if<Empty>(&m_data); }
    const Field   *asField()   const { return std::get_if<Field>(&m_data); }
    const Index   *asIndex()   const { return std::get_if<Index>(&m_data); }
    const Key     *asKey()     const { return std::get_if<Key>(&m_data); }
    const Root    *asRoot()    const { return std::get_if<Root>(&m_data); }
    const Current *asCurrent() const { return std::get_if<Current>(&m_data); }
    const Any     *asAny()     const { return std::get_if<Any>(&m_data); }
    const Filter  *asFilter()  const { return std::get_if<Filter>(&m_data); }

    static int cmp(const PathComponent &p1, const PathComponent &p2);

    PathComponent(Empty &&o): m_data(std::move(o)) {}
    PathComponent(Field &&o): m_data(std::move(o)) {}
    PathComponent(Index &&o): m_data(std::move(o)) {}
    PathComponent(Key &&o): m_data(std::move(o)) {}
    PathComponent(Root &&o): m_data(std::move(o)) {}
    PathComponent(Current &&o): m_data(std::move(o)) {}
    PathComponent(Any &&o): m_data(std::move(o)) {}
    PathComponent(Filter &&o): m_data(std::move(o)) {}

private:
    friend class QQmlJS::Dom::Path;
    friend class QQmlJS::Dom::PathEls::TestPaths;

    using Variant = std::variant<Empty, Field, Index, Key, Root, Current, Any, Filter>;

    template<typename T, Kind K>
    static constexpr bool variantTypeMatches
            = std::is_same_v<std::variant_alternative_t<size_t(K), Variant>, T>;

    static_assert(size_t(Kind::Empty) == 0);
    static_assert(variantTypeMatches<Empty,   Kind::Empty>);
    static_assert(variantTypeMatches<Field,   Kind::Field>);
    static_assert(variantTypeMatches<Key,     Kind::Key>);
    static_assert(variantTypeMatches<Root,    Kind::Root>);
    static_assert(variantTypeMatches<Current, Kind::Current>);
    static_assert(variantTypeMatches<Any,     Kind::Any>);
    static_assert(variantTypeMatches<Filter,  Kind::Filter>);
    static_assert(std::variant_size_v<Variant> == size_t(Kind::Filter) + 1);

    Variant m_data;
};

inline bool operator==(const PathComponent& lhs, const PathComponent& rhs){ return PathComponent::cmp(lhs,rhs) == 0; }
inline bool operator!=(const PathComponent& lhs, const PathComponent& rhs){ return PathComponent::cmp(lhs,rhs) != 0; }
inline bool operator< (const PathComponent& lhs, const PathComponent& rhs){ return PathComponent::cmp(lhs,rhs) <  0; }
inline bool operator> (const PathComponent& lhs, const PathComponent& rhs){ return PathComponent::cmp(lhs,rhs) >  0; }
inline bool operator<=(const PathComponent& lhs, const PathComponent& rhs){ return PathComponent::cmp(lhs,rhs) <= 0; }
inline bool operator>=(const PathComponent& lhs, const PathComponent& rhs){ return PathComponent::cmp(lhs,rhs) >= 0; }

class PathData {
public:
    PathData(const QStringList &strData, const QVector<PathComponent> &components)
        : strData(strData), components(components)
    {}
    PathData(const QStringList &strData, const QVector<PathComponent> &components,
             const std::shared_ptr<PathData> &parent)
        : strData(strData), components(components), parent(parent)
    {}

    QStringList strData;
    QVector<PathComponent> components;
    std::shared_ptr<PathData> parent;
};

} // namespace PathEls

#define QMLDOM_USTRING(s) QStringView(u##s)
#define QMLDOM_FIELD(name) inline constexpr const auto name = QMLDOM_USTRING(#name)
/*!
   \internal
   In an ideal world, the Fields namespace would be an enum, not strings.
   Use FieldType whenever you expect a static String from the Fields namespace instead of an
   arbitrary QStringView.
 */
using FieldType = QStringView;
// namespace, so it cam be reopened to add more entries
namespace Fields{
QMLDOM_FIELD(access);
QMLDOM_FIELD(accessSemantics);
QMLDOM_FIELD(allSources);
QMLDOM_FIELD(alternative);
QMLDOM_FIELD(annotations);
QMLDOM_FIELD(arguments);
QMLDOM_FIELD(astComments);
QMLDOM_FIELD(astRelocatableDump);
QMLDOM_FIELD(attachedType);
QMLDOM_FIELD(attachedTypeName);
QMLDOM_FIELD(autoExports);
QMLDOM_FIELD(base);
QMLDOM_FIELD(binaryExpression);
QMLDOM_FIELD(bindable);
QMLDOM_FIELD(bindingElement);
QMLDOM_FIELD(bindingIdentifiers);
QMLDOM_FIELD(bindingType);
QMLDOM_FIELD(bindings);
QMLDOM_FIELD(block);
QMLDOM_FIELD(body);
QMLDOM_FIELD(callee);
QMLDOM_FIELD(canonicalFilePath);
QMLDOM_FIELD(canonicalPath);
QMLDOM_FIELD(caseBlock);
QMLDOM_FIELD(caseClause);
QMLDOM_FIELD(caseClauses);
QMLDOM_FIELD(catchBlock);
QMLDOM_FIELD(catchParameter);
QMLDOM_FIELD(children);
QMLDOM_FIELD(classNames);
QMLDOM_FIELD(code);
QMLDOM_FIELD(commentedElements);
QMLDOM_FIELD(comments);
QMLDOM_FIELD(components);
QMLDOM_FIELD(condition);
QMLDOM_FIELD(consequence);
QMLDOM_FIELD(contents);
QMLDOM_FIELD(contentsDate);
QMLDOM_FIELD(cppType);
QMLDOM_FIELD(currentExposedAt);
QMLDOM_FIELD(currentIsValid);
QMLDOM_FIELD(currentItem);
QMLDOM_FIELD(currentRevision);
QMLDOM_FIELD(declarations);
QMLDOM_FIELD(defaultClause);
QMLDOM_FIELD(defaultPropertyName);
QMLDOM_FIELD(defaultValue);
QMLDOM_FIELD(designerSupported);
QMLDOM_FIELD(elLocation);
QMLDOM_FIELD(elements);
QMLDOM_FIELD(elementCanonicalPath);
QMLDOM_FIELD(enumerations);
QMLDOM_FIELD(errors);
QMLDOM_FIELD(exportSource);
QMLDOM_FIELD(exports);
QMLDOM_FIELD(expr);
QMLDOM_FIELD(expression);
QMLDOM_FIELD(expressionType);
QMLDOM_FIELD(extensionTypeName);
QMLDOM_FIELD(fileLocationsTree);
QMLDOM_FIELD(fileName);
QMLDOM_FIELD(finallyBlock);
QMLDOM_FIELD(regExpFlags);
QMLDOM_FIELD(forStatement);
QMLDOM_FIELD(fullRegion);
QMLDOM_FIELD(get);
QMLDOM_FIELD(globalScopeName);
QMLDOM_FIELD(globalScopeWithName);
QMLDOM_FIELD(hasCallback);
QMLDOM_FIELD(hasCustomParser);
QMLDOM_FIELD(idStr);
QMLDOM_FIELD(identifier);
QMLDOM_FIELD(ids);
QMLDOM_FIELD(implicit);
QMLDOM_FIELD(import);
QMLDOM_FIELD(importId);
QMLDOM_FIELD(importScope);
QMLDOM_FIELD(importSources);
QMLDOM_FIELD(imported);
QMLDOM_FIELD(imports);
QMLDOM_FIELD(inProgress);
QMLDOM_FIELD(infoItem);
QMLDOM_FIELD(inheritVersion);
QMLDOM_FIELD(initializer);
QMLDOM_FIELD(interfaceNames);
QMLDOM_FIELD(isAlias);
QMLDOM_FIELD(isComposite);
QMLDOM_FIELD(isConstructor);
QMLDOM_FIELD(isCreatable);
QMLDOM_FIELD(isDefaultMember);
QMLDOM_FIELD(isFinal);
QMLDOM_FIELD(isInternal);
QMLDOM_FIELD(isLatest);
QMLDOM_FIELD(isList);
QMLDOM_FIELD(isPointer);
QMLDOM_FIELD(isReadonly);
QMLDOM_FIELD(isRequired);
QMLDOM_FIELD(isSignalHandler);
QMLDOM_FIELD(isSingleton);
QMLDOM_FIELD(isValid);
QMLDOM_FIELD(jsFileWithPath);
QMLDOM_FIELD(kind);
QMLDOM_FIELD(lastRevision);
QMLDOM_FIELD(label);
QMLDOM_FIELD(lastValidRevision);
QMLDOM_FIELD(left);
QMLDOM_FIELD(loadInfo);
QMLDOM_FIELD(loadOptions);
QMLDOM_FIELD(loadPaths);
QMLDOM_FIELD(loadsWithWork);
QMLDOM_FIELD(localOffset);
QMLDOM_FIELD(location);
QMLDOM_FIELD(logicalPath);
QMLDOM_FIELD(majorVersion);
QMLDOM_FIELD(metaRevisions);
QMLDOM_FIELD(methodType);
QMLDOM_FIELD(methods);
QMLDOM_FIELD(minorVersion);
QMLDOM_FIELD(moduleIndex);
QMLDOM_FIELD(moduleIndexWithUri);
QMLDOM_FIELD(moduleScope);
QMLDOM_FIELD(moreCaseClauses);
QMLDOM_FIELD(nAllLoadedCallbacks);
QMLDOM_FIELD(nCallbacks);
QMLDOM_FIELD(nLoaded);
QMLDOM_FIELD(nNotdone);
QMLDOM_FIELD(name);
QMLDOM_FIELD(nameIdentifiers);
QMLDOM_FIELD(newlinesBefore);
QMLDOM_FIELD(nextComponent);
QMLDOM_FIELD(nextScope);
QMLDOM_FIELD(notify);
QMLDOM_FIELD(objects);
QMLDOM_FIELD(onAttachedObject);
QMLDOM_FIELD(operation);
QMLDOM_FIELD(options);
QMLDOM_FIELD(parameters);
QMLDOM_FIELD(parent);
QMLDOM_FIELD(parentObject);
QMLDOM_FIELD(path);
QMLDOM_FIELD(regExpPattern);
QMLDOM_FIELD(plugins);
QMLDOM_FIELD(postCode);
QMLDOM_FIELD(postComments);
QMLDOM_FIELD(pragma);
QMLDOM_FIELD(pragmas);
QMLDOM_FIELD(preCode);
QMLDOM_FIELD(preComments);
QMLDOM_FIELD(properties);
QMLDOM_FIELD(propertyDef);
QMLDOM_FIELD(propertyDefRef);
QMLDOM_FIELD(propertyDefs);
QMLDOM_FIELD(propertyInfos);
QMLDOM_FIELD(propertyName);
QMLDOM_FIELD(prototypes);
QMLDOM_FIELD(qmlDirectoryWithPath);
QMLDOM_FIELD(qmlFileWithPath);
QMLDOM_FIELD(qmlFiles);
QMLDOM_FIELD(qmldirFileWithPath);
QMLDOM_FIELD(qmldirWithPath);
QMLDOM_FIELD(qmltypesFileWithPath);
QMLDOM_FIELD(qmltypesFiles);
QMLDOM_FIELD(qualifiedImports);
QMLDOM_FIELD(rawComment);
QMLDOM_FIELD(read);
QMLDOM_FIELD(referredObject);
QMLDOM_FIELD(referredObjectPath);
QMLDOM_FIELD(regionComments);
QMLDOM_FIELD(regions);
QMLDOM_FIELD(requestedAt);
QMLDOM_FIELD(requestingUniverse);
QMLDOM_FIELD(returnType);
QMLDOM_FIELD(returnTypeName);
QMLDOM_FIELD(right);
QMLDOM_FIELD(rootComponent);
QMLDOM_FIELD(scopeType);
QMLDOM_FIELD(scriptElement);
QMLDOM_FIELD(sources);
QMLDOM_FIELD(statement);
QMLDOM_FIELD(statements);
QMLDOM_FIELD(status);
QMLDOM_FIELD(stringValue);
QMLDOM_FIELD(subComponents);
QMLDOM_FIELD(subImports);
QMLDOM_FIELD(subItems);
QMLDOM_FIELD(symbol);
QMLDOM_FIELD(symbols);
QMLDOM_FIELD(target);
QMLDOM_FIELD(targetPropertyName);
QMLDOM_FIELD(templateLiteral);
QMLDOM_FIELD(text);
QMLDOM_FIELD(type);
QMLDOM_FIELD(typeArgument);
QMLDOM_FIELD(typeArgumentName);
QMLDOM_FIELD(typeName);
QMLDOM_FIELD(types);
QMLDOM_FIELD(universe);
QMLDOM_FIELD(uri);
QMLDOM_FIELD(uris);
QMLDOM_FIELD(validExposedAt);
QMLDOM_FIELD(validItem);
QMLDOM_FIELD(value);
QMLDOM_FIELD(valueTypeName);
QMLDOM_FIELD(values);
QMLDOM_FIELD(version);
QMLDOM_FIELD(when);
QMLDOM_FIELD(write);
} // namespace Fields

class Source;
size_t qHash(const Path &, size_t);
class PathIterator;
// Define a iterator for it?
// begin() can basically be itself, end() the empty path (zero length), iteration though dropFront()
class QMLDOM_EXPORT Path{
    Q_GADGET
    Q_DECLARE_TR_FUNCTIONS(ErrorGroup);
public:
    using Kind = PathEls::Kind;
    using Component = PathEls::PathComponent;
    static ErrorGroups myErrors(); // use static consts and central registration instead?

    Path() = default;
    explicit Path(const PathEls::PathComponent &c) : m_endOffset(0), m_length(0)
    {
        *this = withComponent(c);
    }

    int length() const { return m_length; }
    Path operator[](int i) const;
    explicit operator bool() const;

    PathIterator begin() const;
    PathIterator end() const;

    PathRoot headRoot() const;
    PathCurrent headCurrent() const;
    Kind headKind() const;
    QString headName() const;
    bool checkHeadName(QStringView name) const;
    index_type headIndex(index_type defaultValue=-1) const;
    std::function<bool(const DomItem &)> headFilter() const;
    Path head() const;
    Path last() const;
    Source split() const;

    void dump(const Sink &sink) const;
    QString toString() const;
    Path dropFront(int n = 1) const;
    Path dropTail(int n = 1) const;
    Path mid(int offset, int length) const;
    Path mid(int offset) const;
    Path withComponent(const PathEls::PathComponent &c);

    // # Path construction
    static Path fromString(const QString &s, const ErrorHandler &errorHandler = nullptr);
    static Path fromString(QStringView s, const ErrorHandler &errorHandler = nullptr);
    static Path fromRoot(PathRoot r);
    static Path fromRoot(QStringView s=u"");
    static Path fromRoot(const QString &s);
    static Path fromIndex(index_type i);
    static Path fromField(QStringView s=u"");
    static Path fromField(const QString &s);
    static Path fromKey(QStringView s=u"");
    static Path fromKey(const QString &s);
    static Path fromCurrent(PathCurrent c);
    static Path fromCurrent(QStringView s=u"");
    static Path fromCurrent(const QString &s);
    // add
    Path withEmpty() const;
    Path withField(const QString &name) const;
    Path withField(QStringView name) const;
    Path withKey(const QString &name) const;
    Path withKey(QStringView name) const;
    Path withIndex(index_type i) const;
    Path withAny() const;
    Path withFilter(const std::function<bool(const DomItem &)> &, const QString &) const;
    Path withFilter(const std::function<bool(const DomItem &)> &,
                QStringView desc=u"<native code filter>") const;
    Path withCurrent(PathCurrent s) const;
    Path withCurrent(const QString &s) const;
    Path withCurrent(QStringView s=u"") const;
    Path withPath(const Path &toAdd, bool avoidToAddAsBase = false) const;

    Path expandFront() const;
    Path expandBack() const;

    Path &operator++();
    Path operator ++(int);

    // iterator traits
    using difference_type = long;
    using value_type = Path;
    using pointer = const Component*;
    using reference = const Path&;
    using iterator_category = std::forward_iterator_tag;

    static int cmp(const Path &p1, const Path &p2);

private:
    const Component &component(int i) const;
    explicit Path(quint16 endOffset, quint16 length,
                  const std::shared_ptr<PathEls::PathData> &data);
    friend class QQmlJS::Dom::PathEls::TestPaths;
    friend class FieldFilter;
    friend size_t qHash(const Path &, size_t);

    Path noEndOffset() const;

    quint16 m_endOffset = 0;
    quint16 m_length = 0;
    std::shared_ptr<PathEls::PathData> m_data = {};
};

inline bool operator==(const Path &lhs, const Path &rhs)
{
    return lhs.length() == rhs.length() && Path::cmp(lhs, rhs) == 0;
}
inline bool operator!=(const Path &lhs, const Path &rhs)
{
    return lhs.length() != rhs.length() || Path::cmp(lhs, rhs) != 0;
}
inline bool operator<(const Path &lhs, const Path &rhs)
{
    return Path::cmp(lhs, rhs) < 0;
}
inline bool operator>(const Path &lhs, const Path &rhs)
{
    return Path::cmp(lhs, rhs) > 0;
}
inline bool operator<=(const Path &lhs, const Path &rhs)
{
    return Path::cmp(lhs, rhs) <= 0;
}
inline bool operator>=(const Path &lhs, const Path &rhs)
{
    return Path::cmp(lhs, rhs) >= 0;
}

class PathIterator {
public:
    Path currentEl;
    Path operator *() const { return currentEl.head(); }
    PathIterator operator ++() { currentEl = currentEl.dropFront(); return *this; }
    PathIterator operator ++(int) { PathIterator res{currentEl}; currentEl = currentEl.dropFront(); return res; }
    bool operator ==(const PathIterator &o) const { return currentEl == o.currentEl; }
    bool operator !=(const PathIterator &o) const { return currentEl != o.currentEl; }
};

class Source {
public:
    Path pathToSource;
    Path pathFromSource;
};

inline size_t qHash(const Path &path, size_t seed)
{
    const size_t bufSize = 256;
    size_t buf[bufSize];
    size_t *it = &buf[0];
    *it++ = path.length();
    if (path.length()>0) {
        int iPath = path.length();
        size_t maxPath = bufSize / 2 - 1;
        size_t endPath = (size_t(iPath) > maxPath) ? maxPath - iPath : 0;
        while (size_t(iPath) > endPath) {
            Path p = path[--iPath];
            Path::Kind k = p.headKind();
            *it++ = size_t(k);
            *it++ = qHash(p.component(0).stringView(), seed)^size_t(p.headRoot())^size_t(p.headCurrent());
        }
    }

    // TODO: Get rid of the reinterpret_cast.
    // Rather hash the path components in a more structured way.
    return qHash(QByteArray::fromRawData(reinterpret_cast<char *>(&buf[0]), (it - &buf[0])*sizeof(size_t)), seed);
}

inline QDebug operator<<(QDebug debug, const Path &p)
{
    debug << p.toString();
    return debug;
}

} // end namespace Dom
} // end namespace QQmlJS

QT_END_NAMESPACE

#endif // QMLDOM_PATH_H
