// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QMLDOMITEM_H
#define QMLDOMITEM_H

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

#include "qqmldom_global.h"
#include "qqmldom_fwd_p.h"
#include "qqmldom_utils_p.h"
#include "qqmldomconstants_p.h"
#include "qqmldomstringdumper_p.h"
#include "qqmldompath_p.h"
#include "qqmldomerrormessage_p.h"
#include "qqmldomfunctionref_p.h"
#include "qqmldomfilewriter_p.h"
#include "qqmldomlinewriter_p.h"
#include "qqmldomfieldfilter_p.h"

#include <QtCore/QMap>
#include <QtCore/QMultiMap>
#include <QtCore/QSet>
#include <QtCore/QString>
#include <QtCore/QStringView>
#include <QtCore/QDebug>
#include <QtCore/QDateTime>
#include <QtCore/QMutex>
#include <QtCore/QCborValue>
#include <QtCore/QTimeZone>
#include <QtQml/private/qqmljssourcelocation_p.h>
#include <QtQmlCompiler/private/qqmljsscope_p.h>

#include <memory>
#include <typeinfo>
#include <utility>
#include <type_traits>
#include <variant>
#include <optional>
#include <cstddef>

QT_BEGIN_NAMESPACE

QT_DECLARE_EXPORTED_QT_LOGGING_CATEGORY(writeOutLog, QMLDOM_EXPORT);

namespace QQmlJS {
// we didn't have enough 'O's to properly name everything...
namespace Dom {

class Path;

constexpr bool domTypeIsObjWrap(DomType k);
constexpr bool domTypeIsValueWrap(DomType k);
constexpr bool domTypeIsDomElement(DomType);
constexpr bool domTypeIsOwningItem(DomType);
constexpr bool domTypeIsUnattachedOwningItem(DomType);
constexpr bool domTypeIsScriptElement(DomType);
QMLDOM_EXPORT bool domTypeIsExternalItem(DomType k);
QMLDOM_EXPORT bool domTypeIsTopItem(DomType k);
QMLDOM_EXPORT bool domTypeIsContainer(DomType k);
constexpr bool domTypeCanBeInline(DomType k)
{
    switch (k) {
    case DomType::Empty:
    case DomType::Map:
    case DomType::List:
    case DomType::ListP:
    case DomType::ConstantData:
    case DomType::SimpleObjectWrap:
    case DomType::ScriptElementWrap:
    case DomType::Reference:
        return true;
    default:
        return false;
    }
}
QMLDOM_EXPORT bool domTypeIsScope(DomType k);

QMLDOM_EXPORT QMap<DomType,QString> domTypeToStringMap();
QMLDOM_EXPORT QString domTypeToString(DomType k);
QMLDOM_EXPORT QMap<DomKind, QString> domKindToStringMap();
QMLDOM_EXPORT QString domKindToString(DomKind k);

inline bool noFilter(const DomItem &, const PathEls::PathComponent &, const DomItem &)
{
    return true;
}

using DirectVisitor = function_ref<bool(const PathEls::PathComponent &, function_ref<DomItem()>)>;
// using DirectVisitor = function_ref<bool(Path, const DomItem &)>;

namespace {
template<typename T>
struct IsMultiMap : std::false_type
{
};

template<typename Key, typename T>
struct IsMultiMap<QMultiMap<Key, T>> : std::true_type
{
};

template<typename T>
struct IsMap : std::false_type
{
};

template<typename Key, typename T>
struct IsMap<QMap<Key, T>> : std::true_type
{
};

template<typename... Ts>
using void_t = void;

template<typename T, typename = void>
struct IsDomObject : std::false_type
{
};

template<typename T>
struct IsDomObject<T, void_t<decltype(T::kindValue)>> : std::true_type
{
};

template<typename T, typename = void>
struct IsInlineDom : std::false_type
{
};

template<typename T>
struct IsInlineDom<T, void_t<decltype(T::kindValue)>>
    : std::integral_constant<bool, domTypeCanBeInline(T::kindValue)>
{
};

template<typename T>
struct IsInlineDom<T *, void_t<decltype(T::kindValue)>> : std::true_type
{
};

template<typename T>
struct IsInlineDom<std::shared_ptr<T>, void_t<decltype(T::kindValue)>> : std::true_type
{
};

template<typename T>
struct IsSharedPointerToDomObject : std::false_type
{
};

template<typename T>
struct IsSharedPointerToDomObject<std::shared_ptr<T>> : IsDomObject<T>
{
};

template<typename T, typename = void>
struct IsList : std::false_type
{
};

template<typename T>
struct IsList<T, void_t<typename T::value_type>> : std::true_type
{
};

}

template<typename T>
union SubclassStorage {
    int i;
    T lp;

    // TODO: these are extremely nasty. What is this int doing in here?
    T *data() { return reinterpret_cast<T *>(this); }
    const T *data() const { return reinterpret_cast<const T *>(this); }

    SubclassStorage() { }
    SubclassStorage(T &&el) { el.moveTo(data()); }
    SubclassStorage(const T *el) { el->copyTo(data()); }
    SubclassStorage(const SubclassStorage &o) : SubclassStorage(o.data()) { }
    SubclassStorage(const SubclassStorage &&o) : SubclassStorage(o.data()) { }
    SubclassStorage &operator=(const SubclassStorage &o)
    {
        data()->~T();
        o.data()->copyTo(data());
        return *this;
    }
    ~SubclassStorage() { data()->~T(); }
};

class QMLDOM_EXPORT DomBase
{
public:
    using FilterT = function_ref<bool(const DomItem &, const PathEls::PathComponent &, const DomItem &)>;

    virtual ~DomBase() = default;

    DomBase *domBase() { return this; }
    const DomBase *domBase() const { return this; }

    // minimal overload set:
    virtual DomType kind() const = 0;
    virtual DomKind domKind() const;
    virtual Path pathFromOwner(const DomItem &self) const = 0;
    virtual Path canonicalPath(const DomItem &self) const = 0;
    virtual bool
    iterateDirectSubpaths(const DomItem &self,
                          DirectVisitor visitor) const = 0; // iterates the *direct* subpaths, returns
                                                            // false if a quick end was requested

    bool iterateDirectSubpathsConst(const DomItem &self, DirectVisitor)
            const; // iterates the *direct* subpaths, returns false if a quick end was requested

    virtual DomItem containingObject(
            const DomItem &self) const; // the DomItem corresponding to the canonicalSource source
    virtual void dump(const DomItem &, const Sink &sink, int indent, FilterT filter) const;
    virtual quintptr id() const;
    QString typeName() const;

    virtual QList<QString> fields(const DomItem &self) const;
    virtual DomItem field(const DomItem &self, QStringView name) const;

    virtual index_type indexes(const DomItem &self) const;
    virtual DomItem index(const DomItem &self, index_type index) const;

    virtual QSet<QString> const keys(const DomItem &self) const;
    virtual DomItem key(const DomItem &self, const QString &name) const;

    virtual QString canonicalFilePath(const DomItem &self) const;

    virtual void writeOut(const DomItem &self, OutWriter &lw) const;

    virtual QCborValue value() const {
        return QCborValue();
    }
};

inline DomKind kind2domKind(DomType k)
{
    switch (k) {
    case DomType::Empty:
        return DomKind::Empty;
    case DomType::List:
    case DomType::ListP:
        return DomKind::List;
    case DomType::Map:
        return DomKind::Map;
    case DomType::ConstantData:
        return DomKind::Value;
    default:
        return DomKind::Object;
    }
}

class QMLDOM_EXPORT Empty final : public DomBase
{
public:
    constexpr static DomType kindValue = DomType::Empty;
    DomType kind() const override {  return kindValue; }

    Empty *operator->() { return this; }
    const Empty *operator->() const { return this; }
    Empty &operator*() { return *this; }
    const Empty &operator*() const { return *this; }

    Empty();
    quintptr id() const override { return ~quintptr(0); }
    Path pathFromOwner(const DomItem &self) const override;
    Path canonicalPath(const DomItem &self) const override;
    DomItem containingObject(const DomItem &self) const override;
    bool iterateDirectSubpaths(const DomItem &self, DirectVisitor) const override;
    void dump(const DomItem &, const Sink &s, int indent,
              function_ref<bool(const DomItem &, const PathEls::PathComponent &, const DomItem &)> filter)
            const override;
};

class QMLDOM_EXPORT DomElement: public DomBase {
protected:
    DomElement& operator=(const DomElement&) = default;
public:
    DomElement(const Path &pathFromOwner = Path());
    DomElement(const DomElement &o) = default;
    Path pathFromOwner(const DomItem &self) const override;
    Path pathFromOwner() const { return m_pathFromOwner; }
    Path canonicalPath(const DomItem &self) const override;
    DomItem containingObject(const DomItem &self) const override;
    virtual void updatePathFromOwner(const Path &newPath);

private:
    Path m_pathFromOwner;
};

class QMLDOM_EXPORT Map final : public DomElement
{
public:
    constexpr static DomType kindValue = DomType::Map;
    DomType kind() const override {  return kindValue; }

    Map *operator->() { return this; }
    const Map *operator->() const { return this; }
    Map &operator*() { return *this; }
    const Map &operator*() const { return *this; }

    using LookupFunction = std::function<DomItem(const DomItem &, QString)>;
    using Keys = std::function<QSet<QString>(const DomItem &)>;
    Map(const Path &pathFromOwner, const LookupFunction &lookup,
        const Keys &keys, const QString &targetType);
    quintptr id() const override;
    bool iterateDirectSubpaths(const DomItem &self, DirectVisitor) const override;
    QSet<QString> const keys(const DomItem &self) const override;
    DomItem key(const DomItem &self, const QString &name) const override;

    template<typename T>
    static Map fromMultiMapRef(const Path &pathFromOwner, const QMultiMap<QString, T> &mmap);
    template<typename T>
    static Map fromMultiMap(const Path &pathFromOwner, const QMultiMap<QString, T> &mmap);
    template<typename T>
    static Map
    fromMapRef(
            const Path &pathFromOwner, const QMap<QString, T> &mmap,
            const std::function<DomItem(const DomItem &, const PathEls::PathComponent &, const T &)> &elWrapper);

    template<typename T>
    static Map fromFileRegionMap(
            const Path &pathFromOwner, const QMap<FileLocationRegion, T> &map);
    template<typename T>
    static Map fromFileRegionListMap(
            const Path &pathFromOwner, const QMap<FileLocationRegion, QList<T>> &map);

private:
    template<typename MapT>
    static QSet<QString> fileRegionKeysFromMap(const MapT &map);
    LookupFunction m_lookup;
    Keys m_keys;
    QString m_targetType;
};

class QMLDOM_EXPORT List final : public DomElement
{
public:
    constexpr static DomType kindValue = DomType::List;
    DomType kind() const override {  return kindValue; }

