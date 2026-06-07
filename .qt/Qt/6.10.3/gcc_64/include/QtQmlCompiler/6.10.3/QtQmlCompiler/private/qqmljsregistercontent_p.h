// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:significant

#ifndef QQMLJSREGISTERCONTENT_P_H
#define QQMLJSREGISTERCONTENT_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#include "qqmljsscope_p.h"
#include <QtCore/qhash.h>
#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE

struct QQmlJSRegisterContentPrivate;
class Q_QMLCOMPILER_EXPORT QQmlJSRegisterContent
{
public:
    // ContentVariant determines the relation between this register content and its scope().
    // For example, a property is always a property of a type. That type is given as scope.
    // Most content variants can carry either a specific kind of content, as commented below,
    // or a conversion. If two or more register contents of the same content variant are merged,
    // they retain their content variant but become a conversion with the original register
    // contents linked as conversion origins.

    enum ContentVariant {
        ObjectById,   // type (scope is QML scope of binding/function)
        TypeByName,   // type (TODO: scope is not guaranteed to be useful)
        Singleton,    // type (scope is either import namespace or QML scope)
        Script,       // type (scope is either import namespace or QML scope)
        MetaType,     // type (always QMetaObject, scope is the type reprented by the metaobject)
        Extension,    // type (scope is the type being extended)
        ScopeObject,  // type (either QML scope of binding/function or JS global object)
        ParentScope,  // type (scope is the child scope)

        Property,     // property (scope is the owner (hasOwnProperty) of the property)
        Method,       // method (retrieved as property, including overloads), like property
        Enum,         // enumeration (scope is the type the enumeration belongs to)

        Attachment,   // type (scope is attacher; use attacher() and attachee() for clarity)
        ModulePrefix, // import namespace (scope is either QML scope or type the prefix is used on)

        MethodCall,   // method call (resolved to specific overload), like property

        ListValue,    // property (scope is list retrieved from)
        ListIterator, // property (scope is list being iterated)

        Literal,      // type (scope does not exist)
        Operation,    // type (scope does not exist)

        BaseType,     // type (scope is derived type)
        Cast,         // type (scope is type casted from)

        Storage,      // type (scope does not exist)

        // Either a synthetic type or a merger of multiple different variants.
        // In the latter case, look at conversion origins to find out more.
        // Synthetic types should be short lived.
        Unknown,
    };

    enum { InvalidLookupIndex = -1 };

    QQmlJSRegisterContent() = default;


    // General properties of the register content, (mostly) independent of kind or variant

    bool isNull() const { return !d; }
    bool isValid() const;

    bool isList() const;
    bool isWritable() const;

    ContentVariant variant() const;

    QString descriptiveName() const;
    QString containedTypeName() const;

    int resultLookupIndex() const;

    QQmlJSScope::ConstPtr storedType() const;
    QQmlJSScope::ConstPtr containedType() const;
    QQmlJSScope::ConstPtr scopeType() const;

    bool contains(const QQmlJSScope::ConstPtr &type) const { return type == containedType(); }
    bool isStoredIn(const QQmlJSScope::ConstPtr &type) const { return type == storedType(); }


    // Properties of specific kinds of register contents

    bool isType() const;
    QQmlJSScope::ConstPtr type() const;

    bool isProperty() const;
    QQmlJSMetaProperty property() const;
    int baseLookupIndex() const;

    bool isEnumeration() const;
    QQmlJSMetaEnum enumeration() const;
    QString enumMember() const;

    bool isMethod() const;
    QList<QQmlJSMetaMethod> method() const;
    QQmlJSScope::ConstPtr methodType() const;

    bool isImportNamespace() const;
    uint importNamespace() const;
    QQmlJSScope::ConstPtr importNamespaceType() const;

    bool isConversion() const;
    QQmlJSScope::ConstPtr conversionResultType() const;
    QQmlJSRegisterContent conversionResultScope() const;
    QList<QQmlJSRegisterContent> conversionOrigins() const;

    bool isMethodCall() const;
    QQmlJSMetaMethod methodCall() const;
    bool isJavaScriptReturnValue() const;


    // Linked register contents

    QQmlJSRegisterContent attacher() const;
    QQmlJSRegisterContent attachee() const;

    QQmlJSRegisterContent scope() const;
    QQmlJSRegisterContent storage() const;
    QQmlJSRegisterContent original() const;
    QQmlJSRegisterContent shadowed() const;

    quintptr id() const { return quintptr(d); }

private:
    friend class QQmlJSRegisterContentPool;
    // TODO: Constant string/number/bool/enumval

