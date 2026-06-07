// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLIRBUILDER_P_H
#define QQMLIRBUILDER_P_H

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

#include <private/qqmljsast_p.h>
#include <private/qqmljsengine_p.h>
#include <private/qv4compiler_p.h>
#include <private/qv4compileddata_p.h>
#include <private/qqmljsmemorypool_p.h>
#include <private/qqmljsfixedpoolarray_p.h>
#include <private/qv4codegen_p.h>
#include <private/qv4compiler_p.h>
#include <QTextStream>
#include <QCoreApplication>

QT_BEGIN_NAMESPACE

class QQmlPropertyCache;
class QQmlContextData;
class QQmlTypeNameCache;
struct QQmlIRLoader;

namespace QmlIR {

struct Document;

template <typename T>
struct PoolList
{
    PoolList()
        : first(nullptr)
        , last(nullptr)
    {}

    T *first;
    T *last;
    int count = 0;

    int append(T *item) {
        item->next = nullptr;
        if (last)
            last->next = item;
        else
            first = item;
        last = item;
        return count++;
    }

    void prepend(T *item) {
        item->next = first;
        first = item;
        if (!last)
            last = first;
        ++count;
    }

    template <typename Sortable, typename Base, Sortable Base::*sortMember>
    T *findSortedInsertionPoint(T *item) const
    {
        T *insertPos = nullptr;

        for (T *it = first; it; it = it->next) {
            if (!(it->*sortMember <= item->*sortMember))
                break;
            insertPos = it;
        }

        return insertPos;
    }

    void insertAfter(T *insertionPoint, T *item) {
        if (!insertionPoint) {
            prepend(item);
        } else if (insertionPoint == last) {
            append(item);
        } else {
            item->next = insertionPoint->next;
            insertionPoint->next = item;
            ++count;
        }
    }

    T *unlink(T *before, T *item) {
        T * const newNext = item->next;

        if (before)
            before->next = newNext;
        else
            first = newNext;

        if (item == last) {
            if (newNext)
                last = newNext;
            else
                last = first;
        }

        --count;
        return newNext;
    }

    T *slowAt(int index) const
    {
        T *result = first;
        while (index > 0 && result) {
            result = result->next;
            --index;
        }
        return result;
    }

    struct Iterator {
        // turn Iterator into a proper iterator
        using iterator_category = std::forward_iterator_tag;
        using value_type = T;
        using difference_type = ptrdiff_t;
        using pointer = T *;
        using reference = T &;

        T *ptr;

        explicit Iterator(T *p) : ptr(p) {}

        T *operator->() {
            return ptr;
        }

        const T *operator->() const {
            return ptr;
        }

        T &operator*() {
            return *ptr;
        }

        const T &operator*() const {
            return *ptr;
        }

        Iterator& operator++() {
            ptr = ptr->next;
            return *this;
        }

        Iterator operator++(int) {
            Iterator that {ptr};
            ptr = ptr->next;
            return that;
        }

        bool operator==(const Iterator &rhs) const {
            return ptr == rhs.ptr;
        }

        bool operator!=(const Iterator &rhs) const {
            return ptr != rhs.ptr;
        }

        operator T *() { return ptr; }
        operator const T *() const { return ptr; }
    };

    Iterator begin() { return Iterator(first); }
    Iterator end() { return Iterator(nullptr); }

    using iterator = Iterator;
};

struct Object;

struct EnumValue : public QV4::CompiledData::EnumValue
{
    EnumValue *next;
};

struct Enum
{
    int nameIndex;
    QV4::CompiledData::Location location;
    PoolList<EnumValue> *enumValues;

    int enumValueCount() const { return enumValues->count; }
    PoolList<EnumValue>::Iterator enumValuesBegin() const { return enumValues->begin(); }
    PoolList<EnumValue>::Iterator enumValuesEnd() const { return enumValues->end(); }