    List *operator->() { return this; }
    const List *operator->() const { return this; }
    List &operator*() { return *this; }
    const List &operator*() const { return *this; }

    using LookupFunction = std::function<DomItem(const DomItem &, index_type)>;
    using Length = std::function<index_type(const DomItem &)>;
    using IteratorFunction =
            std::function<bool(const DomItem &, function_ref<bool(index_type, function_ref<DomItem()>)>)>;

    List(const Path &pathFromOwner, const LookupFunction &lookup, const Length &length,
         const IteratorFunction &iterator, const QString &elType);
    quintptr id() const override;
    bool iterateDirectSubpaths(const DomItem &self, DirectVisitor) const override;
    void
    dump(const DomItem &, const Sink &s, int indent,
         function_ref<bool(const DomItem &, const PathEls::PathComponent &, const DomItem &)>) const override;
    index_type indexes(const DomItem &self) const override;
    DomItem index(const DomItem &self, index_type index) const override;

    template<typename T>
    static List
    fromQList(const Path &pathFromOwner, const QList<T> &list,
              const std::function<DomItem(const DomItem &, const PathEls::PathComponent &, const T &)> &elWrapper,
              ListOptions options = ListOptions::Normal);
    template<typename T>
    static List
    fromQListRef(const Path &pathFromOwner, const QList<T> &list,
                 const std::function<DomItem(const DomItem &, const PathEls::PathComponent &, const T &)> &elWrapper,
                 ListOptions options = ListOptions::Normal);
    void writeOut(const DomItem &self, OutWriter &ow, bool compact) const;
    void writeOut(const DomItem &self, OutWriter &ow) const override { writeOut(self, ow, true); }

private:
    LookupFunction m_lookup;
    Length m_length;
    IteratorFunction m_iterator;
    QString m_elType;
};

class QMLDOM_EXPORT ListPBase : public DomElement
{
public:
    constexpr static DomType kindValue = DomType::ListP;
    DomType kind() const override { return kindValue; }

    ListPBase(const Path &pathFromOwner, const QList<const void *> &pList, const QString &elType)
        : DomElement(pathFromOwner), m_pList(pList), m_elType(elType)
    {
    }
    bool iterateDirectSubpaths(const DomItem &self, DirectVisitor v) const override;
    virtual void copyTo(ListPBase *) const { Q_ASSERT(false); };
    virtual void moveTo(ListPBase *) const { Q_ASSERT(false); };
    quintptr id() const override { return quintptr(0); }
    index_type indexes(const DomItem &) const override { return index_type(m_pList.size()); }
    void writeOut(const DomItem &self, OutWriter &ow, bool compact) const;
    void writeOut(const DomItem &self, OutWriter &ow) const override { writeOut(self, ow, true); }

protected:
    QList<const void *> m_pList;
    QString m_elType;
};

template<typename T>
class ListPT final : public ListPBase
{
public:
    constexpr static DomType kindValue = DomType::ListP;

    ListPT(const Path &pathFromOwner, const QList<T *> &pList, const QString &elType = QString(),
           ListOptions options = ListOptions::Normal)
        : ListPBase(pathFromOwner, {},
                    (elType.isEmpty() ? QLatin1String(typeid(T).name()) : elType))
    {
        static_assert(sizeof(ListPBase) == sizeof(ListPT),
                      "ListPT does not have the same size as ListPBase");
        static_assert(alignof(ListPBase) == alignof(ListPT),
                      "ListPT does not have the same size as ListPBase");
        m_pList.reserve(pList.size());
        if (options == ListOptions::Normal) {
            for (const void *p : pList)
                m_pList.append(p);
        } else if (options == ListOptions::Reverse) {
            for (qsizetype i = pList.size(); i-- != 0;)
                // probably writing in reverse and reading sequentially would be better
                m_pList.append(pList.at(i));
        } else {
            Q_ASSERT(false);
        }
    }
    void copyTo(ListPBase *t) const override { new (t) ListPT(*this); }
    void moveTo(ListPBase *t) const override { new (t) ListPT(std::move(*this)); }
    bool iterateDirectSubpaths(const DomItem &self, DirectVisitor v) const override;

    DomItem index(const DomItem &self, index_type index) const override;
};

class QMLDOM_EXPORT ListP
{
public:
    constexpr static DomType kindValue = DomType::ListP;
    template<typename T>
    ListP(const Path &pathFromOwner, const QList<T *> &pList, const QString &elType = QString(),
          ListOptions options = ListOptions::Normal)
        : list(ListPT<T>(pathFromOwner, pList, elType, options))
    {
    }
    ListP() = delete;

    ListPBase *operator->() { return list.data(); }
    const ListPBase *operator->() const { return list.data(); }
    ListPBase &operator*() { return *list.data(); }
    const ListPBase &operator*() const { return *list.data(); }

private:
    SubclassStorage<ListPBase> list;
};

class QMLDOM_EXPORT ConstantData final : public DomElement
{
public:
    constexpr static DomType kindValue = DomType::ConstantData;
    DomType kind() const override { return kindValue; }

    enum class Options {
        MapIsMap,
        FirstMapIsFields
    };

    ConstantData *operator->() { return this; }
    const ConstantData *operator->() const { return this; }
    ConstantData &operator*() { return *this; }
    const ConstantData &operator*() const { return *this; }

    ConstantData(const Path &pathFromOwner, const QCborValue &value,
                 Options options = Options::MapIsMap);
    bool iterateDirectSubpaths(const DomItem &self, DirectVisitor) const override;
    quintptr id() const override;
    DomKind domKind() const override;
    QCborValue value() const override { return m_value; }
    Options options() const { return m_options; }
private:
    QCborValue m_value;
    Options m_options;
};

class QMLDOM_EXPORT SimpleObjectWrapBase : public DomElement
{
public:
    constexpr static DomType kindValue = DomType::SimpleObjectWrap;
    DomType kind() const final override { return m_kind; }

    quintptr id() const final override { return m_id; }
    DomKind domKind() const final override { return m_domKind; }

    template <typename T>
    T const *as() const
    {
        if (m_options & SimpleWrapOption::ValueType) {
            if (m_value.metaType() == QMetaType::fromType<T>())
                return static_cast<const T *>(m_value.constData());
            return nullptr;
        } else {
            return m_value.value<const T *>();
        }
    }

    SimpleObjectWrapBase() = delete;
    virtual void copyTo(SimpleObjectWrapBase *) const { Q_ASSERT(false); }
    virtual void moveTo(SimpleObjectWrapBase *) const { Q_ASSERT(false); }
    bool iterateDirectSubpaths(const DomItem &, DirectVisitor) const override
    {
        Q_ASSERT(false);
        return true;
    }

protected:
    friend class TestDomItem;
    SimpleObjectWrapBase(const Path &pathFromOwner, const QVariant &value, quintptr idValue,
                         DomType kind = kindValue,
                         SimpleWrapOptions options = SimpleWrapOption::None)
        : DomElement(pathFromOwner),
          m_kind(kind),
          m_domKind(kind2domKind(kind)),
          m_value(value),
          m_id(idValue),
          m_options(options)
    {
    }

    DomType m_kind;
    DomKind m_domKind;
    QVariant m_value;
    quintptr m_id;
    SimpleWrapOptions m_options;
};

template<typename T>
class SimpleObjectWrapT final : public SimpleObjectWrapBase
{
public:
    constexpr static DomType kindValue = DomType::SimpleObjectWrap;

    bool iterateDirectSubpaths(const DomItem &self, DirectVisitor visitor) const override
    {
        return asT()->iterateDirectSubpaths(self, visitor);
    }

    void writeOut(const DomItem &self, OutWriter &lw) const override;

    const T *asT() const
    {
        if constexpr (domTypeIsValueWrap(T::kindValue)) {
            if (m_value.metaType() == QMetaType::fromType<T>())
                return static_cast<const T *>(m_value.constData());
            return nullptr;
        } else if constexpr (domTypeIsObjWrap(T::kindValue)) {
            return m_value.value<const T *>();
        } else {
            // need dependent static assert to not unconditially trigger
            static_assert(!std::is_same_v<T, T>, "wrapping of unexpected type");
            return nullptr; // necessary to avoid warnings on INTEGRITY
        }
    }

    void copyTo(SimpleObjectWrapBase *target) const override
    {
        static_assert(sizeof(SimpleObjectWrapBase) == sizeof(SimpleObjectWrapT),
                      "Size mismatch in SimpleObjectWrapT");
        static_assert(alignof(SimpleObjectWrapBase) == alignof(SimpleObjectWrapT),
                      "Size mismatch in SimpleObjectWrapT");
        new (target) SimpleObjectWrapT(*this);
    }

    void moveTo(SimpleObjectWrapBase *target) const override
    {
        static_assert(sizeof(SimpleObjectWrapBase) == sizeof(SimpleObjectWrapT),
                      "Size mismatch in SimpleObjectWrapT");
        static_assert(alignof(SimpleObjectWrapBase) == alignof(SimpleObjectWrapT),
                      "Size mismatch in SimpleObjectWrapT");
        new (target) SimpleObjectWrapT(std::move(*this));
    }

    SimpleObjectWrapT(const Path &pathFromOwner, const QVariant &v,
                      quintptr idValue, SimpleWrapOptions o)
        : SimpleObjectWrapBase(pathFromOwner, v, idValue, T::kindValue, o)
    {
        Q_ASSERT(domTypeIsValueWrap(T::kindValue) == bool(o & SimpleWrapOption::ValueType));
    }
};

class QMLDOM_EXPORT SimpleObjectWrap
{
public:
    constexpr static DomType kindValue = DomType::SimpleObjectWrap;

    SimpleObjectWrapBase *operator->() { return wrap.data(); }
    const SimpleObjectWrapBase *operator->() const { return wrap.data(); }
    SimpleObjectWrapBase &operator*() { return *wrap.data(); }
    const SimpleObjectWrapBase &operator*() const { return *wrap.data(); }

    template<typename T>
    static SimpleObjectWrap fromObjectRef(const Path &pathFromOwner, T &value)
    {
        return SimpleObjectWrap(pathFromOwner, value);
    }
    SimpleObjectWrap() = delete;

private:
    template<typename T>
    SimpleObjectWrap(const Path &pathFromOwner, T &value)
    {
        using BaseT = std::decay_t<T>;
        if constexpr (domTypeIsObjWrap(BaseT::kindValue)) {
            new (wrap.data()) SimpleObjectWrapT<BaseT>(pathFromOwner, QVariant::fromValue(&value),
                                                       quintptr(&value), SimpleWrapOption::None);
        } else if constexpr (domTypeIsValueWrap(BaseT::kindValue)) {
            new (wrap.data()) SimpleObjectWrapT<BaseT>(pathFromOwner, QVariant::fromValue(value),
                                                       quintptr(0), SimpleWrapOption::ValueType);
        } else {
            qCWarning(domLog) << "Unexpected object to wrap in SimpleObjectWrap: "
                              << domTypeToString(BaseT::kindValue);
            Q_ASSERT_X(false, "SimpleObjectWrap",
                       "simple wrap of unexpected object"); // allow? (mocks for testing,...)
            new (wrap.data())
                    SimpleObjectWrapT<BaseT>(pathFromOwner, nullptr, 0, SimpleWrapOption::None);
        }
    }
    SubclassStorage<SimpleObjectWrapBase> wrap;
};

class QMLDOM_EXPORT Reference final : public DomElement
{
    Q_GADGET
public:
    constexpr static DomType kindValue = DomType::Reference;
    DomType kind() const override {  return kindValue; }

