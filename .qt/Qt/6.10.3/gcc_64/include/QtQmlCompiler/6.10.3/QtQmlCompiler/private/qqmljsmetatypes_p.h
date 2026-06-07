// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:significant

#ifndef QQMLJSMETATYPES_P_H
#define QQMLJSMETATYPES_P_H

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

#include <QtCore/qstring.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qsharedpointer.h>
#include <QtCore/qvariant.h>
#include <QtCore/qhash.h>

#include <QtQml/private/qqmljssourcelocation_p.h>
#include <QtQml/private/qqmltranslation_p.h>

#include "qqmlsaconstants.h"
#include "qqmlsa.h"
#include "qqmljsannotation_p.h"

// MetaMethod and MetaProperty have both type names and actual QQmlJSScope types.
// When parsing the information from the relevant QML or qmltypes files, we only
// see the names and don't have a complete picture of the types, yet. In a second
// pass we typically fill in the types. The types may have multiple exported names
// and the the name property of MetaProperty and MetaMethod still carries some
// significance regarding which name was chosen to refer to the type. In a third
// pass we may further specify the type if the context provides additional information.
// The parent of an Item, for example, is typically not just a QtObject, but rather
// some other Item with custom properties.

QT_BEGIN_NAMESPACE

enum ScriptBindingValueType : unsigned int {
    ScriptValue_Unknown,
    ScriptValue_Function, // binding to a function, arrow, lambda or a {}-block
    ScriptValue_Undefined // property int p: undefined
};

using QQmlJSMetaMethodType = QQmlSA::MethodType;

class QQmlJSTypeResolver;
class QQmlJSScope;
class QQmlJSMetaEnum
{
    QStringList m_keys;
    QList<int> m_values; // empty if values unknown.
    QString m_name;
    QString m_alias;
    QString m_typeName;
    QSharedPointer<const QQmlJSScope> m_type;
    bool m_isFlag = false;
    bool m_isScoped = false;
    bool m_isQml = false;

public:
    QQmlJSMetaEnum() = default;
    explicit QQmlJSMetaEnum(QString name) : m_name(std::move(name)) {}

    bool isValid() const { return !m_name.isEmpty(); }

    QString name() const { return m_name; }
    void setName(const QString &name) { m_name = name; }

    QString alias() const { return m_alias; }
    void setAlias(const QString &alias) { m_alias = alias; }

    bool isFlag() const { return m_isFlag; }
    void setIsFlag(bool isFlag) { m_isFlag = isFlag; }

    bool isScoped() const { return m_isScoped; }
    void setIsScoped(bool v) { m_isScoped = v; }

    bool isQml() const { return m_isQml; }
    void setIsQml(bool v) { m_isQml = v; }

    void addKey(const QString &key) { m_keys.append(key); }
    QStringList keys() const { return m_keys; }

    void addValue(int value) { m_values.append(value); }
    QList<int> values() const { return m_values; }

    bool hasValues() const { return !m_values.isEmpty(); }
    int value(const QString &key) const { return m_values.value(m_keys.indexOf(key)); }
    bool hasKey(const QString &key) const { return m_keys.indexOf(key) != -1; }

    QString typeName() const { return m_typeName; }
    void setTypeName(const QString &typeName) { m_typeName = typeName; }

    QSharedPointer<const QQmlJSScope> type() const { return m_type; }
    void setType(const QSharedPointer<const QQmlJSScope> &type) { m_type = type; }

    friend bool operator==(const QQmlJSMetaEnum &a, const QQmlJSMetaEnum &b)
    {
        return a.m_keys == b.m_keys
                && a.m_values == b.m_values
                && a.m_name == b.m_name
                && a.m_alias == b.m_alias
                && a.m_isFlag == b.m_isFlag
                && a.m_type == b.m_type
                && a.m_isScoped == b.m_isScoped;
    }

    friend bool operator!=(const QQmlJSMetaEnum &a, const QQmlJSMetaEnum &b)
    {
        return !(a == b);
    }

    friend size_t qHash(const QQmlJSMetaEnum &e, size_t seed = 0)
    {
        return qHashMulti(
                seed, e.m_keys, e.m_values, e.m_name, e.m_alias, e.m_isFlag, e.m_type, e.m_isScoped);
    }
};