    Enum *next;
};


struct Parameter : public QV4::CompiledData::Parameter
{
    Parameter *next;

    template<typename IdGenerator>
    static bool initType(
            QV4::CompiledData::ParameterType *type, const IdGenerator &idGenerator,
            const QQmlJS::AST::Type *annotation)
    {
        using Flag = QV4::CompiledData::ParameterType::Flag;

        if (!annotation)
            return initType(type, QString(), idGenerator(QString()), Flag::NoFlag);

        const QString typeId = annotation->typeId->toString();
        const QString typeArgument =
                annotation->typeArgument ? annotation->typeArgument->toString() : QString();

        if (typeArgument.isEmpty())
            return initType(type, typeId, idGenerator(typeId), Flag::NoFlag);

        if (typeId == QLatin1String("list"))
            return initType(type, typeArgument, idGenerator(typeArgument), Flag::List);

        const QString annotationString = annotation->toString();
        return initType(type, annotationString, idGenerator(annotationString), Flag::NoFlag);
    }

    static QV4::CompiledData::CommonType stringToBuiltinType(const QString &typeName);

private:
    static bool initType(
            QV4::CompiledData::ParameterType *paramType, const QString &typeName,
            int typeNameIndex, QV4::CompiledData::ParameterType::Flag listFlag);
};

struct Signal
{
    int nameIndex;
    QV4::CompiledData::Location location;
    PoolList<Parameter> *parameters;

    QStringList parameterStringList(const QV4::Compiler::StringTableGenerator *stringPool) const;

    int parameterCount() const { return parameters->count; }
    PoolList<Parameter>::Iterator parametersBegin() const { return parameters->begin(); }
    PoolList<Parameter>::Iterator parametersEnd() const { return parameters->end(); }

    Signal *next;
};

struct Property : public QV4::CompiledData::Property
{
    Property *next;
};

struct Binding : public QV4::CompiledData::Binding
{
    // The offset in the source file where the binding appeared. This is used for sorting to ensure
    // that assignments to list properties are done in the correct order. We use the offset here instead
    // of Binding::location as the latter has limited precision.
    quint32 offset;
    // Binding's compiledScriptIndex is index in object's functionsAndExpressions
    Binding *next;
};

struct InlineComponent : public QV4::CompiledData::InlineComponent
{
    InlineComponent *next;
};

struct Alias : public QV4::CompiledData::Alias
{
    Alias *next;
};

struct RequiredPropertyExtraData : public QV4::CompiledData::RequiredPropertyExtraData
{
    RequiredPropertyExtraData *next;
};

struct Function
{
    QV4::CompiledData::Location location;
    quint32 nameIndex : 31;
    quint32 isQmlFunction : 1;
    quint32 index = 0; // index in parsedQML::functions
    QQmlJS::FixedPoolArray<Parameter> formals;
    QV4::CompiledData::ParameterType returnType;

    // --- QQmlPropertyCacheCreator interface
    const Parameter *formalsBegin() const { return formals.begin(); }
    const Parameter *formalsEnd() const { return formals.end(); }
    // ---

    Function *next;
};

struct Q_QML_COMPILER_EXPORT CompiledFunctionOrExpression
{
    CompiledFunctionOrExpression()
    {}

    QQmlJS::AST::Node *parentNode = nullptr; // FunctionDeclaration, Statement or Expression
    QQmlJS::AST::Node *node = nullptr; // FunctionDeclaration, Statement or Expression
    quint32 nameIndex = 0;
    CompiledFunctionOrExpression *next = nullptr;
};

struct Q_QML_COMPILER_EXPORT Object
{
    Q_DECLARE_TR_FUNCTIONS(Object)
public:
    quint32 inheritedTypeNameIndex;
    quint32 idNameIndex;
    int id;
    int indexOfDefaultPropertyOrAlias;
    bool defaultPropertyIsAlias;
    quint32 flags;

