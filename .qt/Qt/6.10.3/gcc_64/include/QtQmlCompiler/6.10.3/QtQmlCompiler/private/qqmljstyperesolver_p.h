// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:significant

#ifndef QQMLJSTYPERESOLVER_P_H
#define QQMLJSTYPERESOLVER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#include <memory>
#include <qtqmlcompilerexports.h>

#include <private/qqmlirbuilder_p.h>
#include <private/qqmljsast_p.h>
#include "qqmljsimporter_p.h"
#include "qqmljslogger_p.h"
#include "qqmljsregistercontent_p.h"
#include "qqmljsresourcefilemapper_p.h"
#include "qqmljsscope_p.h"
#include "qqmljsscopesbyid_p.h"

QT_BEGIN_NAMESPACE

class QQmlJSImportVisitor;
class Q_QMLCOMPILER_EXPORT QQmlJSTypeResolver
{
public:
    QQmlJSTypeResolver(QQmlJSImporter *importer);

    // Note: must be called after the construction to read the QML program
    void init(QQmlJSImportVisitor *visitor, QQmlJS::AST::Node *program);

    QQmlJSRegisterContentPool *registerContentPool() const { return m_pool.get(); }
    QQmlJSLogger *logger() const { return m_logger; }


    // Configuration options

    enum ParentMode { UseDocumentParent, UseParentProperty };
    void setParentMode(ParentMode mode) { m_parentMode = mode; }
    ParentMode parentMode() const { return m_parentMode; }

    enum CloneMode { CloneTypes, DoNotCloneTypes };
    void setCloneMode(CloneMode mode) { m_cloneMode = mode; }
    CloneMode cloneMode() const { return m_cloneMode; }


    // Builtin types

    QQmlJSScope::ConstPtr voidType() const { return m_voidType; }
    QQmlJSScope::ConstPtr emptyType() const { return m_emptyType; }
    QQmlJSScope::ConstPtr nullType() const { return m_nullType; }
    QQmlJSScope::ConstPtr realType() const { return m_realType; }
    QQmlJSScope::ConstPtr floatType() const { return m_floatType; }
    QQmlJSScope::ConstPtr int8Type() const { return m_int8Type; }
    QQmlJSScope::ConstPtr uint8Type() const { return m_uint8Type; }
    QQmlJSScope::ConstPtr int16Type() const { return m_int16Type; }
    QQmlJSScope::ConstPtr uint16Type() const { return m_uint16Type; }
    QQmlJSScope::ConstPtr int32Type() const { return m_int32Type; }
    QQmlJSScope::ConstPtr uint32Type() const { return m_uint32Type; }
    QQmlJSScope::ConstPtr int64Type() const { return m_int64Type; }
    QQmlJSScope::ConstPtr uint64Type() const { return m_uint64Type; }
    QQmlJSScope::ConstPtr sizeType() const { return m_sizeType; }
    QQmlJSScope::ConstPtr boolType() const { return m_boolType; }
    QQmlJSScope::ConstPtr stringType() const { return m_stringType; }
    QQmlJSScope::ConstPtr stringListType() const { return m_stringListType; }
    QQmlJSScope::ConstPtr byteArrayType() const { return m_byteArrayType; }
    QQmlJSScope::ConstPtr urlType() const { return m_urlType; }
    QQmlJSScope::ConstPtr dateTimeType() const { return m_dateTimeType; }
    QQmlJSScope::ConstPtr dateType() const { return m_dateType; }
    QQmlJSScope::ConstPtr timeType() const { return m_timeType; }
    QQmlJSScope::ConstPtr regexpType() const { return m_regexpType; }
    QQmlJSScope::ConstPtr variantListType() const { return m_variantListType; }
    QQmlJSScope::ConstPtr variantMapType() const { return m_variantMapType; }
    QQmlJSScope::ConstPtr varType() const { return m_varType; }
    QQmlJSScope::ConstPtr qmlPropertyMapType() const { return m_qmlPropertyMapType; }
    QQmlJSScope::ConstPtr jsValueType() const { return m_jsValueType; }
    QQmlJSScope::ConstPtr jsPrimitiveType() const { return m_jsPrimitiveType; }
    QQmlJSScope::ConstPtr listPropertyType() const { return m_listPropertyType; }
    QQmlJSScope::ConstPtr metaObjectType() const { return m_metaObjectType; }
    QQmlJSScope::ConstPtr functionType() const { return m_functionType; }
    QQmlJSScope::ConstPtr jsGlobalObject() const { return m_jsGlobalObject; }
    QQmlJSScope::ConstPtr qObjectType() const { return m_qObjectType; }
    QQmlJSScope::ConstPtr qObjectListType() const { return m_qObjectListType; }
    QQmlJSScope::ConstPtr arrayPrototype() const { return m_arrayPrototype; }
    QQmlJSScope::ConstPtr forInIteratorPtr() const { return m_forInIteratorPtr; }
    QQmlJSScope::ConstPtr forOfIteratorPtr() const { return m_forOfIteratorPtr; }
    QQmlJSScope::ConstPtr qQmlScriptStringType() const { return m_qQmlScriptStringType; }