class QQmlJSMetaParameter
{
public:
    /*!
       \internal
       A non-const parameter is passed either by pointer or by value, depending on its access
       semantics. For types with reference access semantics, they can be const and will be passed
       then as const pointer. Const references are treated like values (i.e. non-const).
     */
    enum Constness {
        NonConst = 0,
        Const,
    };

    QQmlJSMetaParameter(QString name = QString(), QString typeName = QString(),
                        Constness typeQualifier = NonConst,
                        QWeakPointer<const QQmlJSScope> type = {})
        : m_name(std::move(name)),
          m_typeName(std::move(typeName)),
          m_type(type),
          m_typeQualifier(typeQualifier)
    {
    }

    QString name() const { return m_name; }
    void setName(const QString &name) { m_name = name; }
    QString typeName() const { return m_typeName; }
    void setTypeName(const QString &typeName) { m_typeName = typeName; }
    QSharedPointer<const QQmlJSScope> type() const { return m_type.toStrongRef(); }
    void setType(QWeakPointer<const QQmlJSScope> type) { m_type = type; }
    Constness typeQualifier() const { return m_typeQualifier; }
    void setTypeQualifier(Constness typeQualifier) { m_typeQualifier = typeQualifier; }
    bool isPointer() const { return m_isPointer; }
    void setIsPointer(bool isPointer) { m_isPointer = isPointer; }
    bool isList() const { return m_isList; }
    void setIsList(bool isList) { m_isList = isList; }

    friend bool operator==(const QQmlJSMetaParameter &a, const QQmlJSMetaParameter &b)
    {
        return a.m_name == b.m_name && a.m_typeName == b.m_typeName
                && a.m_type.owner_equal(b.m_type)
                && a.m_typeQualifier == b.m_typeQualifier;
    }

    friend bool operator!=(const QQmlJSMetaParameter &a, const QQmlJSMetaParameter &b)
    {
        return !(a == b);
    }

    friend size_t qHash(const QQmlJSMetaParameter &e, size_t seed = 0)
    {
        return qHashMulti(seed, e.m_name, e.m_typeName, e.m_type.owner_hash(),
                          e.m_typeQualifier);
    }

private:
    QString m_name;
    QString m_typeName;
    QWeakPointer<const QQmlJSScope> m_type;
    Constness m_typeQualifier = NonConst;
    bool m_isPointer = false;
    bool m_isList = false;
};

using QQmlJSMetaReturnType = QQmlJSMetaParameter;

class QQmlJSMetaMethod
{
public:
    enum Access { Private, Protected, Public };
    using MethodType = QQmlJSMetaMethodType;

public:
    /*! \internal

        Represents a relative JavaScript function/expression index within a type
        in a QML document. Used as a typed alternative to int with an explicit
        invalid state.
    */
    enum class RelativeFunctionIndex : int { Invalid = -1 };

    /*! \internal

        Represents an absolute JavaScript function/expression index pointing
        into the QV4::ExecutableCompilationUnit::runtimeFunctions array. Used as
        a typed alternative to int with an explicit invalid state.
    */
    enum class AbsoluteFunctionIndex : int { Invalid = -1 };

    QQmlJSMetaMethod() = default;
    explicit QQmlJSMetaMethod(QString name, QString returnType = QString())
        : m_name(std::move(name)),
          m_returnType(QString(), std::move(returnType)),
          m_methodType(MethodType::Method)
    {}

    QString methodName() const { return m_name; }
    void setMethodName(const QString &name) { m_name = name; }

    QQmlJS::SourceLocation sourceLocation() const { return m_sourceLocation; }
    void setSourceLocation(QQmlJS::SourceLocation location) { m_sourceLocation = location; }

    QQmlJSMetaReturnType returnValue() const { return m_returnType; }
    void setReturnValue(const QQmlJSMetaReturnType returnValue) { m_returnType = returnValue; }
    QString returnTypeName() const { return m_returnType.typeName(); }
    void setReturnTypeName(const QString &typeName) { m_returnType.setTypeName(typeName); }
    QSharedPointer<const QQmlJSScope> returnType() const { return m_returnType.type(); }
    void setReturnType(QWeakPointer<const QQmlJSScope> type) { m_returnType.setType(type); }