    QV4::CompiledData::Location location;
    QV4::CompiledData::Location locationOfIdProperty;

    const Property *firstProperty() const { return properties->first; }
    int propertyCount() const { return properties->count; }
    Alias *firstAlias() const { return aliases->first; }
    int aliasCount() const { return aliases->count; }
    const Enum *firstEnum() const { return qmlEnums->first; }
    int enumCount() const { return qmlEnums->count; }
    const Signal *firstSignal() const { return qmlSignals->first; }
    int signalCount() const { return qmlSignals->count; }
    Binding *firstBinding() const { return bindings->first; }
    int bindingCount() const { return bindings->count; }
    const Function *firstFunction() const { return functions->first; }
    int functionCount() const { return functions->count; }
    const InlineComponent *inlineComponent() const { return inlineComponents->first; }
    int inlineComponentCount() const { return inlineComponents->count; }
    const RequiredPropertyExtraData *requiredPropertyExtraData() const {return requiredPropertyExtraDatas->first; }
    int requiredPropertyExtraDataCount() const { return requiredPropertyExtraDatas->count; }
    void simplifyRequiredProperties();

    PoolList<Binding>::Iterator bindingsBegin() const { return bindings->begin(); }
    PoolList<Binding>::Iterator bindingsEnd() const { return bindings->end(); }
    PoolList<Property>::Iterator propertiesBegin() const { return properties->begin(); }
    PoolList<Property>::Iterator propertiesEnd() const { return properties->end(); }
    PoolList<Alias>::Iterator aliasesBegin() const { return aliases->begin(); }
    PoolList<Alias>::Iterator aliasesEnd() const { return aliases->end(); }
    PoolList<Enum>::Iterator enumsBegin() const { return qmlEnums->begin(); }
    PoolList<Enum>::Iterator enumsEnd() const { return qmlEnums->end(); }
    PoolList<Signal>::Iterator signalsBegin() const { return qmlSignals->begin(); }
    PoolList<Signal>::Iterator signalsEnd() const { return qmlSignals->end(); }
    PoolList<Function>::Iterator functionsBegin() const { return functions->begin(); }
    PoolList<Function>::Iterator functionsEnd() const { return functions->end(); }
    PoolList<InlineComponent>::Iterator inlineComponentsBegin() const { return inlineComponents->begin(); }
    PoolList<InlineComponent>::Iterator inlineComponentsEnd() const { return inlineComponents->end(); }
    PoolList<RequiredPropertyExtraData>::Iterator requiredPropertyExtraDataBegin() const {return requiredPropertyExtraDatas->begin(); }
    PoolList<RequiredPropertyExtraData>::Iterator requiredPropertyExtraDataEnd() const {return requiredPropertyExtraDatas->end(); }

    // If set, then declarations for this object (and init bindings for these) should go into the
    // specified object. Used for declarations inside group properties.
    Object *declarationsOverride;

    void init(QQmlJS::MemoryPool *pool, int typeNameIndex, int idIndex, const QV4::CompiledData::Location &location);

    QString appendEnum(Enum *enumeration);
    QString appendSignal(Signal *signal);
    QString appendProperty(Property *prop, const QString &propertyName, bool isDefaultProperty, const QQmlJS::SourceLocation &defaultToken, QQmlJS::SourceLocation *errorLocation);
    QString appendAlias(Alias *prop, const QString &aliasName, bool isDefaultProperty, const QQmlJS::SourceLocation &defaultToken, QQmlJS::SourceLocation *errorLocation);
    void setFirstAlias(Alias *alias) { aliases->first = alias; }

    void appendFunction(QmlIR::Function *f);
    void appendInlineComponent(InlineComponent *ic);
    void appendRequiredPropertyExtraData(RequiredPropertyExtraData *extraData);

    QString appendBinding(Binding *b, bool isListBinding);
    Binding *findBinding(quint32 nameIndex) const;
    Binding *unlinkBinding(Binding *before, Binding *binding) { return bindings->unlink(before, binding); }
    void insertSorted(Binding *b);
    QString bindingAsString(Document *doc, int scriptIndex) const;