    Reference *operator->() { return this; }
    const Reference *operator->() const { return this; }
    Reference &operator*() { return *this; }
    const Reference &operator*() const { return *this; }

    bool shouldCache() const;
    Reference(const Path &referredObject = Path(), const Path &pathFromOwner = Path(),
              const SourceLocation &loc = SourceLocation());
    quintptr id() const override;
    bool iterateDirectSubpaths(const DomItem &self, DirectVisitor) const override;
    DomItem field(const DomItem &self, QStringView name) const override;
    QList<QString> fields(const DomItem &self) const override;
    index_type indexes(const DomItem &) const override { return 0; }
    DomItem index(const DomItem &, index_type) const override;
    QSet<QString> const keys(const DomItem &) const override { return {}; }
    DomItem key(const DomItem &, const QString &) const override;

    DomItem get(const DomItem &self, const ErrorHandler &h = nullptr,
                QList<Path> *visitedRefs = nullptr) const;
    QList<DomItem> getAll(const DomItem &self, const ErrorHandler &h = nullptr,
                          QList<Path> *visitedRefs = nullptr) const;

    Path referredObjectPath;
};

namespace FileLocations {
struct Info;
}

/*!
    \internal
    \brief A common base class for all the script elements.

    This marker class allows to use all the script elements as a ScriptElement*, using virtual
   dispatch. For now, it does not add any extra functionality, compared to a DomElement, but allows
   to forbid DomElement* at the places where only script elements are required.
 */
// TODO: do we need another marker struct like this one to differentiate expressions from
// statements? This would allow to avoid mismatchs between script expressions and script statements,
// using type-safety.
struct ScriptElement : public DomElement
{
    template<typename T>
    using PointerType = std::shared_ptr<T>;

    using DomElement::DomElement;
    virtual void
    createFileLocations(const std::shared_ptr<FileLocations::Node> &fileLocationOfOwner) = 0;

    QQmlJSScope::ConstPtr semanticScope();
    void setSemanticScope(const QQmlJSScope::ConstPtr &scope);

private:
    QQmlJSScope::ConstPtr m_scope;
};

/*!
   \internal
   \brief Use this to contain any script element.
 */
class ScriptElementVariant
{
private:
    template<typename... T>
    using VariantOfPointer = std::variant<ScriptElement::PointerType<T>...>;

    template<typename T, typename Variant>
    struct TypeIsInVariant;

    template<typename T, typename... Ts>
    struct TypeIsInVariant<T, std::variant<Ts...>> : public std::disjunction<std::is_same<T, Ts>...>
    {
    };

public:
    using ScriptElementT =
            VariantOfPointer<ScriptElements::BlockStatement, ScriptElements::IdentifierExpression,
                             ScriptElements::ForStatement, ScriptElements::BinaryExpression,
                             ScriptElements::VariableDeclarationEntry, ScriptElements::Literal,
                             ScriptElements::IfStatement, ScriptElements::GenericScriptElement,
                             ScriptElements::VariableDeclaration, ScriptElements::ReturnStatement>;

    template<typename T>
    static ScriptElementVariant fromElement(const T &element)
    {
        static_assert(TypeIsInVariant<T, ScriptElementT>::value,
                      "Cannot construct ScriptElementVariant from T, as it is missing from the "
                      "ScriptElementT.");
        ScriptElementVariant p;
        p.m_data = element;
        return p;
    }

    ScriptElement::PointerType<ScriptElement> base() const;

    operator bool() const { return m_data.has_value(); }

    template<typename F>
    void visitConst(F &&visitor) const
    {
        if (m_data)
            std::visit(std::forward<F>(visitor), *m_data);
    }

    template<typename F>
    void visit(F &&visitor)
    {
        if (m_data)
            std::visit(std::forward<F>(visitor), *m_data);
    }
    std::optional<ScriptElementT> data() { return m_data; }
    void setData(const ScriptElementT &data) { m_data = data; }

private:
    std::optional<ScriptElementT> m_data;
};

/*!
    \internal

    To avoid cluttering the already unwieldy \l ElementT type below with all the types that the
   different script elements can have, wrap them in an extra class. It will behave like an internal
   Dom structure (e.g. like a List or a Map) and contain a pointer the the script element.
 */
class ScriptElementDomWrapper
{
public:
    ScriptElementDomWrapper(const ScriptElementVariant &element) : m_element(element) { }

    static constexpr DomType kindValue = DomType::ScriptElementWrap;

    DomBase *operator->() { return m_element.base().get(); }
    const DomBase *operator->() const { return m_element.base().get(); }
    DomBase &operator*() { return *m_element.base(); }
    const DomBase &operator*() const { return *m_element.base(); }

    ScriptElementVariant element() const { return m_element; }

private:
    ScriptElementVariant m_element;
};

// TODO: create more "groups" to simplify this variant? Maybe into Internal, ScriptExpression, ???
using ElementT =
        std::variant<ConstantData, Empty, List, ListP, Map, Reference, ScriptElementDomWrapper,
                     SimpleObjectWrap, const AstComments *, const FileLocations::Node *,
                     const DomEnvironment *, const DomUniverse *, const EnumDecl *,
                     const ExternalItemInfoBase *, const ExternalItemPairBase *,
                     const GlobalComponent *, const GlobalScope *, const JsFile *,
                     const JsResource *, const LoadInfo *, const MockObject *, const MockOwner *,
                     const ModuleIndex *, const ModuleScope *, const QmlComponent *,
                     const QmlDirectory *, const QmlFile *, const QmlObject *, const QmldirFile *,
                     const QmltypesComponent *, const QmltypesFile *, const ScriptExpression *>;

using TopT = std::variant<
        std::monostate,
        std::shared_ptr<DomEnvironment>,
        std::shared_ptr<DomUniverse>>;

using OwnerT =
        std::variant<std::monostate, std::shared_ptr<ModuleIndex>, std::shared_ptr<MockOwner>,
                     std::shared_ptr<ExternalItemInfoBase>, std::shared_ptr<ExternalItemPairBase>,
                     std::shared_ptr<QmlDirectory>, std::shared_ptr<QmldirFile>,
                     std::shared_ptr<JsFile>, std::shared_ptr<QmlFile>,
                     std::shared_ptr<QmltypesFile>, std::shared_ptr<GlobalScope>,
                     std::shared_ptr<ScriptExpression>, std::shared_ptr<AstComments>,
                     std::shared_ptr<LoadInfo>, std::shared_ptr<FileLocations::Node>,
                     std::shared_ptr<DomEnvironment>, std::shared_ptr<DomUniverse>>;

inline bool emptyChildrenVisitor(Path, const DomItem &, bool)
{
    return true;
}

class MutableDomItem;

class FileToLoad
{
public:
    struct InMemoryContents
    {
        QString data;
        QDateTime date = QDateTime::currentDateTimeUtc();
    };

    FileToLoad(const std::weak_ptr<DomEnvironment> &environment, const QString &canonicalPath,
               const QString &logicalPath, const std::optional<InMemoryContents> &content);
    FileToLoad() = default;

    static FileToLoad fromMemory(const std::weak_ptr<DomEnvironment> &environment,
                                 const QString &path, const QString &data);
    static FileToLoad fromFileSystem(const std::weak_ptr<DomEnvironment> &environment,
                                     const QString &canonicalPath);

    std::weak_ptr<DomEnvironment> environment() const { return m_environment; }
    QString canonicalPath() const { return m_canonicalPath; }
    QString logicalPath() const { return m_logicalPath; }
    void setCanonicalPath(const QString &canonicalPath) { m_canonicalPath = canonicalPath; }
    void setLogicalPath(const QString &logicalPath) { m_logicalPath = logicalPath; }
    std::optional<InMemoryContents> content() const { return m_content; }

private:
    std::weak_ptr<DomEnvironment> m_environment;
    QString m_canonicalPath;
    QString m_logicalPath;
    std::optional<InMemoryContents> m_content;
};

class QMLDOM_EXPORT DomItem {
    Q_DECLARE_TR_FUNCTIONS(DomItem);
public:
    using Callback = function<void(const Path &, const DomItem &, const DomItem &)>;

    using InternalKind = DomType;
    using Visitor = function_ref<bool(const Path &, const DomItem &)>;
    using ChildrenVisitor = function_ref<bool(const Path &, const DomItem &, bool)>;

    static ErrorGroup domErrorGroup;
    static ErrorGroups myErrors();
    static ErrorGroups myResolveErrors();
    static DomItem empty;

    enum class CopyOption { EnvConnected, EnvDisconnected };

    template<typename F>
    auto visitEl(F f) const
    {
        return std::visit(f, this->m_element);
    }

    explicit operator bool() const { return m_kind != DomType::Empty; }
    InternalKind internalKind() const {
        return m_kind;
    }
    QString internalKindStr() const { return domTypeToString(internalKind()); }
    DomKind domKind() const
    {
        if (m_kind == DomType::ConstantData)
            return std::get<ConstantData>(m_element).domKind();
        else
            return kind2domKind(m_kind);
    }

    Path canonicalPath() const;

    DomItem filterUp(function_ref<bool(DomType k, const DomItem &)> filter, FilterUpOptions options) const;
    DomItem containingObject() const;
    DomItem container() const;
    DomItem owner() const;
    DomItem top() const;
    DomItem environment() const;
    DomItem universe() const;
    DomItem containingFile() const;
    DomItem containingScriptExpression() const;
    DomItem goToFile(const QString &filePath) const;
    DomItem goUp(int) const;
    DomItem directParent() const;

    DomItem qmlObject(GoTo option = GoTo::Strict,
                      FilterUpOptions options = FilterUpOptions::ReturnOuter) const;
    DomItem fileObject(GoTo option = GoTo::Strict) const;
    DomItem rootQmlObject(GoTo option = GoTo::Strict) const;
    DomItem globalScope() const;
    DomItem component(GoTo option = GoTo::Strict) const;
    DomItem scope(FilterUpOptions options = FilterUpOptions::ReturnOuter) const;
    QQmlJSScope::ConstPtr nearestSemanticScope() const;
    QQmlJSScope::ConstPtr semanticScope() const;