    QList<QQmlJSMetaParameter> parameters() const { return m_parameters; }
    std::pair<QList<QQmlJSMetaParameter>::iterator, QList<QQmlJSMetaParameter>::iterator>
    mutableParametersRange()
    {
        return { m_parameters.begin(), m_parameters.end() };
    }

    QStringList parameterNames() const
    {
        QStringList names;
        for (const auto &p : m_parameters)
            names.append(p.name());

        return names;
    }

    void setParameters(const QList<QQmlJSMetaParameter> &parameters) { m_parameters = parameters; }

    void addParameter(const QQmlJSMetaParameter &p) { m_parameters.append(p); }

    QQmlJSMetaMethodType methodType() const { return m_methodType; }
    void setMethodType(MethodType methodType) { m_methodType = methodType; }

    Access access() const { return m_methodAccess; }

    int revision() const { return m_revision; }
    void setRevision(int r) { m_revision = r; }

    bool isCloned() const { return m_isCloned; }
    void setIsCloned(bool isCloned) { m_isCloned= isCloned; }

    bool isConstructor() const { return m_isConstructor; }
    void setIsConstructor(bool isConstructor) { m_isConstructor = isConstructor; }

    bool isJavaScriptFunction() const { return m_isJavaScriptFunction; }
    void setIsJavaScriptFunction(bool isJavaScriptFunction)
    {
        m_isJavaScriptFunction = isJavaScriptFunction;
    }

    bool isImplicitQmlPropertyChangeSignal() const { return m_isImplicitQmlPropertyChangeSignal; }
    void setIsImplicitQmlPropertyChangeSignal(bool isPropertyChangeSignal)
    {
        m_isImplicitQmlPropertyChangeSignal = isPropertyChangeSignal;
    }

    bool isConst() const {  return m_isConst; }
    void setIsConst(bool isConst) { m_isConst = isConst; }

    bool isValid() const { return !m_name.isEmpty(); }

    const QVector<QQmlJSAnnotation>& annotations() const { return m_annotations; }
    void setAnnotations(QVector<QQmlJSAnnotation> annotations) { m_annotations = annotations; }

    void setJsFunctionIndex(RelativeFunctionIndex index)
    {
        Q_ASSERT(!m_isConstructor);
        m_relativeFunctionIndex = index;
    }

    RelativeFunctionIndex jsFunctionIndex() const
    {
        Q_ASSERT(!m_isConstructor);
        return m_relativeFunctionIndex;
    }

    void setConstructorIndex(RelativeFunctionIndex index)
    {
        Q_ASSERT(m_isConstructor);
        m_relativeFunctionIndex = index;
    }

    RelativeFunctionIndex constructorIndex() const
    {
        Q_ASSERT(m_isConstructor);
        return m_relativeFunctionIndex;
    }

    void setMethodIndex(RelativeFunctionIndex index)
    {
        Q_ASSERT(!m_isConstructor);
        m_relativeFunctionIndex = index;
    }

    RelativeFunctionIndex methodIndex() const
    {
        Q_ASSERT(!m_isConstructor);
        return m_relativeFunctionIndex;
    }

    friend bool operator==(const QQmlJSMetaMethod &a, const QQmlJSMetaMethod &b)
    {
        return a.m_name == b.m_name && a.m_sourceLocation == b.m_sourceLocation
                && a.m_returnType == b.m_returnType && a.m_parameters == b.m_parameters
                && a.m_annotations == b.m_annotations && a.m_methodType == b.m_methodType
                && a.m_methodAccess == b.m_methodAccess && a.m_revision == b.m_revision
                && a.m_relativeFunctionIndex == b.m_relativeFunctionIndex
                && a.m_isCloned == b.m_isCloned && a.m_isConstructor == b.m_isConstructor
                && a.m_isJavaScriptFunction == b.m_isJavaScriptFunction
                && a.m_isImplicitQmlPropertyChangeSignal == b.m_isImplicitQmlPropertyChangeSignal
                && a.m_isConst == b.m_isConst;
    }

    friend bool operator!=(const QQmlJSMetaMethod &a, const QQmlJSMetaMethod &b)
    {
        return !(a == b);
    }