    PoolList<CompiledFunctionOrExpression> *functionsAndExpressions;
    QQmlJS::FixedPoolArray<int> runtimeFunctionIndices;

    QQmlJS::FixedPoolArray<quint32> namedObjectsInComponent;
    int namedObjectsInComponentCount() const { return namedObjectsInComponent.size(); }
    const quint32 *namedObjectsInComponentTable() const { return namedObjectsInComponent.begin(); }

    bool hasFlag(QV4::CompiledData::Object::Flag flag) const { return flags & flag; }
    qint32 objectId() const { return id; }
    bool hasAliasAsDefaultProperty() const { return defaultPropertyIsAlias; }

private:
    friend struct ::QQmlIRLoader;

    PoolList<Property> *properties;
    PoolList<Alias> *aliases;
    PoolList<Enum> *qmlEnums;
    PoolList<Signal> *qmlSignals;
    PoolList<Binding> *bindings;
    PoolList<Function> *functions;
    PoolList<InlineComponent> *inlineComponents;
    PoolList<RequiredPropertyExtraData> *requiredPropertyExtraDatas;
};

struct Q_QML_COMPILER_EXPORT Pragma
{
    enum PragmaType
    {
        Singleton,
        Strict,
        ListPropertyAssignBehavior,
        ComponentBehavior,
        FunctionSignatureBehavior,
        NativeMethodBehavior,
        ValueTypeBehavior,
        Translator,
    };

    enum ListPropertyAssignBehaviorValue
    {
        Append,
        Replace,
        ReplaceIfNotDefault,
    };

    enum ComponentBehaviorValue
    {
        Unbound,
        Bound
    };

    enum FunctionSignatureBehaviorValue
    {
        Ignored,
        Enforced
    };

    enum NativeMethodBehaviorValue
    {
        AcceptThisObject,
        RejectThisObject
    };

    enum ValueTypeBehaviorValue
    {
        Copy        = 0x1,
        Addressable = 0x2,
        Assertable  = 0x4,
    };
    Q_DECLARE_FLAGS(ValueTypeBehaviorValues, ValueTypeBehaviorValue);

    PragmaType type;

    union {
        ListPropertyAssignBehaviorValue listPropertyAssignBehavior;
        ComponentBehaviorValue componentBehavior;
        FunctionSignatureBehaviorValue functionSignatureBehavior;
        NativeMethodBehaviorValue nativeMethodBehavior;
        ValueTypeBehaviorValues::Int valueTypeBehavior;
        uint translationContextIndex;
    };

    QV4::CompiledData::Location location;
};

struct Q_QML_COMPILER_EXPORT Document
{
    Document(const QString &fileName, const QString &finalUrl, bool debugMode);
    QString code;
    QQmlJS::Engine jsParserEngine;
    QV4::Compiler::Module jsModule;
    QList<const QV4::CompiledData::Import *> imports;
    QList<Pragma*> pragmas;
    QQmlJS::AST::UiProgram *program;
    QVector<Object*> objects;
    QV4::Compiler::JSUnitGenerator jsGenerator;

    QQmlRefPointer<QV4::CompiledData::CompilationUnit> javaScriptCompilationUnit;

    bool isSingleton() const {
        return std::any_of(pragmas.constBegin(), pragmas.constEnd(), [](const Pragma *pragma) {
            return pragma->type == Pragma::Singleton;
        });
    }

    int registerString(const QString &str) { return jsGenerator.registerString(str); }
    QString stringAt(int index) const { return jsGenerator.stringForIndex(index); }