    // convenience getters
    DomItem get(const ErrorHandler &h = nullptr, QList<Path> *visitedRefs = nullptr) const;
    QList<DomItem> getAll(const ErrorHandler &h = nullptr, QList<Path> *visitedRefs = nullptr) const;
    bool isOwningItem() const { return domTypeIsOwningItem(internalKind()); }
    bool isExternalItem() const { return domTypeIsExternalItem(internalKind()); }
    bool isTopItem() const { return domTypeIsTopItem(internalKind()); }
    bool isContainer() const { return domTypeIsContainer(internalKind()); }
    bool isScope() const { return domTypeIsScope(internalKind()); }
    bool isCanonicalChild(const DomItem &child) const;
    bool hasAnnotations() const;
    QString name() const { return field(Fields::name).value().toString(); }
    DomItem pragmas() const { return field(Fields::pragmas); }
    DomItem ids() const { return field(Fields::ids); }
    QString idStr() const { return field(Fields::idStr).value().toString(); }
    DomItem propertyInfos() const { return field(Fields::propertyInfos); }
    PropertyInfo propertyInfoWithName(const QString &name) const;
    QSet<QString> propertyInfoNames() const;
    DomItem propertyDefs() const { return field(Fields::propertyDefs); }
    DomItem bindings() const { return field(Fields::bindings); }
    DomItem methods() const { return field(Fields::methods); }
    DomItem enumerations() const { return field(Fields::enumerations); }
    DomItem children() const { return field(Fields::children); }
    DomItem child(index_type i) const { return field(Fields::children).index(i); }
    DomItem annotations() const
    {
        if (hasAnnotations())
            return field(Fields::annotations);
        else
            return DomItem();
    }

    bool resolve(const Path &path, Visitor visitor, const ErrorHandler &errorHandler,
                 ResolveOptions options = ResolveOption::None, const Path &fullPath = Path(),
                 QList<Path> *visitedRefs = nullptr) const;

    DomItem operator[](const Path &path) const;
    DomItem operator[](QStringView component) const;
    DomItem operator[](const QString &component) const;
    DomItem operator[](const char16_t *component) const
    {
        return (*this)[QStringView(component)];
    } // to avoid clash with stupid builtin ptrdiff_t[DomItem&], coming from C
    DomItem operator[](index_type i) const { return index(i); }
    DomItem operator[](int i) const { return index(i); }
    index_type size() const { return indexes() + keys().size(); }
    index_type length() const { return size(); }

    DomItem path(const Path &p, const ErrorHandler &h = &defaultErrorHandler) const;
    DomItem path(const QString &p, const ErrorHandler &h = &defaultErrorHandler) const;
    DomItem path(QStringView p, const ErrorHandler &h = &defaultErrorHandler) const;

    QList<QString> fields() const;
    DomItem field(QStringView name) const;

    index_type indexes() const;
    DomItem index(index_type) const;
    bool visitIndexes(function_ref<bool(const DomItem &)> visitor) const;

    QSet<QString> keys() const;
    QStringList sortedKeys() const;
    DomItem key(const QString &name) const;
    DomItem key(QStringView name) const { return key(name.toString()); }
    bool visitKeys(function_ref<bool(const QString &, const DomItem &)> visitor) const;

    QList<DomItem> values() const;
    void writeOutPre(OutWriter &lw) const;
    void writeOut(OutWriter &lw) const;
    void writeOutPost(OutWriter &lw) const;
    bool writeOutForFile(OutWriter &ow, WriteOutChecks extraChecks) const;
    bool writeOut(const QString &path, int nBackups = 2,
                  const LineWriterOptions &opt = LineWriterOptions(), FileWriter *fw = nullptr,
                  WriteOutChecks extraChecks = WriteOutCheck::Default) const;

    bool visitTree(const Path &basePath, ChildrenVisitor visitor,
                   VisitOptions options = VisitOption::Default,
                   ChildrenVisitor openingVisitor = emptyChildrenVisitor,
                   ChildrenVisitor closingVisitor = emptyChildrenVisitor,
                   const FieldFilter &filter = FieldFilter::noFilter()) const;
    bool visitPrototypeChain(function_ref<bool(const DomItem &)> visitor,
                             VisitPrototypesOptions options = VisitPrototypesOption::Normal,
                             const ErrorHandler &h = nullptr, QSet<quintptr> *visited = nullptr,
                             QList<Path> *visitedRefs = nullptr) const;
    bool visitDirectAccessibleScopes(function_ref<bool(const DomItem &)> visitor,
                                     VisitPrototypesOptions options = VisitPrototypesOption::Normal,
                                     const ErrorHandler &h = nullptr, QSet<quintptr> *visited = nullptr,
                                     QList<Path> *visitedRefs = nullptr) const;
    bool
    visitStaticTypePrototypeChains(function_ref<bool(const DomItem &)> visitor,
                                   VisitPrototypesOptions options = VisitPrototypesOption::Normal,
                                   const ErrorHandler &h = nullptr, QSet<quintptr> *visited = nullptr,
                                   QList<Path> *visitedRefs = nullptr) const;

    bool visitUp(function_ref<bool(const DomItem &)> visitor) const;
    bool visitScopeChain(
            function_ref<bool(const DomItem &)> visitor, LookupOptions = LookupOption::Normal,
            const ErrorHandler &h = nullptr, QSet<quintptr> *visited = nullptr,
            QList<Path> *visitedRefs = nullptr) const;
    bool visitLocalSymbolsNamed(
            const QString &name, function_ref<bool(const DomItem &)> visitor) const;
    bool visitLookup1(
            const QString &symbolName, function_ref<bool(const DomItem &)> visitor,
            LookupOptions = LookupOption::Normal, const ErrorHandler &h = nullptr,
            QSet<quintptr> *visited = nullptr, QList<Path> *visitedRefs = nullptr) const;
    bool visitLookup(
            const QString &symbolName, function_ref<bool(const DomItem &)> visitor,
            LookupType type = LookupType::Symbol, LookupOptions = LookupOption::Normal,
            const ErrorHandler &errorHandler = nullptr, QSet<quintptr> *visited = nullptr,
            QList<Path> *visitedRefs = nullptr) const;
    bool visitSubSymbolsNamed(
            const QString &name, function_ref<bool(const DomItem &)> visitor) const;
    DomItem proceedToScope(
            const ErrorHandler &h = nullptr, QList<Path> *visitedRefs = nullptr) const;
    QList<DomItem> lookup(
            const QString &symbolName, LookupType type = LookupType::Symbol,
            LookupOptions = LookupOption::Normal, const ErrorHandler &errorHandler = nullptr) const;
    DomItem lookupFirst(
            const QString &symbolName, LookupType type = LookupType::Symbol,
            LookupOptions = LookupOption::Normal, const ErrorHandler &errorHandler = nullptr) const;

    quintptr id() const;
    Path pathFromOwner() const;
    QString canonicalFilePath() const;
    MutableDomItem makeCopy(CopyOption option = CopyOption::EnvConnected) const;
    bool commitToBase(const std::shared_ptr<DomEnvironment> &validPtr = nullptr) const;
    DomItem refreshed() const { return top().path(canonicalPath()); }
    QCborValue value() const;

    void dumpPtr(const Sink &sink) const;
    void dump(const Sink &, int indent = 0,
              function_ref<bool(const DomItem &, const PathEls::PathComponent &, const DomItem &)> filter =
                      noFilter) const;
    FileWriter::Status
    dump(const QString &path,
         function_ref<bool(const DomItem &, const PathEls::PathComponent &, const DomItem &)> filter = noFilter,
         int nBackups = 2, int indent = 0, FileWriter *fw = nullptr) const;
    QString toString() const;

    // OwnigItem elements
    int derivedFrom() const;
    int revision() const;
    QDateTime createdAt() const;
    QDateTime frozenAt() const;
    QDateTime lastDataUpdateAt() const;

    void addError(ErrorMessage &&msg) const;
    ErrorHandler errorHandler() const;
    void clearErrors(const ErrorGroups &groups = ErrorGroups({}), bool iterate = true) const;
    // return false if a quick exit was requested
    bool iterateErrors(
            function_ref<bool (const DomItem &, const ErrorMessage &)> visitor, bool iterate,
            Path inPath = Path()) const;

    bool iterateSubOwners(function_ref<bool(const DomItem &owner)> visitor) const;
    bool iterateDirectSubpaths(DirectVisitor v) const;

    template<typename T>
    DomItem subDataItem(const PathEls::PathComponent &c, const T &value,
                        ConstantData::Options options = ConstantData::Options::MapIsMap) const;
    template<typename T>
    DomItem subDataItemField(QStringView f, const T &value,
                             ConstantData::Options options = ConstantData::Options::MapIsMap) const
    {
        return subDataItem(PathEls::Field(f), value, options);
    }
    template<typename T>
    DomItem subValueItem(const PathEls::PathComponent &c, const T &value,
                         ConstantData::Options options = ConstantData::Options::MapIsMap) const;
    template<typename T>
    bool dvValue(DirectVisitor visitor, const PathEls::PathComponent &c, const T &value,
                 ConstantData::Options options = ConstantData::Options::MapIsMap) const;
    template<typename T>
    bool dvValueField(DirectVisitor visitor, QStringView f, const T &value,
                      ConstantData::Options options = ConstantData::Options::MapIsMap) const
    {
        return this->dvValue<T>(std::move(visitor), PathEls::Field(f), value, options);
    }
    template<typename F>
    bool dvValueLazy(DirectVisitor visitor, const PathEls::PathComponent &c, F valueF,
                     ConstantData::Options options = ConstantData::Options::MapIsMap) const;
    template<typename F>
    bool dvValueLazyField(DirectVisitor visitor, QStringView f, F valueF,
                          ConstantData::Options options = ConstantData::Options::MapIsMap) const
    {
        return this->dvValueLazy(std::move(visitor), PathEls::Field(f), valueF, options);
    }
    DomItem subLocationItem(const PathEls::PathComponent &c, SourceLocation loc) const
    {
        return this->subDataItem(c, sourceLocationToQCborValue(loc));
    }
    // bool dvSubReference(DirectVisitor visitor, const PathEls::PathComponent &c, Path
    // referencedObject);
    DomItem subReferencesItem(const PathEls::PathComponent &c, const QList<Path> &paths) const;
    DomItem subReferenceItem(const PathEls::PathComponent &c, const Path &referencedObject) const;
    bool dvReference(DirectVisitor visitor, const PathEls::PathComponent &c, const Path &referencedObject) const
    {
        return dvItem(std::move(visitor), c, [c, this, referencedObject]() {
            return this->subReferenceItem(c, referencedObject);
        });
    }
    bool dvReferences(
            DirectVisitor visitor, const PathEls::PathComponent &c, const QList<Path> &paths) const
    {
        return dvItem(std::move(visitor), c, [c, this, paths]() {
            return this->subReferencesItem(c, paths);
        });
    }
    bool dvReferenceField(DirectVisitor visitor, QStringView f, const Path &referencedObject) const
    {
        return dvReference(std::move(visitor), PathEls::Field(f), referencedObject);
    }
    bool dvReferencesField(DirectVisitor visitor, QStringView f, const QList<Path> &paths) const
    {
        return dvReferences(std::move(visitor), PathEls::Field(f), paths);
    }
    bool dvItem(DirectVisitor visitor, const PathEls::PathComponent &c, function_ref<DomItem()> it) const
    {
        return visitor(c, it);
    }
    bool dvItemField(DirectVisitor visitor, QStringView f, function_ref<DomItem()> it) const
    {
        return dvItem(std::move(visitor), PathEls::Field(f), it);
    }
    DomItem subListItem(const List &list) const;
    DomItem subMapItem(const Map &map) const;
    DomItem subObjectWrapItem(SimpleObjectWrap obj) const
    {
        return DomItem(m_top, m_owner, m_ownerPath, obj);
    }