    friend size_t qHash(const QQmlJSMetaMethod &method, size_t seed = 0)
    {
        QtPrivate::QHashCombine combine(seed);

        seed = combine(seed, method.m_name);
        seed = combine(seed, method.m_sourceLocation);
        seed = combine(seed, method.m_returnType);
        seed = combine(seed, method.m_parameters);
        seed = combine(seed, method.m_annotations);
        seed = combine(seed, method.m_methodType);
        seed = combine(seed, method.m_methodAccess);
        seed = combine(seed, method.m_revision);
        seed = combine(seed, method.m_relativeFunctionIndex);
        seed = combine(seed, method.m_isCloned);
        seed = combine(seed, method.m_isConstructor);
        seed = combine(seed, method.m_isJavaScriptFunction);
        seed = combine(seed, method.m_isImplicitQmlPropertyChangeSignal);
        seed = combine(seed, method.m_isConst);

        return seed;
    }

private:
    QString m_name;

    QQmlJS::SourceLocation m_sourceLocation;

    QQmlJSMetaReturnType m_returnType;
    QList<QQmlJSMetaParameter> m_parameters;
    QList<QQmlJSAnnotation> m_annotations;

    MethodType m_methodType = MethodType::Signal;
    Access m_methodAccess = Public;
    int m_revision = 0;
    RelativeFunctionIndex m_relativeFunctionIndex = RelativeFunctionIndex::Invalid;
    bool m_isCloned = false;
    bool m_isConstructor = false;
    bool m_isJavaScriptFunction = false;
    bool m_isImplicitQmlPropertyChangeSignal = false;
    bool m_isConst = false;
};

class QQmlJSMetaProperty
{
    QString m_propertyName;
    QString m_typeName;
    QString m_read;
    QString m_write;
    QString m_reset;
    QString m_bindable;
    QString m_notify;
    QString m_privateClass;
    QString m_aliasExpr;
    QString m_aliasTargetName;
    QWeakPointer<const QQmlJSScope> m_aliasTargetScope;
    QWeakPointer<const QQmlJSScope> m_type;
    QQmlJS::SourceLocation m_sourceLocation;
    QVector<QQmlJSAnnotation> m_annotations;
    bool m_isList = false;
    bool m_isWritable = false;
    bool m_isPointer = false;
    bool m_isTypeConstant = false;
    bool m_isFinal = false;
    bool m_isPropertyConstant = false;
    int m_revision = 0;
    int m_index = -1; // relative property index within owning QQmlJSScope

public:
    QQmlJSMetaProperty() = default;

    void setPropertyName(const QString &propertyName) { m_propertyName = propertyName; }
    QString propertyName() const { return m_propertyName; }

    void setTypeName(const QString &typeName) { m_typeName = typeName; }
    QString typeName() const { return m_typeName; }

    void setRead(const QString &read) { m_read = read; }
    QString read() const { return m_read; }

    void setWrite(const QString &write) { m_write = write; }
    QString write() const { return m_write; }

    void setReset(const QString &reset) { m_reset = reset; }
    QString reset() const { return m_reset; }

    void setBindable(const QString &bindable) { m_bindable = bindable; }
    QString bindable() const { return m_bindable; }

    void setNotify(const QString &notify) { m_notify = notify; }
    QString notify() const { return m_notify; }

    void setPrivateClass(const QString &privateClass) { m_privateClass = privateClass; }
    QString privateClass() const { return m_privateClass; }
    bool isPrivate() const { return !m_privateClass.isEmpty(); } // exists for convenience

    void setType(const QSharedPointer<const QQmlJSScope> &type) { m_type = type; }
    QSharedPointer<const QQmlJSScope> type() const { return m_type.toStrongRef(); }

    void setSourceLocation(const QQmlJS::SourceLocation &newSourceLocation)
    { m_sourceLocation = newSourceLocation; }
    QQmlJS::SourceLocation sourceLocation() const { return m_sourceLocation; }

    void setAnnotations(const QList<QQmlJSAnnotation> &annotation) { m_annotations = annotation; }
    const QList<QQmlJSAnnotation> &annotations() const { return m_annotations; }

    void setIsList(bool isList) { m_isList = isList; }
    bool isList() const { return m_isList; }

    void setIsWritable(bool isWritable) { m_isWritable = isWritable; }
    bool isWritable() const { return m_isWritable; }

    void setIsPointer(bool isPointer) { m_isPointer = isPointer; }
    bool isPointer() const { return m_isPointer; }

    void setIsTypeConstant(bool isTypeConstant) { m_isTypeConstant = isTypeConstant; }
    bool isTypeConstant() const { return m_isTypeConstant; }