    int objectCount() const {return objects.size();}
    Object* objectAt(int i) const {return objects.at(i);}
};

class Q_QML_COMPILER_EXPORT ScriptDirectivesCollector : public QQmlJS::Directives
{
    QmlIR::Document *document;
    QQmlJS::Engine *engine;
    QV4::Compiler::JSUnitGenerator *jsGenerator;

public:
    ScriptDirectivesCollector(QmlIR::Document *doc);

    void pragmaLibrary() override;
    void importFile(const QString &jsfile, const QString &module, int lineNumber, int column) override;
    void importModule(const QString &uri, const QString &version, const QString &module, int lineNumber, int column) override;
};

struct Q_QML_COMPILER_EXPORT IRBuilder : public QQmlJS::AST::Visitor
{
    Q_DECLARE_TR_FUNCTIONS(QQmlCodeGenerator)
public:
    IRBuilder();
    bool generateFromQml(const QString &code, const QString &url, Document *output);

    using QQmlJS::AST::Visitor::visit;
    using QQmlJS::AST::Visitor::endVisit;

    bool visit(QQmlJS::AST::UiArrayMemberList *ast) override;
    bool visit(QQmlJS::AST::UiImport *ast) override;
    bool visit(QQmlJS::AST::UiPragma *ast) override;
    bool visit(QQmlJS::AST::UiHeaderItemList *ast) override;
    bool visit(QQmlJS::AST::UiObjectInitializer *ast) override;
    bool visit(QQmlJS::AST::UiObjectMemberList *ast) override;
    bool visit(QQmlJS::AST::UiParameterList *ast) override;
    bool visit(QQmlJS::AST::UiProgram *) override;
    bool visit(QQmlJS::AST::UiQualifiedId *ast) override;
    bool visit(QQmlJS::AST::UiArrayBinding *ast) override;
    bool visit(QQmlJS::AST::UiObjectBinding *ast) override;
    bool visit(QQmlJS::AST::UiObjectDefinition *ast) override;
    bool visit(QQmlJS::AST::UiInlineComponent *ast) override;
    bool visit(QQmlJS::AST::UiEnumDeclaration *ast) override;
    bool visit(QQmlJS::AST::UiPublicMember *ast) override;
    bool visit(QQmlJS::AST::UiScriptBinding *ast) override;
    bool visit(QQmlJS::AST::UiSourceElement *ast) override;
    bool visit(QQmlJS::AST::UiRequired *ast) override;

    void throwRecursionDepthError() override
    {
        recordError(QQmlJS::SourceLocation(),
                    QStringLiteral("Maximum statement or expression depth exceeded"));
    }

    void accept(QQmlJS::AST::Node *node);

    // returns index in _objects
    bool defineQMLObject(
            int *objectIndex, QQmlJS::AST::UiQualifiedId *qualifiedTypeNameId,
            const QV4::CompiledData::Location &location,
            QQmlJS::AST::UiObjectInitializer *initializer, Object *declarationsOverride = nullptr);

    bool defineQMLObject(
            int *objectIndex, QQmlJS::AST::UiObjectDefinition *node,
            Object *declarationsOverride = nullptr)
    {
        const QQmlJS::SourceLocation location = node->qualifiedTypeNameId->firstSourceLocation();
        return defineQMLObject(
                    objectIndex, node->qualifiedTypeNameId,
                    { location.startLine, location.startColumn }, node->initializer,
                    declarationsOverride);
    }

    static QString asString(QQmlJS::AST::UiQualifiedId *node);
    QStringView asStringRef(QQmlJS::AST::Node *node);
    static QTypeRevision extractVersion(QStringView string);
    QStringView textRefAt(const QQmlJS::SourceLocation &loc) const
    { return QStringView(sourceCode).mid(loc.offset, loc.length); }
    QStringView textRefAt(const QQmlJS::SourceLocation &first,
                         const QQmlJS::SourceLocation &last) const;

    virtual void setBindingValue(QV4::CompiledData::Binding *binding,
                                 QQmlJS::AST::Statement *statement, QQmlJS::AST::Node *parentNode);
    void tryGeneratingTranslationBinding(QStringView base, QQmlJS::AST::ArgumentList *args, QV4::CompiledData::Binding *binding);