    QQmlJSRegisterContent jsGlobalObjectContent() const { return m_jsGlobalObjectContent; }
    QQmlJSScope::ConstPtr mathObject() const;
    QQmlJSScope::ConstPtr consoleObject() const;

    QQmlJSScope::ConstPtr typeForConst(QV4::ReturnedValue rv) const;


    // Querying imports and imported types

    bool isPrefix(const QString &name) const
    {
        return m_imports.hasType(name) && !m_imports.type(name).scope;
    }

    const QHash<QString, QQmlJS::ImportedScope<QQmlJSScope::ConstPtr>> &importedTypes() const
    {
        return m_imports.types();
    }

    const auto &importedNames() const
    {
        return m_imports.contextualTypes().names();
    }

    QQmlJSScope::ConstPtr typeForName(const QString &name) const
    {
        return m_imports.type(name).scope;
    }

    QString nameForType(const QQmlJSScope::ConstPtr &type) const
    {
        return m_imports.name(type);
    }

    QStringList seenModuleQualifiers() const { return m_seenModuleQualifiers; }


    // Querying types from current document

    QQmlJSScope::ConstPtr scopeForLocation(const QV4::CompiledData::Location &location) const;
    QQmlJSScope::ConstPtr typeFromAST(QQmlJS::AST::Type *type) const;
    QQmlJSScope::ConstPtr typeForId(
            const QQmlJSScope::ConstPtr &scope, const QString &name,
            QQmlJSScopesByIdOptions options = Default) const
    {
        return m_objectsById.scope(name, scope, options);
    }

    const QQmlJSScopesById &objectsById() const { return m_objectsById; }
    bool canCallJSFunctions() const { return m_objectsById.signaturesAreEnforced(); }
    bool canAddressValueTypes() const { return m_objectsById.valueTypesAreAddressable(); }

    QQmlJSScope::ConstPtr scopedType(
            const QQmlJSScope::ConstPtr &scope, const QString &name,
            QQmlJSScopesByIdOptions options = Default) const;

    const QHash<QQmlJS::SourceLocation, QQmlJSMetaSignalHandler> &signalHandlers() const
    {
        return m_signalHandlers;
    }


    // Classification of types

    bool isPrimitive(QQmlJSRegisterContent type) const;
    bool isPrimitive(const QQmlJSScope::ConstPtr &type) const;

    bool isNumeric(QQmlJSRegisterContent type) const;
    bool isNumeric(const QQmlJSScope::ConstPtr &type) const;

    bool isIntegral(QQmlJSRegisterContent type) const;
    bool isIntegral(const QQmlJSScope::ConstPtr &type) const;

    bool isSignedInteger(const QQmlJSScope::ConstPtr &type) const;
    bool isUnsignedInteger(const QQmlJSScope::ConstPtr &type) const;
    bool isNativeArrayIndex(const QQmlJSScope::ConstPtr &type) const;

    bool canHold(const QQmlJSScope::ConstPtr &container,
                 const QQmlJSScope::ConstPtr &contained) const;
    bool canHoldUndefined(QQmlJSRegisterContent content) const;
    bool isOptionalType(QQmlJSRegisterContent content) const;

    bool canPopulate(
            const QQmlJSScope::ConstPtr &type, const QQmlJSScope::ConstPtr &argument,
            bool *isExtension) const;

    bool canConvertFromTo(const QQmlJSScope::ConstPtr &from, const QQmlJSScope::ConstPtr &to) const;
    bool canConvertFromTo(QQmlJSRegisterContent from, QQmlJSRegisterContent to) const;

    bool areEquivalentLists(const QQmlJSScope::ConstPtr &a, const QQmlJSScope::ConstPtr &b) const;

    bool isTriviallyCopyable(const QQmlJSScope::ConstPtr &type) const;

    bool inherits(const QQmlJSScope::ConstPtr &derived, const QQmlJSScope::ConstPtr &base) const;


    // Querying of types given other types

    enum class ComponentIsGeneric { No, Yes };
    QQmlJSScope::ConstPtr genericType(
            const QQmlJSScope::ConstPtr &type,
            ComponentIsGeneric allowComponent = ComponentIsGeneric::No) const;