    void setAliasExpression(const QString &aliasString) { m_aliasExpr = aliasString; }
    QString aliasExpression() const { return m_aliasExpr; }
    bool isAlias() const { return !m_aliasExpr.isEmpty(); } // exists for convenience

    void setAliasTargetName(const QString &name) { m_aliasTargetName = name; }
    QString aliasTargetName() const { return m_aliasTargetName; }

    void setAliasTargetScope(const QSharedPointer<const QQmlJSScope> &scope)
    {
        m_aliasTargetScope = scope;
    }
    QSharedPointer<const QQmlJSScope> aliasTargetScope() const
    {
        return m_aliasTargetScope.toStrongRef();
    }

    void setIsFinal(bool isFinal) { m_isFinal = isFinal; }
    bool isFinal() const { return m_isFinal; }

    void setIsPropertyConstant(bool isPropertyConstant) { m_isPropertyConstant = isPropertyConstant; }
    bool isPropertyConstant() const { return m_isPropertyConstant; }

    void setRevision(int revision) { m_revision = revision; }
    int revision() const { return m_revision; }

    void setIndex(int index) { m_index = index; }
    int index() const { return m_index; }

    bool isValid() const { return !m_propertyName.isEmpty(); }

    friend bool operator==(const QQmlJSMetaProperty &a, const QQmlJSMetaProperty &b)
    {
        return a.m_index == b.m_index && a.m_propertyName == b.m_propertyName
                && a.m_typeName == b.m_typeName && a.m_bindable == b.m_bindable
                && a.m_type.owner_equal(b.m_type) && a.m_isList == b.m_isList
                && a.m_isWritable == b.m_isWritable && a.m_isPointer == b.m_isPointer
                && a.m_aliasExpr == b.m_aliasExpr && a.m_revision == b.m_revision
                && a.m_isFinal == b.m_isFinal;
    }

    friend bool operator!=(const QQmlJSMetaProperty &a, const QQmlJSMetaProperty &b)
    {
        return !(a == b);
    }

    friend size_t qHash(const QQmlJSMetaProperty &prop, size_t seed = 0)
    {
        return qHashMulti(seed, prop.m_propertyName, prop.m_typeName, prop.m_bindable,
                          prop.m_type.toStrongRef().data(), prop.m_isList, prop.m_isWritable,
                          prop.m_isPointer, prop.m_aliasExpr, prop.m_revision, prop.m_isFinal,
                          prop.m_index);
    }
};

class Q_QMLCOMPILER_EXPORT QQmlJSMetaPropertyBinding
{
    using BindingType = QQmlSA::BindingType;
    using ScriptBindingKind = QQmlSA::ScriptBindingKind;

    // needs to be kept in sync with the BindingType enum
    struct Content {
        using Invalid = std::monostate;
        struct BoolLiteral {
            bool value;
            friend bool operator==(BoolLiteral a, BoolLiteral b) { return a.value == b.value; }
            friend bool operator!=(BoolLiteral a, BoolLiteral b) { return !(a == b); }
        };
        struct NumberLiteral {
            QT_WARNING_PUSH
            QT_WARNING_DISABLE_CLANG("-Wfloat-equal")
            QT_WARNING_DISABLE_GCC("-Wfloat-equal")
            friend bool operator==(NumberLiteral a, NumberLiteral b) { return a.value == b.value; }
            friend bool operator!=(NumberLiteral a, NumberLiteral b) { return !(a == b); }
            QT_WARNING_POP