    void appendBinding(QQmlJS::AST::UiQualifiedId *name, QQmlJS::AST::Statement *value,
                       QQmlJS::AST::Node *parentNode);
    void appendBinding(QQmlJS::AST::UiQualifiedId *name, int objectIndex, bool isOnAssignment = false);
    void appendBinding(const QQmlJS::SourceLocation &qualifiedNameLocation,
                       const QQmlJS::SourceLocation &nameLocation, quint32 propertyNameIndex,
                       QQmlJS::AST::Statement *value, QQmlJS::AST::Node *parentNode);
    void appendBinding(const QQmlJS::SourceLocation &qualifiedNameLocation,
                       const QQmlJS::SourceLocation &nameLocation, quint32 propertyNameIndex,
                       int objectIndex, bool isListItem = false, bool isOnAssignment = false);

    bool appendAlias(QQmlJS::AST::UiPublicMember *node);

    enum class IsQmlFunction { Yes, No };
    virtual void registerFunctionExpr(QQmlJS::AST::FunctionExpression *fexp, IsQmlFunction);

    Object *bindingsTarget() const;

    bool setId(const QQmlJS::SourceLocation &idLocation, QQmlJS::AST::Statement *value);

    // resolves qualified name (font.pixelSize for example) and returns the last name along
    // with the object any right-hand-side of a binding should apply to.
    bool resolveQualifiedId(QQmlJS::AST::UiQualifiedId **nameToResolve, Object **object, bool onAssignment = false);

    void recordError(const QQmlJS::SourceLocation &location, const QString &description);

    quint32 registerString(const QString &str) const { return jsGenerator->registerString(str); }
    template <typename _Tp> _Tp *New() { return pool->New<_Tp>(); }

    QString stringAt(int index) const { return jsGenerator->stringForIndex(index); }

    static bool isStatementNodeScript(QQmlJS::AST::Statement *statement);
    static bool isRedundantNullInitializerForPropertyDeclaration(Property *property, QQmlJS::AST::Statement *statement);

    QString sanityCheckFunctionNames(Object *obj, QQmlJS::SourceLocation *errorLocation);

    QList<QQmlJS::DiagnosticMessage> errors;

    QSet<QString> inlineComponentsNames;

    QList<const QV4::CompiledData::Import *> _imports;
    QList<Pragma*> _pragmas;
    QVector<Object*> _objects;

    QV4::CompiledData::TypeReferenceMap _typeReferences;

    Object *_object;
    Property *_propertyDeclaration;

    QQmlJS::MemoryPool *pool;
    QString sourceCode;
    QV4::Compiler::JSUnitGenerator *jsGenerator;

    bool insideInlineComponent = false;
};

struct Q_QML_COMPILER_EXPORT QmlUnitGenerator
{
    void generate(Document &output, const QV4::CompiledData::DependentTypesHasher &dependencyHasher = QV4::CompiledData::DependentTypesHasher());

private:
    typedef bool (Binding::*BindingFilter)() const;
    char *writeBindings(char *bindingPtr, const Object *o, BindingFilter filter) const;
};

struct Q_QML_COMPILER_EXPORT JSCodeGen : public QV4::Compiler::Codegen
{
    JSCodeGen(Document *document,
              QV4::Compiler::CodegenWarningInterface *iface =
                      QV4::Compiler::defaultCodegenWarningInterface(),
              bool storeSourceLocations = false);

    // Returns mapping from input functions to index in IR::Module::functions / compiledData->runtimeFunctions
    QVector<int>
    generateJSCodeForFunctionsAndBindings(const QList<CompiledFunctionOrExpression> &functions);