    DomItem subScriptElementWrapperItem(const ScriptElementVariant &obj) const
    {
        Q_ASSERT(obj);
        return DomItem(m_top, m_owner, m_ownerPath, ScriptElementDomWrapper(obj));
    }

    template<typename Owner>
    DomItem subOwnerItem(const PathEls::PathComponent &c, Owner o) const
    {
        if constexpr (domTypeIsUnattachedOwningItem(Owner::element_type::kindValue))
            return DomItem(m_top, o, canonicalPath().withComponent(c), o.get());
        else
            return DomItem(m_top, o, Path(), o.get());
    }
    template<typename T>
    DomItem wrap(const PathEls::PathComponent &c, const T &obj) const;
    template<typename T>
    DomItem wrapField(QStringView f, const T &obj) const
    {
        return wrap<T>(PathEls::Field(f), obj);
    }
    template<typename T>
    bool dvWrap(DirectVisitor visitor, const PathEls::PathComponent &c, T &obj) const;
    template<typename T>
    bool dvWrapField(DirectVisitor visitor, QStringView f, T &obj) const
    {
        return dvWrap<T>(std::move(visitor), PathEls::Field(f), obj);
    }

    DomItem() = default;
    DomItem(const std::shared_ptr<DomEnvironment> &);
    DomItem(const std::shared_ptr<DomUniverse> &);

    // TODO move to DomEnvironment?
    static DomItem fromCode(const QString &code, DomType fileType = DomType::QmlFile);

    // --- start of potentially dangerous stuff, make private? ---

    std::shared_ptr<DomTop> topPtr() const;
    std::shared_ptr<OwningItem> owningItemPtr() const;

    // keep the DomItem around to ensure that it doesn't get deleted
    template<typename T, typename std::enable_if<std::is_base_of_v<DomBase, T>, bool>::type = true>
    T const *as() const
    {
        if (m_kind == T::kindValue) {
            if constexpr (domTypeIsObjWrap(T::kindValue) || domTypeIsValueWrap(T::kindValue))
                return std::get<SimpleObjectWrap>(m_element)->as<T>();
            else
                return static_cast<T const *>(base());
        }
        return nullptr;
    }

    template<typename T, typename std::enable_if<!std::is_base_of_v<DomBase, T>, bool>::type = true>
    T const *as() const
    {
        if (m_kind == T::kindValue) {
            Q_ASSERT(domTypeIsObjWrap(m_kind) || domTypeIsValueWrap(m_kind));
            return std::get<SimpleObjectWrap>(m_element)->as<T>();
        }
        return nullptr;
    }

    template<typename T>
    std::shared_ptr<T> ownerAs() const;

    template<typename Owner, typename T>
    DomItem copy(const Owner &owner, const Path &ownerPath, const T &base) const
    {
        Q_ASSERT(!std::holds_alternative<std::monostate>(m_top));
        static_assert(IsInlineDom<std::decay_t<T>>::value, "Expected an inline item or pointer");
        return DomItem(m_top, owner, ownerPath, base);
    }

    template<typename Owner>
    DomItem copy(const Owner &owner, const Path &ownerPath) const
    {
        Q_ASSERT(!std::holds_alternative<std::monostate>(m_top));
        return DomItem(m_top, owner, ownerPath, owner.get());
    }

    template<typename T>
    DomItem copy(const T &base) const
    {
        Q_ASSERT(!std::holds_alternative<std::monostate>(m_top));
        using BaseT = std::decay_t<T>;
        static_assert(!std::is_same_v<BaseT, ElementT>,
                      "variant not supported, pass in the stored types");
        static_assert(IsInlineDom<BaseT>::value || std::is_same_v<BaseT, std::monostate>,
                      "expected either a pointer or an inline item");

        if constexpr (IsSharedPointerToDomObject<BaseT>::value)
            return DomItem(m_top, base, Path(), base.get());
        else if constexpr (IsInlineDom<BaseT>::value)
            return DomItem(m_top, m_owner, m_ownerPath, base);

        Q_UNREACHABLE_RETURN(DomItem(m_top, m_owner, m_ownerPath, nullptr));
    }

private:
    enum class WriteOutCheckResult { Success, Failed };
    WriteOutCheckResult performWriteOutChecks(const DomItem &, OutWriter &, WriteOutChecks) const;
    const DomBase *base() const;

    template<typename Env, typename Owner>
    DomItem(Env, Owner, Path, std::nullptr_t) : DomItem()
    {
    }

    template<typename Env, typename Owner, typename T,
             typename = std::enable_if_t<IsInlineDom<std::decay_t<T>>::value>>
    DomItem(Env env, Owner owner, const Path &ownerPath, const T &el)
        : m_top(env), m_owner(owner), m_ownerPath(ownerPath), m_element(el)
    {
        using BaseT = std::decay_t<T>;
        if constexpr (std::is_pointer_v<BaseT>) {
            if (!el || el->kind() == DomType::Empty) { // avoid null ptr, and allow only a
                // single kind of Empty
                m_kind = DomType::Empty;
                m_top = std::monostate();
                m_owner = std::monostate();
                m_ownerPath = Path();
                m_element = Empty();
            } else {
                using DomT = std::remove_pointer_t<BaseT>;
                m_element = el;
                m_kind = DomT::kindValue;
            }
        } else {
            static_assert(!std::is_same_v<BaseT, ElementT>,
                          "variant not supported, pass in the internal type");
            m_kind = el->kind();
        }
    }
    friend class DomBase;
    friend class DomElement;
    friend class Map;
    friend class List;
    friend class QmlObject;
    friend class DomUniverse;
    friend class DomEnvironment;
    friend class ExternalItemInfoBase;
    friend class ConstantData;
    friend class MutableDomItem;
    friend class ScriptExpression;
    friend class AstComments;
    friend class FileLocations::Node;
    friend class TestDomItem;
    friend QMLDOM_EXPORT bool operator==(const DomItem &, const DomItem &);
    DomType m_kind = DomType::Empty;
    TopT m_top;
    OwnerT m_owner;
    Path m_ownerPath;
    ElementT m_element = Empty();
};

QMLDOM_EXPORT bool operator==(const DomItem &o1, const DomItem &o2);

inline bool operator!=(const DomItem &o1, const DomItem &o2)
{
    return !(o1 == o2);
}

template<typename T>
static DomItem keyMultiMapHelper(const DomItem &self, const QString &key,
                                 const QMultiMap<QString, T> &mmap)
{
    auto it = mmap.find(key);
    auto end = mmap.cend();
    if (it == end)
        return DomItem();
    else {
        // special case single element (++it == end || it.key() != key)?
        QList<const T *> values;
        while (it != end && it.key() == key)
            values.append(&(*it++));
        ListP ll(self.pathFromOwner().withComponent(PathEls::Key(key)), values, QString(),
                 ListOptions::Reverse);
        return self.copy(ll);
    }
}

template<typename T>
Map Map::fromMultiMapRef(const Path &pathFromOwner, const QMultiMap<QString, T> &mmap)
{
    return Map(
            pathFromOwner,
            [&mmap](const DomItem &self, const QString &key) {
                return keyMultiMapHelper(self, key, mmap);
            },
            [&mmap](const DomItem &) { return QSet<QString>(mmap.keyBegin(), mmap.keyEnd()); },
            QLatin1String(typeid(T).name()));
}

template<typename T>
Map Map::fromMapRef(
        const Path &pathFromOwner, const QMap<QString, T> &map,
        const std::function<DomItem(const DomItem &, const PathEls::PathComponent &, const T &)> &elWrapper)
{
    return Map(
            pathFromOwner,
            [&map, elWrapper](const DomItem &self, const QString &key) {
                const auto it = map.constFind(key);
                if (it == map.constEnd())
                    return DomItem();
                return elWrapper(self, PathEls::Key(key), it.value());
            },
            [&map](const DomItem &) { return QSet<QString>(map.keyBegin(), map.keyEnd()); },
            QLatin1String(typeid(T).name()));
}

template<typename MapT>
QSet<QString> Map::fileRegionKeysFromMap(const MapT &map)
{
    QSet<QString> keys;
    std::transform(map.keyBegin(), map.keyEnd(), std::inserter(keys, keys.begin()), fileLocationRegionName);
    return keys;
}

template<typename T>
Map Map::fromFileRegionMap(const Path &pathFromOwner, const QMap<FileLocationRegion, T> &map)
{
    auto result = Map(
            pathFromOwner,
            [&map](const DomItem &mapItem, const QString &key) -> DomItem {
                auto it = map.constFind(fileLocationRegionValue(key));
                if (it == map.constEnd())
                    return {};

                return mapItem.wrap(PathEls::Key(key), *it);
            },
            [&map](const DomItem &) { return fileRegionKeysFromMap(map); },
            QString::fromLatin1(typeid(T).name()));
    return result;
}

template<typename T>
Map Map::fromFileRegionListMap(const Path &pathFromOwner,
                                   const QMap<FileLocationRegion, QList<T>> &map)
{
    using namespace Qt::StringLiterals;
    auto result = Map(
            pathFromOwner,
            [&map](const DomItem &mapItem, const QString &key) -> DomItem {
                const QList<SourceLocation> locations = map.value(fileLocationRegionValue(key));
                if (locations.empty())
                    return {};

                auto list = List::fromQList<SourceLocation>(
                        mapItem.pathFromOwner(), locations,
                        [](const DomItem &self, const PathEls::PathComponent &path,
                           const SourceLocation &location) {
                            return self.subLocationItem(path, location);
                        });
                return mapItem.subListItem(list);
            },
            [&map](const DomItem &) { return fileRegionKeysFromMap(map); },
            u"QList<%1>"_s.arg(QString::fromLatin1(typeid(T).name())));
    return result;
}

template<typename T>
List List::fromQList(
        const Path &pathFromOwner, const QList<T> &list,
        const std::function<DomItem(const DomItem &, const PathEls::PathComponent &, const T &)> &elWrapper,
        ListOptions options)
{
    index_type len = list.size();
    if (options == ListOptions::Reverse) {
        return List(
                pathFromOwner,
                [list, elWrapper](const DomItem &self, index_type i) mutable {
                    if (i < 0 || i >= list.size())
                        return DomItem();
                    return elWrapper(self, PathEls::Index(i), list[list.size() - i - 1]);
                },
                [len](const DomItem &) { return len; }, nullptr, QLatin1String(typeid(T).name()));
    } else {
        return List(
                pathFromOwner,
                [list, elWrapper](const DomItem &self, index_type i) mutable {
                    if (i < 0 || i >= list.size())
                        return DomItem();
                    return elWrapper(self, PathEls::Index(i), list[i]);
                },
                [len](const DomItem &) { return len; }, nullptr, QLatin1String(typeid(T).name()));
    }
}

template<typename T>
List List::fromQListRef(
        const Path &pathFromOwner, const QList<T> &list,
        const std::function<DomItem(const DomItem &, const PathEls::PathComponent &, const T &)> &elWrapper,
        ListOptions options)
{
    if (options == ListOptions::Reverse) {
        return List(
                pathFromOwner,
                [&list, elWrapper](const DomItem &self, index_type i) {
                    if (i < 0 || i >= list.size())
                        return DomItem();
                    return elWrapper(self, PathEls::Index(i), list[list.size() - i - 1]);
                },
                [&list](const DomItem &) { return list.size(); }, nullptr,
                QLatin1String(typeid(T).name()));
    } else {
        return List(
                pathFromOwner,
                [&list, elWrapper](const DomItem &self, index_type i) {
                    if (i < 0 || i >= list.size())
                        return DomItem();
                    return elWrapper(self, PathEls::Index(i), list[i]);
                },
                [&list](const DomItem &) { return list.size(); }, nullptr,
                QLatin1String(typeid(T).name()));
    }
}

class QMLDOM_EXPORT OwningItem: public DomBase {
protected:
    virtual std::shared_ptr<OwningItem> doCopy(const DomItem &self) const = 0;

public:
    OwningItem(const OwningItem &o);
    OwningItem(int derivedFrom=0);
    OwningItem(int derivedFrom, const QDateTime &lastDataUpdateAt);
    OwningItem(const OwningItem &&) = delete;
    OwningItem &operator=(const OwningItem &&) = delete;
    static int nextRevision();