    QQmlJSScope::ConstPtr storedType(const QQmlJSScope::ConstPtr &type) const;

    QQmlJSRegisterContent original(QQmlJSRegisterContent type) const;
    QQmlJSScope::ConstPtr originalContainedType(QQmlJSRegisterContent container) const;

    QQmlJSScope::ConstPtr merge(
            const QQmlJSScope::ConstPtr &a, const QQmlJSScope::ConstPtr &b) const;

    QQmlJSRegisterContent extractNonVoidFromOptionalType(
            QQmlJSRegisterContent content) const;

    QQmlJSMetaMethod selectConstructor(
            const QQmlJSScope::ConstPtr &type, const QQmlJSScope::ConstPtr &argument,
            bool *isExtension) const;


    // Creation of "tracked" QQmlJSRegisterContents

    QQmlJSRegisterContent typeForBinaryOperation(
            QSOperator::Op oper, QQmlJSRegisterContent left,
            QQmlJSRegisterContent right) const;

    enum class UnaryOperator { Not, Plus, Minus, Increment, Decrement, Complement };
    QQmlJSRegisterContent typeForArithmeticUnaryOperation(
            UnaryOperator op, QQmlJSRegisterContent operand) const;

    QQmlJSRegisterContent merge(
            QQmlJSRegisterContent a, QQmlJSRegisterContent b) const;

    QQmlJSRegisterContent literalType(const QQmlJSScope::ConstPtr &type) const;
    QQmlJSRegisterContent operationType(const QQmlJSScope::ConstPtr &type) const;
    QQmlJSRegisterContent namedType(const QQmlJSScope::ConstPtr &type) const;
    QQmlJSRegisterContent syntheticType(const QQmlJSScope::ConstPtr &type) const;

    QQmlJSRegisterContent scopedType(
            QQmlJSRegisterContent scope, const QString &name,
            int lookupIndex = QQmlJSRegisterContent::InvalidLookupIndex,
            QQmlJSScopesByIdOptions options = Default) const;

    QQmlJSRegisterContent memberType(
            QQmlJSRegisterContent type, const QString &name,
            int lookupIndex = QQmlJSRegisterContent::InvalidLookupIndex) const;

    QQmlJSRegisterContent valueType(QQmlJSRegisterContent list) const;

    QQmlJSRegisterContent returnType(
            const QQmlJSMetaMethod &method, const QQmlJSScope::ConstPtr &returnType,
            QQmlJSRegisterContent scope) const;

    QQmlJSRegisterContent extensionType(
            const QQmlJSScope::ConstPtr &extension, QQmlJSRegisterContent base) const;

    QQmlJSRegisterContent baseType(
            const QQmlJSScope::ConstPtr &base, QQmlJSRegisterContent derived) const;

    QQmlJSRegisterContent parentScope(
            const QQmlJSScope::ConstPtr &parent, QQmlJSRegisterContent child) const;

    QQmlJSRegisterContent iteratorPointer(
            QQmlJSRegisterContent listType, QQmlJS::AST::ForEachType type,
            int lookupIndex) const;

    QQmlJSRegisterContent convert(
            QQmlJSRegisterContent from, QQmlJSRegisterContent to) const;
    QQmlJSRegisterContent convert(
            QQmlJSRegisterContent from, const QQmlJSScope::ConstPtr &to) const;


    // Type adjustment

    [[nodiscard]] bool adjustTrackedType(
            QQmlJSRegisterContent tracked, const QQmlJSScope::ConstPtr &conversion) const;
    [[nodiscard]] bool adjustTrackedType(
            QQmlJSRegisterContent tracked, QQmlJSRegisterContent conversion) const;
    [[nodiscard]] bool adjustTrackedType(
            QQmlJSRegisterContent tracked,
            const QList<QQmlJSRegisterContent> &conversions) const;
    void adjustOriginalType(
            QQmlJSRegisterContent tracked, const QQmlJSScope::ConstPtr &conversion) const;
    void generalizeType(QQmlJSRegisterContent type) const;


protected:

    QQmlJSRegisterContent memberType(QQmlJSRegisterContent type, const QString &name,
                                     int baseLookupIndex, int resultLookupIndex) const;
    QQmlJSRegisterContent memberEnumType(QQmlJSRegisterContent type,
                                         const QString &name) const;
    bool checkEnums(QQmlJSRegisterContent scope, const QString &name,
                    QQmlJSRegisterContent *result) const;
    bool canPrimitivelyConvertFromTo(
            const QQmlJSScope::ConstPtr &from, const QQmlJSScope::ConstPtr &to) const;
    QQmlJSRegisterContent lengthProperty(bool isWritable, QQmlJSRegisterContent scope) const;