    bool generateRuntimeFunctions(QmlIR::Object *object);

private:
    Document *document;
};

// RegisterStringN ~= std::function<int(QStringView)>
// FinalizeTranlationData ~= std::function<void(QV4::CompiledData::Binding::ValueType, QV4::CompiledData::TranslationData)>
/*
    \internal
    \a base: name of the potential translation function
    \a args: arguments to the function call
    \a registerMainString: Takes the first argument passed to the translation function, and it's
    result will be stored in a TranslationData's stringIndex for translation bindings and in numbeIndex
    for string bindings.
    \a registerCommentString: Takes the comment argument passed to some of the translation functions.
    Result will be stored in a TranslationData's commentIndex
    \a finalizeTranslationData: Takes the type of the binding and the previously set up TranslationData
 */
template<
        typename RegisterMainString,
        typename RegisterCommentString,
        typename RegisterContextString,
        typename FinalizeTranslationData>
void tryGeneratingTranslationBindingBase(QStringView base, QQmlJS::AST::ArgumentList *args,
                                         RegisterMainString registerMainString,
                                         RegisterCommentString registerCommentString,
                                         RegisterContextString registerContextString,
                                         FinalizeTranslationData finalizeTranslationData
                                         )
{
    if (base == QLatin1String("qsTr")) {
        QV4::CompiledData::TranslationData translationData;
        translationData.number = -1;

        // empty string
        translationData.commentIndex = 0;

        // No context (not empty string)
        translationData.contextIndex = QV4::CompiledData::TranslationData::NoContextIndex;

        if (!args || !args->expression)
            return; // no arguments, stop

        QStringView translation;
        if (QQmlJS::AST::StringLiteral *arg1 = QQmlJS::AST::cast<QQmlJS::AST::StringLiteral *>(args->expression)) {
            translation = arg1->value;
        } else {
            return; // first argument is not a string, stop
        }

        translationData.stringIndex = registerMainString(translation);

        args = args->next;

        if (args) {
            QQmlJS::AST::StringLiteral *arg2 = QQmlJS::AST::cast<QQmlJS::AST::StringLiteral *>(args->expression);
            if (!arg2)
                return; // second argument is not a string, stop
            translationData.commentIndex = registerCommentString(arg2->value);

            args = args->next;
            if (args) {
                if (QQmlJS::AST::NumericLiteral *arg3 = QQmlJS::AST::cast<QQmlJS::AST::NumericLiteral *>(args->expression)) {
                    translationData.number = int(arg3->value);
                    args = args->next;
                } else {
                    return; // third argument is not a translation number, stop
                }
            }
        }

        if (args)
            return; // too many arguments, stop

        finalizeTranslationData(QV4::CompiledData::Binding::Type_Translation, translationData);
    } else if (base == QLatin1String("qsTrId")) {
        QV4::CompiledData::TranslationData translationData;
        translationData.number = -1;

        // empty string, but unused
        translationData.commentIndex = 0;

        // No context (not empty string)
        translationData.contextIndex = QV4::CompiledData::TranslationData::NoContextIndex;

        if (!args || !args->expression)
            return; // no arguments, stop

        QStringView id;
        if (QQmlJS::AST::StringLiteral *arg1 = QQmlJS::AST::cast<QQmlJS::AST::StringLiteral *>(args->expression)) {
            id = arg1->value;
        } else {
            return; // first argument is not a string, stop
        }
        translationData.stringIndex = registerMainString(id);

        args = args->next;

        if (args) {
            if (QQmlJS::AST::NumericLiteral *arg3 = QQmlJS::AST::cast<QQmlJS::AST::NumericLiteral *>(args->expression)) {
                translationData.number = int(arg3->value);
                args = args->next;
            } else {
                return; // third argument is not a translation number, stop
            }
        }

        if (args)
            return; // too many arguments, stop

        finalizeTranslationData(QV4::CompiledData::Binding::Type_TranslationById, translationData);
    } else if (base == QLatin1String("QT_TR_NOOP") || base == QLatin1String("QT_TRID_NOOP")) {
        if (!args || !args->expression)
            return; // no arguments, stop

        QStringView str;
        if (QQmlJS::AST::StringLiteral *arg1 = QQmlJS::AST::cast<QQmlJS::AST::StringLiteral *>(args->expression)) {
            str = arg1->value;
        } else {
            return; // first argument is not a string, stop
        }

        args = args->next;
        // QT_TR_NOOP can have a disambiguation string, QT_TRID_NOOP can't
        if (args && base == QLatin1String("QT_TR_NOOP")) {
            // we have a disambiguation string; we don't need to do anything with it
            if (QQmlJS::AST::cast<QQmlJS::AST::StringLiteral *>(args->expression))
                args = args->next;
            else // second argument is not a string, stop
                return;
        }

        if (args)
            return; // too many arguments, stop

        QV4::CompiledData::TranslationData translationData;
        translationData.number = registerMainString(str);
        finalizeTranslationData(QV4::CompiledData::Binding::Type_String, translationData);
    } else if (base == QLatin1String("QT_TRANSLATE_NOOP")) {
        if (!args || !args->expression)
            return; // no arguments, stop

        args = args->next;
        if (!args || !args->expression)
            return; // no second arguments, stop

        QStringView str;
        if (QQmlJS::AST::StringLiteral *arg2 = QQmlJS::AST::cast<QQmlJS::AST::StringLiteral *>(args->expression)) {
            str = arg2->value;
        } else {
            return; // first argument is not a string, stop
        }

        args = args->next;
        if (args) {
            // we have a disambiguation string; we don't need to do anything with it
            if (QQmlJS::AST::cast<QQmlJS::AST::StringLiteral *>(args->expression))
                args = args->next;
            else // third argument is not a string, stop
                return;
        }

        if (args)
            return; // too many arguments, stop

        QV4::CompiledData::TranslationData fakeTranslationData;
        fakeTranslationData.number = registerMainString(str);
        finalizeTranslationData(QV4::CompiledData::Binding::Type_String, fakeTranslationData);
    } else if (base == QLatin1String("qsTranslate")) {
        QV4::CompiledData::TranslationData translationData;
        translationData.number = -1;
        translationData.commentIndex = 0; // empty string

        if (!args || !args->next)
            return; // less than 2 arguments, stop

        QStringView translation;
        if (QQmlJS::AST::StringLiteral *arg1
                = QQmlJS::AST::cast<QQmlJS::AST::StringLiteral *>(args->expression)) {
            translation = arg1->value;
        } else {
            return; // first argument is not a string, stop
        }

        translationData.contextIndex = registerContextString(translation);

        args = args->next;
        Q_ASSERT(args);

        QQmlJS::AST::StringLiteral *arg2
                = QQmlJS::AST::cast<QQmlJS::AST::StringLiteral *>(args->expression);
        if (!arg2)
            return; // second argument is not a string, stop
        translationData.stringIndex = registerMainString(arg2->value);

        args = args->next;
        if (args) {
            QQmlJS::AST::StringLiteral *arg3
                    = QQmlJS::AST::cast<QQmlJS::AST::StringLiteral *>(args->expression);
            if (!arg3)
                return; // third argument is not a string, stop
            translationData.commentIndex = registerCommentString(arg3->value);

            args = args->next;
            if (args) {
                if (QQmlJS::AST::NumericLiteral *arg4
                        = QQmlJS::AST::cast<QQmlJS::AST::NumericLiteral *>(args->expression)) {
                    translationData.number = int(arg4->value);
                    args = args->next;
                } else {
                    return; // fourth argument is not a translation number, stop
                }
            }
        }

        if (args)
            return; // too many arguments, stop

        finalizeTranslationData(QV4::CompiledData::Binding::Type_Translation, translationData);
    }
}

} // namespace QmlIR

QT_END_NAMESPACE

#endif // QQMLIRBUILDER_P_H