            double value; // ### TODO: int?
        };
        struct StringLiteral {
            friend bool operator==(StringLiteral a, StringLiteral b) { return a.value == b.value; }
            friend bool operator!=(StringLiteral a, StringLiteral b) { return !(a == b); }
            QString value;
        };
        struct RegexpLiteral {
            friend bool operator==(RegexpLiteral a, RegexpLiteral b) { return a.value == b.value; }
            friend bool operator!=(RegexpLiteral a, RegexpLiteral b) { return !(a == b); }
            QString value;
        };
        struct Null {
            friend bool operator==(Null , Null ) { return true; }
            friend bool operator!=(Null a, Null b) { return !(a == b); }
        };
        struct TranslationString {
            friend bool operator==(TranslationString a, TranslationString b)
            {
                return a.text == b.text && a.comment == b.comment && a.number == b.number && a.context == b.context;
            }
            friend bool operator!=(TranslationString a, TranslationString b) { return !(a == b); }
            QString text;
            QString comment;
            QString context;
            int number;
        };
        struct TranslationById {
            friend bool operator==(TranslationById a, TranslationById b)
            {
                return a.id == b.id && a.number == b.number;
            }
            friend bool operator!=(TranslationById a, TranslationById b) { return !(a == b); }
            QString id;
            int number;
        };
        struct Script {
            friend bool operator==(Script a, Script b)
            {
                return a.index == b.index && a.kind == b.kind;
            }
            friend bool operator!=(Script a, Script b) { return !(a == b); }
            QQmlJSMetaMethod::RelativeFunctionIndex index =
                    QQmlJSMetaMethod::RelativeFunctionIndex::Invalid;
            ScriptBindingKind kind = ScriptBindingKind::Invalid;
            ScriptBindingValueType valueType = ScriptBindingValueType::ScriptValue_Unknown;
        };
        struct Object {
            friend bool operator==(Object a, Object b) { return a.value.owner_equal(b.value) && a.typeName == b.typeName; }
            friend bool operator!=(Object a, Object b) { return !(a == b); }
            QString typeName;
            QWeakPointer<const QQmlJSScope> value;
        };
        struct Interceptor {
            friend bool operator==(Interceptor a, Interceptor b)
            {
                return a.value.owner_equal(b.value) && a.typeName == b.typeName;
            }
            friend bool operator!=(Interceptor a, Interceptor b) { return !(a == b); }
            QString typeName;
            QWeakPointer<const QQmlJSScope> value;
        };
        struct ValueSource {
            friend bool operator==(ValueSource a, ValueSource b)
            {
                return a.value.owner_equal(b.value) && a.typeName == b.typeName;
            }
            friend bool operator!=(ValueSource a, ValueSource b) { return !(a == b); }
            QString typeName;
            QWeakPointer<const QQmlJSScope> value;
        };
        struct AttachedProperty {
            /*
                AttachedProperty binding is a grouping for a series of bindings
                belonging to the same scope(QQmlJSScope::AttachedPropertyScope).
                Thus, the attached property binding itself only exposes the
                attaching type object. Such object is unique per the enclosing
                scope, so attaching types attached to different QML scopes are
                different (think of them as objects in C++ terms).

                An attaching type object, being a QQmlJSScope, has bindings
                itself. For instance:
                ```
                Type {
                    Keys.enabled: true
                }
                ```
                tells us that "Type" has an AttachedProperty binding with
                property name "Keys". The attaching object of that binding
                (binding.attachedType()) has type "Keys" and a BoolLiteral
                binding with property name "enabled".
            */
            friend bool operator==(AttachedProperty a, AttachedProperty b)
            {
                return a.value.owner_equal(b.value);
            }
            friend bool operator!=(AttachedProperty a, AttachedProperty b) { return !(a == b); }
            QWeakPointer<const QQmlJSScope> value;
        };
        struct GroupProperty {
            /* Given a group property declaration like
               anchors.left: root.left
               the QQmlJSMetaPropertyBinding will have name "anchors", and a m_bindingContent
               of type GroupProperty, with groupScope pointing to the scope introudced by anchors
               In that scope, there will be another QQmlJSMetaPropertyBinding, with name "left" and
               m_bindingContent Script (for root.left).
               There should never be more than one GroupProperty for the same name in the same
               scope, though: If the scope also contains anchors.top: root.top that should reuse the
               GroupProperty content (and add a top: root.top binding in it). There might however
               still be an additional object or script binding ( anchors: {left: foo, right: bar };
               anchors: root.someFunction() ) or another binding to the property in a "derived"
               type.

               ### TODO: Obtaining the effective binding result requires some resolving function
            */
            QWeakPointer<const QQmlJSScope> groupScope;
            friend bool operator==(GroupProperty a, GroupProperty b) { return a.groupScope.owner_equal(b.groupScope); }
            friend bool operator!=(GroupProperty a, GroupProperty b) { return !(a == b); }
        };
        using type = std::variant<Invalid, BoolLiteral, NumberLiteral, StringLiteral,
                                  RegexpLiteral, Null, TranslationString,
                                  TranslationById, Script, Object, Interceptor,
                                  ValueSource, AttachedProperty, GroupProperty
                                 >;
    };
    using BindingContent = Content::type;