    QQmlJSScope::ConstPtr containedTypeForName(const QString &name) const;
    QQmlJSRegisterContent registerContentForName(
            const QString &name, QQmlJSRegisterContent scopeType = {}) const;

    QQmlJSScope::ConstPtr resolveParentProperty(
            const QString &name, const QQmlJSScope::ConstPtr &base,
            const QQmlJSScope::ConstPtr &propType) const;

    std::unique_ptr<QQmlJSRegisterContentPool> m_pool;

    QQmlJSScope::ConstPtr m_voidType;
    QQmlJSScope::ConstPtr m_emptyType;
    QQmlJSScope::ConstPtr m_nullType;
    QQmlJSScope::ConstPtr m_numberPrototype;
    QQmlJSScope::ConstPtr m_arrayPrototype;
    QQmlJSScope::ConstPtr m_realType;
    QQmlJSScope::ConstPtr m_floatType;
    QQmlJSScope::ConstPtr m_int8Type;
    QQmlJSScope::ConstPtr m_uint8Type;
    QQmlJSScope::ConstPtr m_int16Type;
    QQmlJSScope::ConstPtr m_uint16Type;
    QQmlJSScope::ConstPtr m_int32Type;
    QQmlJSScope::ConstPtr m_uint32Type;
    QQmlJSScope::ConstPtr m_int64Type;
    QQmlJSScope::ConstPtr m_uint64Type;
    QQmlJSScope::ConstPtr m_sizeType;
    QQmlJSScope::ConstPtr m_boolType;
    QQmlJSScope::ConstPtr m_stringType;
    QQmlJSScope::ConstPtr m_stringListType;
    QQmlJSScope::ConstPtr m_byteArrayType;
    QQmlJSScope::ConstPtr m_urlType;
    QQmlJSScope::ConstPtr m_dateTimeType;
    QQmlJSScope::ConstPtr m_dateType;
    QQmlJSScope::ConstPtr m_timeType;
    QQmlJSScope::ConstPtr m_regexpType;
    QQmlJSScope::ConstPtr m_variantListType;
    QQmlJSScope::ConstPtr m_variantMapType;
    QQmlJSScope::ConstPtr m_varType;
    QQmlJSScope::ConstPtr m_qmlPropertyMapType;
    QQmlJSScope::ConstPtr m_jsValueType;
    QQmlJSScope::ConstPtr m_jsPrimitiveType;
    QQmlJSScope::ConstPtr m_listPropertyType;
    QQmlJSScope::ConstPtr m_qObjectType;
    QQmlJSScope::ConstPtr m_qObjectListType;
    QQmlJSScope::ConstPtr m_qQmlScriptStringType;
    QQmlJSScope::ConstPtr m_metaObjectType;
    QQmlJSScope::ConstPtr m_functionType;
    QQmlJSScope::ConstPtr m_jsGlobalObject;
    QQmlJSScope::ConstPtr m_forInIteratorPtr;
    QQmlJSScope::ConstPtr m_forOfIteratorPtr;

    QQmlJSRegisterContent m_jsGlobalObjectContent;

    QQmlJSScopesById m_objectsById;
    QHash<QV4::CompiledData::Location, QQmlJSScope::ConstPtr> m_objectsByLocation;
    QQmlJSImporter::ImportedTypes m_imports;
    QHash<QQmlJS::SourceLocation, QQmlJSMetaSignalHandler> m_signalHandlers;
    QStringList m_seenModuleQualifiers;

    ParentMode m_parentMode = UseParentProperty;
    CloneMode m_cloneMode = CloneTypes;
    QQmlJSLogger *m_logger = nullptr;
};

/*!
\internal

QQmlJSTypeResolver expects to be outlived by its importer and mapper. It crashes when its importer
or mapper gets destructed. Therefore, you can use this struct to extend the lifetime of its
dependencies in case you need to store the resolver as a class member.
QQmlJSTypeResolver also expects to be outlived by the logger used by the importvisitor, while the
importvisitor actually does not and will not outlive the QQmlJSTypeResolver.
*/
struct QQmlJSTypeResolverDependencies
{
    std::shared_ptr<QQmlJSImporter> importer;
    std::shared_ptr<QQmlJSResourceFileMapper> mapper;
    std::shared_ptr<QQmlJSLogger> logger;
};

QT_END_NAMESPACE

#endif // QQMLJSTYPERESOLVER_P_H
