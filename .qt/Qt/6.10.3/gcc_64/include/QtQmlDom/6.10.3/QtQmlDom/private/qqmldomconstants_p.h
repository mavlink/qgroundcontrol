// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLDOMCONSTANTS_P_H
#define QQMLDOMCONSTANTS_P_H

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

#include <QtCore/QObject>
#include <QtCore/QMetaObject>
#include <QtCore/private/qglobal_p.h>

QT_BEGIN_NAMESPACE

namespace QQmlJS{
namespace Dom {

Q_NAMESPACE_EXPORT(QMLDOM_EXPORT)

enum class PathRoot {
    Other,
    Modules,
    Cpp,
    Libs,
    Top,
    Env,
    Universe
};
Q_ENUM_NS(PathRoot)

enum class PathCurrent {
    Other,
    Obj,
    ObjChain,
    ScopeChain,
    Component,
    Module,
    Ids,
    Types,
    LookupStrict,
    LookupDynamic,
    Lookup
};
Q_ENUM_NS(PathCurrent)

enum class Language { QmlQuick1, QmlQuick2, QmlQuick3, QmlCompiled, QmlAnnotation, Qbs };
Q_ENUM_NS(Language)

enum class ResolveOption{
    None=0,
    TraceVisit=0x1 // call the function along all elements of the path, not just for the target (the function might be called even if the target is never reached)
};
Q_ENUM_NS(ResolveOption)
Q_DECLARE_FLAGS(ResolveOptions, ResolveOption)
Q_DECLARE_OPERATORS_FOR_FLAGS(ResolveOptions)

enum class VisitOption {
    None = 0,
    VisitSelf = 0x1, // Visit the start item
    VisitAdopted = 0x2, // Visit adopted types (but never recurses them)
    Recurse = 0x4, // recurse non adopted types
    NoPath = 0x8, // does not generate path consistent with visit
    Default = VisitOption::VisitSelf | VisitOption::VisitAdopted | VisitOption::Recurse
};
Q_ENUM_NS(VisitOption)
Q_DECLARE_FLAGS(VisitOptions, VisitOption)
Q_DECLARE_OPERATORS_FOR_FLAGS(VisitOptions)

enum class LookupOption {
    Normal = 0,
    Strict = 0x1,
    VisitTopClassType = 0x2, // static lookup of class (singleton) or attached type, the default is
                             // visiting instance methods
    SkipFirstScope = 0x4
};
Q_ENUM_NS(LookupOption)
Q_DECLARE_FLAGS(LookupOptions, LookupOption)
Q_DECLARE_OPERATORS_FOR_FLAGS(LookupOptions)

enum class LookupType { PropertyDef, Binding, Property, Method, Type, CppType, Symbol };
Q_ENUM_NS(LookupType)

enum class VisitPrototypesOption {
    Normal = 0,
    SkipFirst = 0x1,
    RevisitWarn = 0x2,
    ManualProceedToScope = 0x4
};
Q_ENUM_NS(VisitPrototypesOption)
Q_DECLARE_FLAGS(VisitPrototypesOptions, VisitPrototypesOption)
Q_DECLARE_OPERATORS_FOR_FLAGS(VisitPrototypesOptions)

enum class DomKind { Empty, Object, List, Map, Value, ScriptElement };
Q_ENUM_NS(DomKind)

enum class DomType {
    Empty, // only for default ctor

    ExternalItemInfo, // base class for anything represented by an actual file
    ExternalItemPair, // pair of newest version of item, and latest valid update ### REVISIT
    // ExternalOwningItems refer to an external path and can be shared between environments
    QmlDirectory, // dir e.g. used for implicit import
    QmldirFile, // qmldir
    JsFile, // file
    QmlFile, // file
    QmltypesFile, // qmltypes
    GlobalScope, // language dependent (currently no difference)
    /* enum A {  B, C }
                 *  *
    EnumItem  is marked with * */
    EnumItem,

    // types
    EnumDecl, // A in above example
    JsResource, // QML file contains QML object, JSFile contains JsResource
    QmltypesComponent, // Component inside a qmltypes fles; compared to component it has exported
                       // meta-object revisions; singleton flag; can export multiple names
    QmlComponent, // "normal" QML file based Component; also can represent inline components
    GlobalComponent, // component of global object ### REVISIT, try to replace with one of the above

    ModuleAutoExport, // dependent imports to automatically load when a module is imported
    ModuleIndex, // index for all the imports of a major version
    ModuleScope, // a specific import with full version
    ImportScope, // the scope including the types coming from one or more imports
    Export, // An exported type

    // header stuff
    Import, // wrapped
    Pragma,