    Path canonicalPath(const DomItem &self) const override = 0;

    bool iterateDirectSubpaths(const DomItem &self, DirectVisitor) const override;
    std::shared_ptr<OwningItem> makeCopy(const DomItem &self) const { return doCopy(self); }
    Path pathFromOwner() const { return Path(); }
    Path pathFromOwner(const DomItem &) const override final { return Path(); }
    DomItem containingObject(const DomItem &self) const override;
    int derivedFrom() const;
    virtual int revision() const;

    QDateTime createdAt() const;
    virtual QDateTime lastDataUpdateAt() const;
    virtual void refreshedDataAt(QDateTime tNew);

    // explicit freeze handling needed?
    virtual bool frozen() const;
    virtual bool freeze();
    QDateTime frozenAt() const;

    virtual void addError(const DomItem &self, ErrorMessage &&msg);
    void addErrorLocal(ErrorMessage &&msg);
    void clearErrors(const ErrorGroups &groups = ErrorGroups({}));
    // return false if a quick exit was requested
    bool iterateErrors(
            const DomItem &self,
            function_ref<bool(const DomItem &source, const ErrorMessage &msg)> visitor,
            const Path &inPath = Path());
    QMultiMap<Path, ErrorMessage> localErrors() const {
        QMutexLocker l(mutex());
        return m_errors;
    }

    virtual bool iterateSubOwners(const DomItem &self, function_ref<bool(const DomItem &owner)> visitor);

    QBasicMutex *mutex() const { return &m_mutex; }
private:
    mutable QBasicMutex m_mutex;
    int m_derivedFrom;
    int m_revision;
    QDateTime m_createdAt;
    QDateTime m_lastDataUpdateAt;
    QDateTime m_frozenAt;
    QMultiMap<Path, ErrorMessage> m_errors;
    QMap<ErrorMessage, quint32> m_errorsCounts;
};

template<typename T>
std::shared_ptr<T> DomItem::ownerAs() const
{
    if constexpr (domTypeIsOwningItem(T::kindValue)) {
        if (!std::holds_alternative<std::monostate>(m_owner)) {
            if constexpr (T::kindValue == DomType::FileLocationsNode) {
                if (std::holds_alternative<std::shared_ptr<FileLocations::Node>>(m_owner))
                    return std::static_pointer_cast<T>(
                            std::get<std::shared_ptr<FileLocations::Node>>(m_owner));
            } else if constexpr (T::kindValue == DomType::ExternalItemInfo) {
                if (std::holds_alternative<std::shared_ptr<ExternalItemInfoBase>>(m_owner))
                    return std::static_pointer_cast<T>(
                            std::get<std::shared_ptr<ExternalItemInfoBase>>(m_owner));
            } else if constexpr (T::kindValue == DomType::ExternalItemPair) {
                if (std::holds_alternative<std::shared_ptr<ExternalItemPairBase>>(m_owner))
                    return std::static_pointer_cast<T>(
                            std::get<std::shared_ptr<ExternalItemPairBase>>(m_owner));
            } else {
                if (std::holds_alternative<std::shared_ptr<T>>(m_owner)) {
                    return std::get<std::shared_ptr<T>>(m_owner);
                }
            }
        }
    } else {
        Q_ASSERT_X(false, "DomItem::ownerAs", "unexpected non owning value in ownerAs");
    }
    return std::shared_ptr<T> {};
}

template<int I>
struct rank : rank<I - 1>
{
    static_assert(I > 0, "");
};
template<>
struct rank<0>
{
};

template<typename T>
auto writeOutWrap(const T &t, const DomItem &self, OutWriter &lw, rank<1>)
        -> decltype(t.writeOut(self, lw))
{
    t.writeOut(self, lw);
}

template<typename T>
auto writeOutWrap(const T &, const DomItem &, OutWriter &, rank<0>) -> void
{
    qCWarning(writeOutLog) << "Ignoring writeout to wrapped object not supporting it ("
                           << typeid(T).name();
}
template<typename T>
auto writeOutWrap(const T &t, const DomItem &self, OutWriter &lw) -> void
{
    writeOutWrap(t, self, lw, rank<1>());
}

template<typename T>
void SimpleObjectWrapT<T>::writeOut(const DomItem &self, OutWriter &lw) const
{
    writeOutWrap<T>(*asT(), self, lw);
}

QMLDOM_EXPORT QDebug operator<<(QDebug debug, const DomItem &c);

class QMLDOM_EXPORT MutableDomItem {
public:
    using CopyOption = DomItem::CopyOption;

    explicit operator bool() const
    {
        return bool(m_owner);
    } // this is weaker than item(), but normally correct
    DomType internalKind() { return item().internalKind(); }
    QString internalKindStr() { return domTypeToString(internalKind()); }
    DomKind domKind() { return kind2domKind(internalKind()); }

    Path canonicalPath() const { return m_owner.canonicalPath().withPath(m_pathFromOwner); }
    MutableDomItem containingObject()
    {
        if (m_pathFromOwner)
            return MutableDomItem(m_owner, m_pathFromOwner.split().pathToSource);
        else {
            DomItem cObj = m_owner.containingObject();
            return MutableDomItem(cObj.owner(), (domTypeIsOwningItem(cObj.internalKind()) ? Path() :cObj.pathFromOwner()));
        }
    }

    MutableDomItem container()
    {
        if (m_pathFromOwner)
            return MutableDomItem(m_owner, m_pathFromOwner.dropTail());
        else {
            return MutableDomItem(item().container());
        }
    }

    MutableDomItem qmlObject(GoTo option = GoTo::Strict,
                             FilterUpOptions fOptions = FilterUpOptions::ReturnOuter)
    {
        return MutableDomItem(item().qmlObject(option, fOptions));
    }
    MutableDomItem fileObject(GoTo option = GoTo::Strict)
    {
        return MutableDomItem(item().fileObject(option));
    }
    MutableDomItem rootQmlObject(GoTo option = GoTo::Strict)
    {
        return MutableDomItem(item().rootQmlObject(option));
    }
    MutableDomItem globalScope() { return MutableDomItem(item().globalScope()); }
    MutableDomItem scope() { return MutableDomItem(item().scope()); }

    MutableDomItem component(GoTo option = GoTo::Strict)
    {
        return MutableDomItem { item().component(option) };
    }
    MutableDomItem owner() { return MutableDomItem(m_owner); }
    MutableDomItem top() { return MutableDomItem(item().top()); }
    MutableDomItem environment() { return MutableDomItem(item().environment()); }
    MutableDomItem universe() { return MutableDomItem(item().universe()); }
    Path pathFromOwner() { return m_pathFromOwner; }
    MutableDomItem operator[](const Path &path) { return MutableDomItem(item()[path]); }
    MutableDomItem operator[](QStringView component) { return MutableDomItem(item()[component]); }
    MutableDomItem operator[](const QString &component)
    {
        return MutableDomItem(item()[component]);
    }
    MutableDomItem operator[](const char16_t *component)
    {
        // to avoid clash with stupid builtin ptrdiff_t[MutableDomItem&], coming from C
        return MutableDomItem(item()[QStringView(component)]);
    }
    MutableDomItem operator[](index_type i) { return MutableDomItem(item().index(i)); }

    MutableDomItem path(const Path &p) { return MutableDomItem(item().path(p)); }
    MutableDomItem path(const QString &p) { return path(Path::fromString(p)); }
    MutableDomItem path(QStringView p) { return path(Path::fromString(p)); }

    QList<QString> const fields() { return item().fields(); }
    MutableDomItem field(QStringView name) { return MutableDomItem(item().field(name)); }
    index_type indexes() { return item().indexes(); }
    MutableDomItem index(index_type i) { return MutableDomItem(item().index(i)); }

    QSet<QString> const keys() { return item().keys(); }
    MutableDomItem key(const QString &name) { return MutableDomItem(item().key(name)); }
    MutableDomItem key(QStringView name) { return key(name.toString()); }

    void
    dump(const Sink &s, int indent = 0,
         function_ref<bool(const DomItem &, const PathEls::PathComponent &, const DomItem &)> filter = noFilter)
    {
        item().dump(s, indent, filter);
    }
    FileWriter::Status
    dump(const QString &path,
         function_ref<bool(const DomItem &, const PathEls::PathComponent &, const DomItem &)> filter = noFilter,
         int nBackups = 2, int indent = 0, FileWriter *fw = nullptr)
    {
        return item().dump(path, filter, nBackups, indent, fw);
    }
    void writeOut(OutWriter &lw) { return item().writeOut(lw); }
    bool writeOut(const QString &path, int nBackups = 2,
                  const LineWriterOptions &opt = LineWriterOptions(), FileWriter *fw = nullptr)
    {
        return item().writeOut(path, nBackups, opt, fw);
    }

