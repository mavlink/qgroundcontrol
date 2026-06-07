// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:significant

#ifndef QQMLTYPEREGISTRARCONSTANTS_P_H
#define QQMLTYPEREGISTRARCONSTANTS_P_H

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

#include <QtCore/qlatin1stringview.h>

QT_BEGIN_NAMESPACE

namespace Constants {

// Strings that commonly occur in .qmltypes files.
namespace DotQmltypes {
static constexpr QLatin1StringView S_ACCESS_SEMANTICS             { "accessSemantics" };
static constexpr QLatin1StringView S_ALIAS                        { "alias" };
static constexpr QLatin1StringView S_ALIASES                      { "aliases" };
static constexpr QLatin1StringView S_ARGUMENTS                    { "arguments" };
static constexpr QLatin1StringView S_ATTACHED_TYPE                { "attachedType" };
static constexpr QLatin1StringView S_BINDABLE                     { "bindable" };
static constexpr QLatin1StringView S_COMPONENT                    { "Component" };
static constexpr QLatin1StringView S_DEFAULT_PROPERTY             { "defaultProperty" };
static constexpr QLatin1StringView S_DEFERRED_NAMES               { "deferredNames" };
static constexpr QLatin1StringView S_ENFORCES_SCOPED_ENUMS        { "enforcesScopedEnums" };
static constexpr QLatin1StringView S_ENUM                         { "Enum" };
static constexpr QLatin1StringView S_EXPORTS                      { "exports" };
static constexpr QLatin1StringView S_EXPORT_META_OBJECT_REVISIONS { "exportMetaObjectRevisions" };
static constexpr QLatin1StringView S_EXTENSION                    { "extension" };
static constexpr QLatin1StringView S_EXTENSION_IS_JAVA_SCRIPT     { "extensionIsJavaScript" };
static constexpr QLatin1StringView S_EXTENSION_IS_NAMESPACE       { "extensionIsNamespace" };
static constexpr QLatin1StringView S_FILE                         { "file" };
static constexpr QLatin1StringView S_HAS_CUSTOM_PARSER            { "hasCustomParser" };
static constexpr QLatin1StringView S_IMMEDIATE_NAMES              { "immediateNames" };
static constexpr QLatin1StringView S_INDEX                        { "index" };
static constexpr QLatin1StringView S_INTERFACES                   { "interfaces" };
static constexpr QLatin1StringView S_IS_CLONED                    { "isCloned" };
static constexpr QLatin1StringView S_IS_CONSTRUCTOR               { "isConstructor" };
static constexpr QLatin1StringView S_IS_CREATABLE                 { "isCreatable" };
static constexpr QLatin1StringView S_IS_FINAL                     { "isFinal" };
static constexpr QLatin1StringView S_IS_FLAG                      { "isFlag" };
static constexpr QLatin1StringView S_IS_JAVASCRIPT_BUILTIN        { "isJavaScriptBuiltin" };
static constexpr QLatin1StringView S_IS_JAVASCRIPT_FUNCTION       { "isJavaScriptFunction" };
static constexpr QLatin1StringView S_IS_LIST                      { "isList" };
static constexpr QLatin1StringView S_IS_METHOD_CONSTANT           { "isMethodConstant" };
static constexpr QLatin1StringView S_IS_POINTER                   { "isPointer" };
static constexpr QLatin1StringView S_IS_PROPERTY_CONSTANT         { "isPropertyConstant" };
static constexpr QLatin1StringView S_IS_READONLY                  { "isReadonly" };
static constexpr QLatin1StringView S_IS_REQUIRED                  { "isRequired" };
static constexpr QLatin1StringView S_IS_SCOPED                    { "isScoped" };
static constexpr QLatin1StringView S_IS_SINGLETON                 { "isSingleton" };
static constexpr QLatin1StringView S_IS_STRUCTURED                { "isStructured" };
static constexpr QLatin1StringView S_IS_TYPE_CONSTANT             { "isTypeConstant" };
static constexpr QLatin1StringView S_LINE_NUMBER                  { "lineNumber" };
static constexpr QLatin1StringView S_METHOD                       { "Method" };
static constexpr QLatin1StringView S_MODULE                       { "Module" };
static constexpr QLatin1StringView S_NAME                         { "name" };
static constexpr QLatin1StringView S_NONE                         { "none" };
static constexpr QLatin1StringView S_NOTIFY                       { "notify" };
static constexpr QLatin1StringView S_PARAMETER                    { "Parameter" };
static constexpr QLatin1StringView S_PARENT_PROPERTY              { "parentProperty" };
static constexpr QLatin1StringView S_PRIVATE_CLASS                { "privateClass" };
static constexpr QLatin1StringView S_PROPERTY                     { "Property" };
static constexpr QLatin1StringView S_PROTOTYPE                    { "prototype" };
static constexpr QLatin1StringView S_READ                         { "read" };
static constexpr QLatin1StringView S_REFERENCE                    { "reference" };
static constexpr QLatin1StringView S_RESET                        { "reset" };
static constexpr QLatin1StringView S_REVISION                     { "revision" };
static constexpr QLatin1StringView S_SEQUENCE                     { "sequence" };
static constexpr QLatin1StringView S_SIGNAL                       { "Signal" };
static constexpr QLatin1StringView S_TYPE                         { "type" };
static constexpr QLatin1StringView S_VALUE                        { "value" };
static constexpr QLatin1StringView S_VALUES                       { "values" };
static constexpr QLatin1StringView S_VALUE_TYPE                   { "valueType" };
static constexpr QLatin1StringView S_WRITE                        { "write" };
}

// Strings that commonly occur in metatypes.json files.
namespace MetatypesDotJson {
static constexpr QLatin1StringView S_ACCESS                       { "access" };
static constexpr QLatin1StringView S_ALIAS                        { "alias" };
static constexpr QLatin1StringView S_ANONYMOUS                    { "anonymous" };
static constexpr QLatin1StringView S_ARGUMENTS                    { "arguments" };
static constexpr QLatin1StringView S_AUTO                         { "auto" };
static constexpr QLatin1StringView S_BINDABLE                     { "bindable" };
static constexpr QLatin1StringView S_CLASSES                      { "classes" };
static constexpr QLatin1StringView S_CLASS_INFOS                  { "classInfos" };
static constexpr QLatin1StringView S_CLASS_NAME                   { "className" };
static constexpr QLatin1StringView S_CONSTANT                     { "constant" };
static constexpr QLatin1StringView S_CONSTRUCT                    { "construct" };
static constexpr QLatin1StringView S_CONSTRUCTORS                 { "constructors" };
static constexpr QLatin1StringView S_DEFAULT_PROPERTY             { "DefaultProperty" };
static constexpr QLatin1StringView S_DEFERRED_PROPERTY_NAMES      { "DeferredPropertyNames" };
static constexpr QLatin1StringView S_ENUMS                        { "enums" };
static constexpr QLatin1StringView S_FALSE                        { "false" };
static constexpr QLatin1StringView S_FINAL                        { "final" };
static constexpr QLatin1StringView S_GADGET                       { "gadget" };
static constexpr QLatin1StringView S_IMMEDIATE_PROPERTY_NAMES     { "ImmediatePropertyNames" };
static constexpr QLatin1StringView S_INDEX                        { "index" };
static constexpr QLatin1StringView S_INPUT_FILE                   { "inputFile" };
static constexpr QLatin1StringView S_INTERFACES                   { "interfaces" };
static constexpr QLatin1StringView S_IS_CLASS                     { "isClass" };
static constexpr QLatin1StringView S_IS_CLONED                    { "isCloned" };
static constexpr QLatin1StringView S_IS_CONST                     { "isConst" };
static constexpr QLatin1StringView S_IS_CONSTRUCTOR               { "isConstructor" };
static constexpr QLatin1StringView S_IS_FLAG                      { "isFlag" };
static constexpr QLatin1StringView S_IS_JAVASCRIPT_FUNCTION       { "isJavaScriptFunction" };
static constexpr QLatin1StringView S_LINENUMBER                   { "lineNumber" };
static constexpr QLatin1StringView S_MEMBER                       { "member" };
static constexpr QLatin1StringView S_METHOD                       { "method" };
static constexpr QLatin1StringView S_METHODS                      { "methods" };
static constexpr QLatin1StringView S_NAME                         { "name" };
static constexpr QLatin1StringView S_NAMESPACE                    { "namespace" };
static constexpr QLatin1StringView S_NOTIFY                       { "notify" };
static constexpr QLatin1StringView S_OBJECT                       { "object" };
static constexpr QLatin1StringView S_PARENT_PROPERTY              { "ParentProperty" };
static constexpr QLatin1StringView S_PRIVATE                      { "private" };
static constexpr QLatin1StringView S_PRIVATE_CLASS                { "privateClass" };
static constexpr QLatin1StringView S_PROPERTIES                   { "properties" };
static constexpr QLatin1StringView S_PROPERTY                     { "property" };
static constexpr QLatin1StringView S_PROTECTED                    { "protected" };
static constexpr QLatin1StringView S_PUBLIC                       { "public" };
static constexpr QLatin1StringView S_QUALIFIED_CLASS_NAME         { "qualifiedClassName" };
static constexpr QLatin1StringView S_READ                         { "read" };

static constexpr QLatin1StringView S_REGISTER_ENUM_CLASSES_UNSCOPED {
    "RegisterEnumClassesUnscoped"
};

static constexpr QLatin1StringView S_REQUIRED                     { "required" };
static constexpr QLatin1StringView S_RESET                        { "reset" };
static constexpr QLatin1StringView S_RETURN_TYPE                  { "returnType" };
static constexpr QLatin1StringView S_REVISION                     { "revision" };
static constexpr QLatin1StringView S_SIGNALS                      { "signals" };
static constexpr QLatin1StringView S_SLOTS                        { "slots" };
static constexpr QLatin1StringView S_STRUCTURED                   { "structured" };
static constexpr QLatin1StringView S_SUPER_CLASSES                { "superClasses" };
static constexpr QLatin1StringView S_TRUE                         { "true" };
static constexpr QLatin1StringView S_TYPE                         { "type" };
static constexpr QLatin1StringView S_VALUE                        { "value" };
static constexpr QLatin1StringView S_VALUES                       { "values" };
static constexpr QLatin1StringView S_WRITE                        { "write" };

// QML-Related Strings that commonly occur in metatypes.json files.
namespace Qml {
static constexpr QLatin1StringView S_ADDED_IN_VERSION             { "QML.AddedInVersion" };
static constexpr QLatin1StringView S_ATTACHED                     { "QML.Attached" };
static constexpr QLatin1StringView S_CREATABLE                    { "QML.Creatable" };
static constexpr QLatin1StringView S_CREATION_METHOD              { "QML.CreationMethod" };
static constexpr QLatin1StringView S_ELEMENT                      { "QML.Element" };
static constexpr QLatin1StringView S_EXTENDED                     { "QML.Extended" };
static constexpr QLatin1StringView S_EXTENSION_IS_JAVA_SCRIPT     { "QML.ExtensionIsJavaScript" };
static constexpr QLatin1StringView S_EXTENSION_IS_NAMESPACE       { "QML.ExtensionIsNamespace" };
static constexpr QLatin1StringView S_FOREIGN                      { "QML.Foreign" };
static constexpr QLatin1StringView S_FOREIGN_IS_NAMESPACE         { "QML.ForeignIsNamespace" };
static constexpr QLatin1StringView S_HAS_CUSTOM_PARSER            { "QML.HasCustomParser" };
static constexpr QLatin1StringView S_PRIMITIVE_ALIAS              { "QML.PrimitiveAlias" };
static constexpr QLatin1StringView S_REMOVED_IN_VERSION           { "QML.RemovedInVersion" };
static constexpr QLatin1StringView S_ROOT                         { "QML.Root" };
static constexpr QLatin1StringView S_SEQUENCE                     { "QML.Sequence" };
static constexpr QLatin1StringView S_SINGLETON                    { "QML.Singleton" };
static constexpr QLatin1StringView S_UNCREATABLE_REASON           { "QML.UncreatableReason" };
static constexpr QLatin1StringView S_USING                        { "QML.Using" };
} // namespace Qml

} // namespace MetatypesJson

}

QT_END_NAMESPACE

#endif // QQMLTYPEREGISTRARCONSTANTS_P_H