    // qml elements
    Id,
    QmlObject, // the Item in Item {}; also used to represent types in qmltype files
    ConstantData, // the 2 in  "property int i: 2"; can be any generic data in a QML document
    SimpleObjectWrap, // internal wrapping to give uniform DOMItem access; ### research more
    ScriptExpression, // wraps an AST script expression as a DOMItem
    Reference, // reference to another DOMItem; e.g. asking for a type of an object returns a
               // Reference
    PropertyDefinition, // _just_ the property definition; without the binding, even if it's one
                        // line
    Binding, // the part after the ":"
    MethodParameter,
    MethodInfo, // container of MethodParameter
    Version, // wrapped
    Comment,
    CommentedElement, // attached to AST if they have pre-/post-comments?
    RegionComments, // DomItems have attached RegionComments; can attach comments to fine grained
                    // "regions" in a DomItem; like the default keyword of a property definition
    AstComments, // hash-table from AST node to commented element
    FileLocationsInfo, // mapping from DomItem to file location ### REVISIT: try to move out of
                       // hierarchy?

    // convenience collecting types
    PropertyInfo, // not a DOM Item, just a convenience class

    // Moc objects, mainly for testing ### Try to remove them; replace their usage in tests with
    // "real" instances
    MockObject,
    MockOwner,

    // containers
    Map,
    List,
    ListP,

    // supporting objects
    LoadInfo, // owning, used inside DomEnvironment ### REVISIT: move out of hierarchy
    ErrorMessage, // wrapped
    FileLocationsNode, // owning

    // Dom top level
    DomEnvironment, // a consistent view of modules, types, files, etc.
    DomUniverse, // a cache of what can be found in the DomEnvironment, contains the latest valid
                 // version for every file/type, etc. + latest overall

    // Dom Script elements
    // TODO
    ScriptElementWrap, // internal wrapping to give uniform access of script elements (e.g. for
                       // statement lists)
    ScriptElementStart, // marker to check if a DomType is a scriptelement or not
    ScriptBlockStatement = ScriptElementStart,
    ScriptIdentifierExpression,
    ScriptLiteral,
    ScriptRegExpLiteral,
    ScriptForStatement,
    ScriptIfStatement,
    ScriptPostExpression,
    ScriptUnaryExpression,
    ScriptBinaryExpression,
    ScriptVariableDeclaration,
    ScriptVariableDeclarationEntry,
    ScriptReturnStatement,
    ScriptGenericElement,
    ScriptCallExpression,
    ScriptFormalParameter,
    ScriptArray,
    ScriptObject,
    ScriptProperty,
    ScriptType,
    ScriptElision,
    ScriptArrayEntry,
    ScriptPattern,
    ScriptSwitchStatement,
    ScriptCaseBlock,
    ScriptCaseClause,
    ScriptDefaultClause,
    ScriptWhileStatement,
    ScriptDoWhileStatement,
    ScriptForEachStatement,
    ScriptTemplateExpressionPart,
    ScriptTemplateLiteral,
    ScriptTemplateStringPart,
    ScriptTaggedTemplate,
    ScriptTryCatchStatement,
    ScriptThrowStatement,
    ScriptLabelledStatement,
    ScriptBreakStatement,
    ScriptContinueStatement,
    ScriptConditionalExpression,
    ScriptEmptyStatement,
    ScriptParenthesizedExpression,
    ScriptFunctionExpression,
    ScriptYieldExpression,
    ScriptNewExpression,
    ScriptNewMemberExpression,
    ScriptThisExpression,
    ScriptSuperLiteral,