    MutableDomItem makeCopy(CopyOption option = CopyOption::EnvConnected)
    {
        return item().makeCopy(option);
    }
    bool commitToBase(const std::shared_ptr<DomEnvironment> &validEnvPtr = nullptr)
    {
        return item().commitToBase(validEnvPtr);
    }
    QString canonicalFilePath() const { return item().canonicalFilePath(); }

    MutableDomItem refreshed() { return MutableDomItem(item().refreshed()); }

    QCborValue value() { return item().value(); }

    QString toString() { return item().toString(); }

    // convenience getters
    QString name() { return item().name(); }
    MutableDomItem pragmas() { return item().pragmas(); }
    MutableDomItem ids() { return MutableDomItem::item().ids(); }
    QString idStr() { return item().idStr(); }
    MutableDomItem propertyDefs() { return MutableDomItem(item().propertyDefs()); }
    MutableDomItem bindings() { return MutableDomItem(item().bindings()); }
    MutableDomItem methods() { return MutableDomItem(item().methods()); }
    MutableDomItem children() { return MutableDomItem(item().children()); }
    MutableDomItem child(index_type i) { return MutableDomItem(item().child(i)); }
    MutableDomItem annotations() { return MutableDomItem(item().annotations()); }

    //    // OwnigItem elements
    int derivedFrom() { return m_owner.derivedFrom(); }
    int revision() { return m_owner.revision(); }
    QDateTime createdAt() { return m_owner.createdAt(); }
    QDateTime frozenAt() { return m_owner.frozenAt(); }
    QDateTime lastDataUpdateAt() { return m_owner.lastDataUpdateAt(); }

    void addError(ErrorMessage &&msg) { item().addError(std::move(msg)); }
    ErrorHandler errorHandler();

    // convenience setters
    MutableDomItem addPrototypePath(const Path &prototypePath);
    MutableDomItem setNextScopePath(const Path &nextScopePath);
    MutableDomItem setPropertyDefs(QMultiMap<QString, PropertyDefinition> propertyDefs);
    MutableDomItem setBindings(QMultiMap<QString, Binding> bindings);
    MutableDomItem setMethods(QMultiMap<QString, MethodInfo> functionDefs);
    MutableDomItem setChildren(const QList<QmlObject> &children);
    MutableDomItem setAnnotations(const QList<QmlObject> &annotations);
    MutableDomItem setScript(const std::shared_ptr<ScriptExpression> &exp);
    MutableDomItem setCode(const QString &code);
    MutableDomItem addPropertyDef(const PropertyDefinition &propertyDef,
                                  AddOption option = AddOption::Overwrite);
    MutableDomItem addBinding(Binding binding, AddOption option = AddOption::Overwrite);
    MutableDomItem addMethod(
            const MethodInfo &functionDef, AddOption option = AddOption::Overwrite);
    MutableDomItem addChild(QmlObject child);
    MutableDomItem addAnnotation(QmlObject child);
    MutableDomItem addPreComment(const Comment &comment, FileLocationRegion region);
    MutableDomItem addPostComment(const Comment &comment, FileLocationRegion region);
    QQmlJSScope::ConstPtr semanticScope();
    void setSemanticScope(const QQmlJSScope::ConstPtr &scope);

    MutableDomItem() = default;
    MutableDomItem(const DomItem &owner, const Path &pathFromOwner):
        m_owner(owner), m_pathFromOwner(pathFromOwner)
    {}
    MutableDomItem(const DomItem &item):
        m_owner(item.owner()), m_pathFromOwner(item.pathFromOwner())
    {}

    std::shared_ptr<DomTop> topPtr() { return m_owner.topPtr(); }
    std::shared_ptr<OwningItem> owningItemPtr() { return m_owner.owningItemPtr(); }

    template<typename T>
    T const *as()
    {
        return item().as<T>();
    }

    template <typename T>
    T *mutableAs() {
        Q_ASSERT(!m_owner || !m_owner.owningItemPtr()->frozen());

        DomItem self = item();
        if (self.m_kind != T::kindValue)
            return nullptr;

        const T *t = nullptr;
        if constexpr (domTypeIsObjWrap(T::kindValue) || domTypeIsValueWrap(T::kindValue))
            t = static_cast<const SimpleObjectWrapBase *>(self.base())->as<T>();
        else if constexpr (std::is_base_of<DomBase, T>::value)
            t = static_cast<const T *>(self.base());
        else
            Q_UNREACHABLE_RETURN(nullptr);

        // Nasty. But since ElementT has to store the const pointers, we allow it in this one place.
        return const_cast<T *>(t);
    }

    template<typename T>
    std::shared_ptr<T> ownerAs() const
    {
        return m_owner.ownerAs<T>();
    }
    // it is dangerous to assume it stays valid when updates are preformed...
    DomItem item() const { return m_owner.path(m_pathFromOwner); }