    QQmlJS::SourceLocation m_sourceLocation;
    QString m_propertyName; // TODO: this is a debug-only information
    BindingContent m_bindingContent;

    void ensureSetBindingTypeOnce()
    {
        Q_ASSERT(bindingType() == BindingType::Invalid);
    }

    bool isLiteralBinding() const { return isLiteralBinding(bindingType()); }


public:
    static bool isLiteralBinding(BindingType type)
    {
        return type == BindingType::BoolLiteral || type == BindingType::NumberLiteral
                || type == BindingType::StringLiteral || type == BindingType::RegExpLiteral
                || type == BindingType::Null; // special. we record it as literal
    }

    QQmlJSMetaPropertyBinding();
    QQmlJSMetaPropertyBinding(QQmlJS::SourceLocation location) : m_sourceLocation(location) { }
    explicit QQmlJSMetaPropertyBinding(QQmlJS::SourceLocation location, const QString &propName)
        : m_sourceLocation(location), m_propertyName(propName)
    {
    }
    explicit QQmlJSMetaPropertyBinding(QQmlJS::SourceLocation location,
                                       const QQmlJSMetaProperty &prop)
        : QQmlJSMetaPropertyBinding(location, prop.propertyName())
    {
    }

    void setPropertyName(const QString &propertyName) { m_propertyName = propertyName; }
    QString propertyName() const { return m_propertyName; }

    const QQmlJS::SourceLocation &sourceLocation() const { return m_sourceLocation; }

    BindingType bindingType() const { return BindingType(m_bindingContent.index()); }

    bool isValid() const;

    void setStringLiteral(QAnyStringView value)
    {
        ensureSetBindingTypeOnce();
        m_bindingContent = Content::StringLiteral { value.toString() };
    }

    void
    setScriptBinding(QQmlJSMetaMethod::RelativeFunctionIndex value, ScriptBindingKind kind,
                     ScriptBindingValueType valueType = ScriptBindingValueType::ScriptValue_Unknown)
    {
        ensureSetBindingTypeOnce();
        m_bindingContent = Content::Script { value, kind, valueType };
    }

    void setGroupBinding(const QSharedPointer<const QQmlJSScope> &groupScope)
    {
        ensureSetBindingTypeOnce();
        m_bindingContent = Content::GroupProperty { groupScope };
    }

    void setAttachedBinding(const QSharedPointer<const QQmlJSScope> &attachingScope)
    {
        ensureSetBindingTypeOnce();
        m_bindingContent = Content::AttachedProperty { attachingScope };
    }

    void setBoolLiteral(bool value)
    {
        ensureSetBindingTypeOnce();
        m_bindingContent = Content::BoolLiteral { value };
    }

    void setNullLiteral()
    {
        ensureSetBindingTypeOnce();
        m_bindingContent = Content::Null {};
    }

    void setNumberLiteral(double value)
    {
        ensureSetBindingTypeOnce();
        m_bindingContent = Content::NumberLiteral { value };
    }

    void setRegexpLiteral(QAnyStringView value)
    {
        ensureSetBindingTypeOnce();
        m_bindingContent = Content::RegexpLiteral { value.toString() };
    }

    void setTranslation(QStringView text, QStringView comment, QStringView context, int number)
    {
        ensureSetBindingTypeOnce();
        m_bindingContent =
                Content::TranslationString{ text.toString(), comment.toString(), context.toString(), number };
    }

    void setTranslationId(QStringView id, int number)
    {
        ensureSetBindingTypeOnce();
        m_bindingContent = Content::TranslationById{ id.toString(), number };
    }

    void setObject(const QString &typeName, const QSharedPointer<const QQmlJSScope> &type)
    {
        ensureSetBindingTypeOnce();
        m_bindingContent = Content::Object { typeName, type };
    }

    void setInterceptor(const QString &typeName, const QSharedPointer<const QQmlJSScope> &type)
    {
        ensureSetBindingTypeOnce();
        m_bindingContent = Content::Interceptor { typeName, type };
    }

    void setValueSource(const QString &typeName, const QSharedPointer<const QQmlJSScope> &type)
    {
        ensureSetBindingTypeOnce();
        m_bindingContent = Content::ValueSource { typeName, type };
    }