    ScriptElementStop, // marker to check if a DomType is a scriptelement or not
};
Q_ENUM_NS(DomType)

enum class SimpleWrapOption { None = 0, ValueType = 1 };
Q_ENUM_NS(SimpleWrapOption)
Q_DECLARE_FLAGS(SimpleWrapOptions, SimpleWrapOption)
Q_DECLARE_OPERATORS_FOR_FLAGS(SimpleWrapOptions)

enum class BindingValueKind { Object, ScriptExpression, Array, Empty };
Q_ENUM_NS(BindingValueKind)

enum class BindingType { Normal, OnBinding };
Q_ENUM_NS(BindingType)

enum class ListOptions {
    Normal,
    Reverse
};
Q_ENUM_NS(ListOptions)

enum class EscapeOptions{
    OuterQuotes,
    NoOuterQuotes
};
Q_ENUM_NS(EscapeOptions)

enum class ErrorLevel{
    Debug = QtMsgType::QtDebugMsg,
    Info = QtMsgType::QtInfoMsg,
    Warning = QtMsgType::QtWarningMsg,
    Error = QtMsgType::QtCriticalMsg,
    Fatal = QtMsgType::QtFatalMsg
};
Q_ENUM_NS(ErrorLevel)

enum class AstDumperOption {
    None=0,
    NoLocations=0x1,
    NoAnnotations=0x2,
    DumpNode=0x4,
    SloppyCompare=0x8
};
Q_ENUM_NS(AstDumperOption)
Q_DECLARE_FLAGS(AstDumperOptions, AstDumperOption)
Q_DECLARE_OPERATORS_FOR_FLAGS(AstDumperOptions)

enum class GoTo {
    Strict, // never go to an non uniquely defined result
    MostLikely // if needed go up to the most likely location between multiple options
};
Q_ENUM_NS(GoTo)

enum class AddOption { KeepExisting, Overwrite };
Q_ENUM_NS(AddOption)

/*!
\internal
FilterUpOptions decide in which direction the filtering is done.
ReturnInner starts the search at top(), and work its way down to the current
element.
ReturnOuter and ReturnOuterNoSelf starts the search at the current element and
works their way up to to top().
*/
enum class FilterUpOptions { ReturnOuter, ReturnOuterNoSelf, ReturnInner };
Q_ENUM_NS(FilterUpOptions)

enum class WriteOutCheck {
    None = 0x0,
    Reparse = 0x4,
    ReparseCompare = 0x8,
    ReparseStable = 0x10,
    Default = Reparse | ReparseCompare | ReparseStable
};
Q_ENUM_NS(WriteOutCheck)
Q_DECLARE_FLAGS(WriteOutChecks, WriteOutCheck)
Q_DECLARE_OPERATORS_FOR_FLAGS(WriteOutChecks)

enum class LocalSymbolsType {
    None = 0x0,
    ObjectType = 0x1,
    ValueType = 0x2,
    Signal = 0x4,
    Method = 0x8,
    Attribute = 0x10,
    Id = 0x20,
    Namespace = 0x40,
    Global = 0x80,
    MethodParameter = 0x100,
    Singleton = 0x200,
    AttachedType = 0x400,
};
Q_ENUM_NS(LocalSymbolsType)
Q_DECLARE_FLAGS(LocalSymbolsTypes, LocalSymbolsType)
Q_DECLARE_OPERATORS_FOR_FLAGS(LocalSymbolsTypes)

/*!
\internal
The FileLocationRegion allows to map the different FileLocation subregions to their position in
the actual code. For example, \c{ColonTokenRegion} denotes the position of the ':' token in a
binding like `myProperty: something()`, or the ':' token in a pragma like `pragma Hello: World`.

These are used for formatting in qmlformat and autocompletion in qmlls.

MainRegion denotes the entire FileLocation region.

\sa{OutWriter::regionToString}, {FileLocations::regionName}
*/
enum FileLocationRegion : int {
    AsTokenRegion,
    BreakKeywordRegion,
    DoKeywordRegion,
    CaseKeywordRegion,
    CatchKeywordRegion,
    ColonTokenRegion,
    CommaTokenRegion,
    ComponentKeywordRegion,
    ContinueKeywordRegion,
    DefaultKeywordRegion,
    DollarLeftBraceTokenRegion,
    EllipsisTokenRegion,
    ElseKeywordRegion,
    EnumKeywordRegion,
    EnumValueRegion,
    EqualTokenRegion,
    ForKeywordRegion,
    FinalKeywordRegion,
    FinallyKeywordRegion,
    FirstSemicolonTokenRegion,
    FunctionKeywordRegion,
    IdColonTokenRegion,
    IdNameRegion,
    IdTokenRegion,
    IdentifierRegion,
    IfKeywordRegion,
    ImportTokenRegion,
    ImportUriRegion,
    InOfTokenRegion,
    LeftBacktickTokenRegion,
    LeftBraceRegion,
    LeftBracketRegion,
    LeftParenthesisRegion,
    MainRegion,
    NewKeywordRegion,
    OperatorTokenRegion,
    OnTargetRegion,
    OnTokenRegion,
    PragmaKeywordRegion,
    PragmaValuesRegion,
    PropertyKeywordRegion,
    QuestionMarkTokenRegion,
    ReadonlyKeywordRegion,
    RequiredKeywordRegion,
    ReturnKeywordRegion,
    RightBacktickTokenRegion,
    RightBraceRegion,
    RightBracketRegion,
    RightParenthesisRegion,
    SecondSemicolonRegion,
    SemicolonTokenRegion,
    SignalKeywordRegion,
    SuperKeywordRegion,
    StarTokenRegion,
    SwitchKeywordRegion,
    ThisKeywordRegion,
    ThrowKeywordRegion,
    TryKeywordRegion,
    TypeIdentifierRegion,
    TypeModifierRegion,
    VersionRegion,
    WhileKeywordRegion,
    YieldKeywordRegion,
};
Q_ENUM_NS(FileLocationRegion);

enum DomCreationOption : char {
    Default, // required by qmlformat for example
    Extended, // required by qmlls for example
    Minimal, // required by QmlDocumentParser in Qt Design Studio, for example
};

} // end namespace Dom
} // end namespace QQmlJS

QT_END_NAMESPACE

#endif // QQMLDOMCONSTANTS_P_H