    friend bool operator==(const MutableDomItem &o1, const MutableDomItem &o2)
    {
        return o1.m_owner == o2.m_owner && o1.m_pathFromOwner == o2.m_pathFromOwner;
    }
    friend bool operator!=(const MutableDomItem &o1, const MutableDomItem &o2)
    {
        return !(o1 == o2);
    }

private:
    DomItem m_owner;
    Path m_pathFromOwner;
};

QMLDOM_EXPORT QDebug operator<<(QDebug debug, const MutableDomItem &c);

template<typename K, typename T>
Path insertUpdatableElementInMultiMap(const Path &mapPathFromOwner, QMultiMap<K, T> &mmap, K key,
                                      const T &value, AddOption option = AddOption::KeepExisting,
                                      T **valuePtr = nullptr)
{
    if (option == AddOption::Overwrite) {
        auto it = mmap.find(key);
        if (it != mmap.end()) {
            T &v = *it;
            v = value;
            if (++it != mmap.end() && it.key() == key) {
                qWarning() << " requested overwrite of " << key
                           << " that contains aleready multiple entries in" << mapPathFromOwner;
            }
            Path newPath = mapPathFromOwner.withKey(key).withIndex(0);
            v.updatePathFromOwner(newPath);
            if (valuePtr)
                *valuePtr = &v;
            return newPath;
        }
    }
    mmap.insert(key, value);
    auto it = mmap.find(key);
    auto it2 = it;
    int nVal = 0;
    while (it2 != mmap.end() && it2.key() == key) {
       ++nVal;
        ++it2;
    }
    Path newPath = mapPathFromOwner.withKey(key).withIndex(nVal-1);
    T &v = *it;
    v.updatePathFromOwner(newPath);
    if (valuePtr)
        *valuePtr = &v;
    return newPath;
}

template<typename T>
Path appendUpdatableElementInQList(const Path &listPathFromOwner, QList<T> &list, const T &value,
                                   T **vPtr = nullptr)
{
    int idx = list.size();
    list.append(value);
    Path newPath = listPathFromOwner.withIndex(idx);
    T &targetV = list[idx];
    targetV.updatePathFromOwner(newPath);
    if (vPtr)
        *vPtr = &targetV;
    return newPath;
}

template <typename T, typename K = QString>
void updatePathFromOwnerMultiMap(QMultiMap<K, T> &mmap, const Path &newPath)
{
    auto it = mmap.begin();
    auto end = mmap.end();
    index_type i = 0;
    K name;
    QList<T*> els;
    while (it != end) {
        if (i > 0 && name != it.key()) {
            Path pName = newPath.withKey(QString(name));
            for (T *el : els)
                el->updatePathFromOwner(pName.withIndex(--i));
            els.clear();
            els.append(&(*it));
            name = it.key();
            i = 1;
        } else {
            els.append(&(*it));
            name = it.key();
            ++i;
        }
        ++it;
    }
    Path pName = newPath.withKey(name);
    for (T *el : els)
        el->updatePathFromOwner(pName.withIndex(--i));
}

template <typename T>
void updatePathFromOwnerQList(QList<T> &list, const Path &newPath)
{
    auto it = list.begin();
    auto end = list.end();
    index_type i = 0;
    while (it != end)
        (it++)->updatePathFromOwner(newPath.withIndex(i++));
}

constexpr bool domTypeIsObjWrap(DomType k)
{
    switch (k) {
    case DomType::Binding:
    case DomType::EnumItem:
    case DomType::ErrorMessage:
    case DomType::Export:
    case DomType::Id:
    case DomType::Import:
    case DomType::ImportScope:
    case DomType::MethodInfo:
    case DomType::MethodParameter:
    case DomType::ModuleAutoExport:
    case DomType::Pragma:
    case DomType::PropertyDefinition:
    case DomType::Version:
    case DomType::Comment:
    case DomType::CommentedElement:
    case DomType::RegionComments:
    case DomType::FileLocationsInfo:
        return true;
    default:
        return false;
    }
}

constexpr bool domTypeIsValueWrap(DomType k)
{
    switch (k) {
    case DomType::PropertyInfo:
        return true;
    default:
        return false;
    }
}

constexpr bool domTypeIsDomElement(DomType k)
{
    switch (k) {
    case DomType::ModuleScope:
    case DomType::QmlObject:
    case DomType::ConstantData:
    case DomType::SimpleObjectWrap:
    case DomType::Reference:
    case DomType::Map:
    case DomType::List:
    case DomType::ListP:
    case DomType::EnumDecl:
    case DomType::JsResource:
    case DomType::QmltypesComponent:
    case DomType::QmlComponent:
    case DomType::GlobalComponent:
    case DomType::MockObject:
        return true;
    default:
        return false;
    }
}

constexpr bool domTypeIsOwningItem(DomType k)
{
    switch (k) {
    case DomType::ModuleIndex:

    case DomType::MockOwner:

    case DomType::ExternalItemInfo:
    case DomType::ExternalItemPair:

    case DomType::QmlDirectory:
    case DomType::QmldirFile:
    case DomType::JsFile:
    case DomType::QmlFile:
    case DomType::QmltypesFile:
    case DomType::GlobalScope:

    case DomType::ScriptExpression:
    case DomType::AstComments:

    case DomType::LoadInfo:
    case DomType::FileLocationsNode:

    case DomType::DomEnvironment:
    case DomType::DomUniverse:
        return true;
    default:
        return false;
    }
}

constexpr bool domTypeIsUnattachedOwningItem(DomType k)
{
    switch (k) {
    case DomType::ScriptExpression:
    case DomType::AstComments:
    case DomType::FileLocationsNode:
        return true;
    default:
        return false;
    }
}

constexpr bool domTypeIsScriptElement(DomType k)
{
    return DomType::ScriptElementStart <= k && k <= DomType::ScriptElementStop;
}

template<typename T>
DomItem DomItem::subValueItem(const PathEls::PathComponent &c, const T &value,
                              ConstantData::Options options) const
{
    using BaseT = std::remove_cv_t<std::remove_reference_t<T>>;
    if constexpr (
            std::is_base_of_v<
                    QCborValue,
                    BaseT> || std::is_base_of_v<QCborArray, BaseT> || std::is_base_of_v<QCborMap, BaseT>) {
        return DomItem(m_top, m_owner, m_ownerPath,
                       ConstantData(pathFromOwner().withComponent(c), value, options));
    } else if constexpr (std::is_same_v<DomItem, BaseT>) {
        Q_UNUSED(options);
        return value;
    } else if constexpr (IsList<T>::value && !std::is_convertible_v<BaseT, QStringView>) {
        return subListItem(List::fromQList<typename BaseT::value_type>(
                pathFromOwner().withComponent(c), value,
                [options](const DomItem &list, const PathEls::PathComponent &p,
                          const typename T::value_type &v) { return list.subValueItem(p, v, options); }));
    } else if constexpr (IsSharedPointerToDomObject<BaseT>::value) {
        Q_UNUSED(options);
        return subOwnerItem(c, value);
    } else {
        return subDataItem(c, value, options);
    }
}

template<typename T>
DomItem DomItem::subDataItem(const PathEls::PathComponent &c, const T &value,
                             ConstantData::Options options) const
{
    using BaseT = std::remove_cv_t<std::remove_reference_t<T>>;
    if constexpr (std::is_same_v<BaseT, ConstantData>) {
        return this->copy(value);
    } else if constexpr (std::is_base_of_v<QCborValue, BaseT>) {
        return DomItem(m_top, m_owner, m_ownerPath,
                       ConstantData(pathFromOwner().withComponent(c), value, options));
    } else {
        return DomItem(
                m_top, m_owner, m_ownerPath,
                ConstantData(pathFromOwner().withComponent(c), QCborValue(value), options));
    }
}

template<typename T>
bool DomItem::dvValue(DirectVisitor visitor, const PathEls::PathComponent &c, const T &value,
                      ConstantData::Options options) const
{
    auto lazyWrap = [this, &c, &value, options]() {
        return this->subValueItem<T>(c, value, options);
    };
    return visitor(c, lazyWrap);
}

template<typename F>
bool DomItem::dvValueLazy(DirectVisitor visitor, const PathEls::PathComponent &c, F valueF,
                          ConstantData::Options options) const
{
    auto lazyWrap = [this, &c, &valueF, options]() {
        return this->subValueItem<decltype(valueF())>(c, valueF(), options);
    };
    return visitor(c, lazyWrap);
}

template<typename T>
DomItem DomItem::wrap(const PathEls::PathComponent &c, const T &obj) const
{
    using BaseT = std::decay_t<T>;
    if constexpr (std::is_same_v<QString, BaseT> || std::is_arithmetic_v<BaseT>) {
        return this->subDataItem(c, QCborValue(obj));
    } else if constexpr (std::is_same_v<SourceLocation, BaseT>) {
        return this->subLocationItem(c, obj);
    } else if constexpr (std::is_same_v<BaseT, Reference>) {
        Q_ASSERT_X(false, "DomItem::wrap",
                   "wrapping a reference object, probably an error (wrap the target path instead)");
        return this->copy(obj);
    } else if constexpr (std::is_same_v<BaseT, ConstantData>) {
        return this->subDataItem(c, obj);
    } else if constexpr (std::is_same_v<BaseT, Map>) {
        return this->subMapItem(obj);
    } else if constexpr (std::is_same_v<BaseT, List>) {
        return this->subListItem(obj);
    } else if constexpr (std::is_base_of_v<ListPBase, BaseT>) {
        return this->subListItem(obj);
    } else if constexpr (std::is_same_v<BaseT, SimpleObjectWrap>) {
        return this->subObjectWrapItem(obj);
    } else if constexpr (IsDomObject<BaseT>::value) {
        if constexpr (domTypeIsObjWrap(BaseT::kindValue) || domTypeIsValueWrap(BaseT::kindValue)) {
            return this->subObjectWrapItem(
                    SimpleObjectWrap::fromObjectRef(this->pathFromOwner().withComponent(c), obj));
        } else if constexpr (domTypeIsDomElement(BaseT::kindValue)) {
            return this->copy(&obj);
        } else {
            qCWarning(domLog) << "Unhandled object of type " << domTypeToString(BaseT::kindValue)
                              << " in DomItem::wrap, not using a shared_ptr for an "
                              << "OwningItem, or unexpected wrapped object?";
            return DomItem();
        }
    } else if constexpr (IsSharedPointerToDomObject<BaseT>::value) {
        if constexpr (domTypeIsOwningItem(BaseT::element_type::kindValue)) {
            return this->subOwnerItem(c, obj);
        } else {
            Q_ASSERT_X(false, "DomItem::wrap", "shared_ptr with non owning item");
            return DomItem();
        }
    } else if constexpr (IsMultiMap<BaseT>::value) {
        if constexpr (std::is_same_v<typename BaseT::key_type, QString>) {
            return subMapItem(Map::fromMultiMapRef<typename BaseT::mapped_type>(
                    pathFromOwner().withComponent(c), obj));
        } else {
            Q_ASSERT_X(false, "DomItem::wrap", "non string keys not supported (try .toString()?)");
        }
    } else if constexpr (IsMap<BaseT>::value) {
        if constexpr (std::is_same_v<typename BaseT::key_type, QString>) {
            return subMapItem(Map::fromMapRef<typename BaseT::mapped_type>(
                    pathFromOwner().withComponent(c), obj,
                    [](const DomItem &map, const PathEls::PathComponent &p,
                       const typename BaseT::mapped_type &el) { return map.wrap(p, el); }));
        } else {
            Q_ASSERT_X(false, "DomItem::wrap", "non string keys not supported (try .toString()?)");
        }
    } else if constexpr (IsList<BaseT>::value) {
        if constexpr (IsDomObject<typename BaseT::value_type>::value) {
            return subListItem(List::fromQListRef<typename BaseT::value_type>(
                    pathFromOwner().withComponent(c), obj,
                    [](const DomItem &list, const PathEls::PathComponent &p,
                       const typename BaseT::value_type &el) { return list.wrap(p, el); }));
        } else {
            Q_ASSERT_X(false, "DomItem::wrap", "Unsupported list type T");
            return DomItem();
        }
    } else {
        qCWarning(domLog) << "Cannot wrap " << typeid(BaseT).name();
        Q_ASSERT_X(false, "DomItem::wrap", "Do not know how to wrap type T");
        return DomItem();
    }
}

template<typename T>
bool DomItem::dvWrap(DirectVisitor visitor, const PathEls::PathComponent &c, T &obj) const
{
    auto lazyWrap = [this, &c, &obj]() { return this->wrap<T>(c, obj); };
    return visitor(c, lazyWrap);
}

template<typename T>
bool ListPT<T>::iterateDirectSubpaths(const DomItem &self, DirectVisitor v) const
{
    index_type len = index_type(m_pList.size());
    for (index_type i = 0; i < len; ++i) {
        if (!v(PathEls::Index(i), [this, &self, i] { return this->index(self, i); }))
            return false;
    }
    return true;
}

template<typename T>
DomItem ListPT<T>::index(const DomItem &self, index_type index) const
{
    if (index >= 0 && index < m_pList.size())
        return self.wrap(PathEls::Index(index), *static_cast<const T *>(m_pList.value(index)));
    return DomItem();
}

// allow inlining of DomBase
inline DomKind DomBase::domKind() const
{
    return kind2domKind(kind());
}

inline bool DomBase::iterateDirectSubpathsConst(const DomItem &self, DirectVisitor visitor) const
{
    Q_ASSERT(self.base() == this);
    return self.iterateDirectSubpaths(std::move(visitor));
}

inline DomItem DomBase::containingObject(const DomItem &self) const
{
    Path path = pathFromOwner(self);
    DomItem base = self.owner();
    if (!path) {
        path = canonicalPath(self);
        base = self;
    }
    Source source = path.split();
    return base.path(source.pathToSource);
}

inline quintptr DomBase::id() const
{
    return quintptr(this);
}

inline QString DomBase::typeName() const
{
    return domTypeToString(kind());
}

inline QList<QString> DomBase::fields(const DomItem &self) const
{
    QList<QString> res;
    self.iterateDirectSubpaths([&res](const PathEls::PathComponent &c, function_ref<DomItem()>) {
        if (c.kind() == Path::Kind::Field)
            res.append(c.name());
        return true;
    });
    return res;
}

inline DomItem DomBase::field(const DomItem &self, QStringView name) const
{
    DomItem res;
    self.iterateDirectSubpaths(
            [&res, name](const PathEls::PathComponent &c, function_ref<DomItem()> obj) {
                if (c.kind() == Path::Kind::Field && c.checkName(name)) {
                    res = obj();
                    return false;
                }
                return true;
            });
    return res;
}

inline index_type DomBase::indexes(const DomItem &self) const
{
    index_type res = 0;
    self.iterateDirectSubpaths([&res](const PathEls::PathComponent &c, function_ref<DomItem()>) {
        if (c.kind() == Path::Kind::Index) {
            index_type i = c.index() + 1;
            if (res < i)
                res = i;
        }
        return true;
    });
    return res;
}

inline DomItem DomBase::index(const DomItem &self, qint64 index) const
{
    DomItem res;
    self.iterateDirectSubpaths(
            [&res, index](const PathEls::PathComponent &c, function_ref<DomItem()> obj) {
                if (c.kind() == Path::Kind::Index && c.index() == index) {
                    res = obj();
                    return false;
                }
                return true;
            });
    return res;
}

inline QSet<QString> const DomBase::keys(const DomItem &self) const
{
    QSet<QString> res;
    self.iterateDirectSubpaths([&res](const PathEls::PathComponent &c, function_ref<DomItem()>) {
        if (c.kind() == Path::Kind::Key)
            res.insert(c.name());
        return true;
    });
    return res;
}

inline DomItem DomBase::key(const DomItem &self, const QString &name) const
{
    DomItem res;
    self.iterateDirectSubpaths(
            [&res, name](const PathEls::PathComponent &c, function_ref<DomItem()> obj) {
                if (c.kind() == Path::Kind::Key && c.checkName(name)) {
                    res = obj();
                    return false;
                }
                return true;
            });
    return res;
}

inline DomItem DomItem::subListItem(const List &list) const
{
    return DomItem(m_top, m_owner, m_ownerPath, list);
}

inline DomItem DomItem::subMapItem(const Map &map) const
{
    return DomItem(m_top, m_owner, m_ownerPath, map);
}

} // end namespace Dom
} // end namespace QQmlJS

QT_END_NAMESPACE
#endif // QMLDOMITEM_H