    // ### TODO: here and below: Introduce an allowConversion parameter, if yes, enable conversions e.g. bool -> number?
    bool boolValue() const;

    double numberValue() const;

    QString stringValue() const;

    QString regExpValue() const;

    QQmlTranslation translationDataValue(QString qmlFileNameForContext = QString()) const;

    QSharedPointer<const QQmlJSScope> literalType(const QQmlJSTypeResolver *resolver) const;

    QQmlJSMetaMethod::RelativeFunctionIndex scriptIndex() const
    {
        if (auto *script = std::get_if<Content::Script>(&m_bindingContent))
            return script->index;
        // warn
        return QQmlJSMetaMethod::RelativeFunctionIndex::Invalid;
    }

    ScriptBindingKind scriptKind() const
    {
        if (auto *script = std::get_if<Content::Script>(&m_bindingContent))
            return script->kind;
        // warn
        return ScriptBindingKind::Invalid;
    }

    ScriptBindingValueType scriptValueType() const
    {
        if (auto *script = std::get_if<Content::Script>(&m_bindingContent))
            return script->valueType;
        // warn
        return ScriptBindingValueType::ScriptValue_Unknown;
    }

    QString objectTypeName() const
    {
        if (auto *object = std::get_if<Content::Object>(&m_bindingContent))
            return object->typeName;
        // warn
        return {};
    }
    QSharedPointer<const QQmlJSScope> objectType() const
    {
        if (auto *object = std::get_if<Content::Object>(&m_bindingContent))
            return object->value.lock();
        // warn
        return {};
    }

    QString interceptorTypeName() const
    {
        if (auto *interceptor = std::get_if<Content::Interceptor>(&m_bindingContent))
            return interceptor->typeName;
        // warn
        return {};
    }
    QSharedPointer<const QQmlJSScope> interceptorType() const
    {
        if (auto *interceptor = std::get_if<Content::Interceptor>(&m_bindingContent))
            return interceptor->value.lock();
        // warn
        return {};
    }

    QString valueSourceTypeName() const
    {
        if (auto *valueSource = std::get_if<Content::ValueSource>(&m_bindingContent))
            return valueSource->typeName;
        // warn
        return {};
    }
    QSharedPointer<const QQmlJSScope> valueSourceType() const
    {
        if (auto *valueSource = std::get_if<Content::ValueSource>(&m_bindingContent))
            return valueSource->value.lock();
        // warn
        return {};
    }

    QSharedPointer<const QQmlJSScope> groupType() const
    {
        if (auto *group = std::get_if<Content::GroupProperty>(&m_bindingContent))
            return group->groupScope.lock();
        // warn
        return {};
    }

    QSharedPointer<const QQmlJSScope> attachedType() const
    {
        if (auto *attached = std::get_if<Content::AttachedProperty>(&m_bindingContent))
            return attached->value.lock();
        // warn
        return {};
    }

    bool hasLiteral() const
    {
        // TODO: Assumption: if the type is literal, we must have one
        return isLiteralBinding();
    }
    bool hasObject() const { return bindingType() == BindingType::Object; }
    bool hasInterceptor() const
    {
        return bindingType() == BindingType::Interceptor;
    }
    bool hasValueSource() const
    {
        return bindingType() == BindingType::ValueSource;
    }

    friend bool operator==(const QQmlJSMetaPropertyBinding &a, const QQmlJSMetaPropertyBinding &b)
    {
        return a.m_propertyName == b.m_propertyName
                &&  a.m_bindingContent == b.m_bindingContent
                &&  a.m_sourceLocation == b.m_sourceLocation;
    }

    friend bool operator!=(const QQmlJSMetaPropertyBinding &a, const QQmlJSMetaPropertyBinding &b)
    {
        return !(a == b);
    }

    friend size_t qHash(const QQmlJSMetaPropertyBinding &binding, size_t seed = 0)
    {
        // we don't need to care about the actual binding content when hashing
        return qHashMulti(seed, binding.m_propertyName, binding.m_sourceLocation,
                          binding.bindingType());
    }
};

struct Q_QMLCOMPILER_EXPORT QQmlJSMetaSignalHandler
{
    QStringList signalParameters;
    bool isMultiline;
};

QT_END_NAMESPACE

#endif // QQMLJSMETATYPES_P_H