    QQmlJSRegisterContent(QQmlJSRegisterContentPrivate *dd) : d(dd) {};

    friend bool operator==(QQmlJSRegisterContent a, QQmlJSRegisterContent b)
    {
        return a.d == b.d;
    }

    friend bool operator!=(QQmlJSRegisterContent a, QQmlJSRegisterContent b)
    {
        return !(a == b);
    }

    friend size_t qHash(QQmlJSRegisterContent registerContent, size_t seed = 0)
    {
        return qHash(registerContent.d, seed);
    }

    QQmlJSRegisterContentPrivate *d = nullptr;
};

class Q_QMLCOMPILER_EXPORT QQmlJSRegisterContentPool
{
    Q_DISABLE_COPY_MOVE(QQmlJSRegisterContentPool)
public:
    using ContentVariant = QQmlJSRegisterContent::ContentVariant;

    QQmlJSRegisterContentPool();
    ~QQmlJSRegisterContentPool();


    // Create new register contents of specific kinds

    QQmlJSRegisterContent createType(
            const QQmlJSScope::ConstPtr &type, int resultLookupIndex, ContentVariant variant,
            QQmlJSRegisterContent scope = {});

    QQmlJSRegisterContent createProperty(
            const QQmlJSMetaProperty &property, int baseLookupIndex, int resultLookupIndex,
            ContentVariant variant, QQmlJSRegisterContent scope);

    QQmlJSRegisterContent createEnumeration(
            const QQmlJSMetaEnum &enumeration, const QString &enumMember, ContentVariant variant,
            QQmlJSRegisterContent scope);

    QQmlJSRegisterContent createMethod(
            const QList<QQmlJSMetaMethod> &methods, const QQmlJSScope::ConstPtr &methodType,
            ContentVariant variant, QQmlJSRegisterContent scope);

    QQmlJSRegisterContent createMethodCall(
            const QQmlJSMetaMethod &method, const QQmlJSScope::ConstPtr &returnType,
            QQmlJSRegisterContent scope);

    QQmlJSRegisterContent createImportNamespace(
            uint importNamespaceStringId, const QQmlJSScope::ConstPtr &importNamespaceType,
            ContentVariant variant, QQmlJSRegisterContent scope);

    QQmlJSRegisterContent createConversion(
            const QList<QQmlJSRegisterContent> &origins, const QQmlJSScope::ConstPtr &conversion,
            QQmlJSRegisterContent conversionScope, ContentVariant variant,
            QQmlJSRegisterContent scope);


    // Clone and possibly adapt register contents. This leaves the original intact.

    QQmlJSRegisterContent storedIn(
            QQmlJSRegisterContent content, const QQmlJSScope::ConstPtr &newStoredType);

    QQmlJSRegisterContent castTo(
            QQmlJSRegisterContent content, const QQmlJSScope::ConstPtr &newContainedType);

    QQmlJSRegisterContent clone(QQmlJSRegisterContent from) { return clone(from.d); }


    // Change the internals of the given register content. Adjusting and generalizing store a
    // copy of the previous content as original() and shadowed(), respectively. Storing creates
    // a new register content for storage(). All of those assume you only do this once per
    // register content (although you can adjust and generalize the storage, for example).

    void storeType(
            QQmlJSRegisterContent content, const QQmlJSScope::ConstPtr &stored);
    void adjustType(
            QQmlJSRegisterContent content, const QQmlJSScope::ConstPtr &adjusted);
    void generalizeType(
            QQmlJSRegisterContent content, const QQmlJSScope::ConstPtr &generalized);

    enum AllocationMode { Permanent, Temporary };
    void setAllocationMode(AllocationMode mode);
    void clearTemporaries();

private:
    struct Deleter {
        // It's a template so that we only need the QQmlJSRegisterContentPrivate dtor on usage.
        template<typename Private>
        constexpr void operator()(Private *d) const { delete d; }
    };

    using Pool = std::vector<std::unique_ptr<QQmlJSRegisterContentPrivate, Deleter>>;

    QQmlJSRegisterContentPrivate *clone(const QQmlJSRegisterContentPrivate *from);
    QQmlJSRegisterContentPrivate *create() { return clone(nullptr); }
    QQmlJSRegisterContentPrivate *create(QQmlJSRegisterContent scope, ContentVariant variant);

    Pool m_pool;
    qsizetype m_checkpoint = -1;
};

QT_END_NAMESPACE

#endif // REGISTERCONTENT_H
