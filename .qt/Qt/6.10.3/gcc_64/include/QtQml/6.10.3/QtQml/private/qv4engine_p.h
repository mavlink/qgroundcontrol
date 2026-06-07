// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant
#ifndef QV4ENGINE_H
#define QV4ENGINE_H

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

#include <private/qintrusivelist_p.h>
#include <private/qqmldelayedcallqueue_p.h>
#include <private/qqmlrefcount_p.h>
#include <private/qv4compileddata_p.h>
#include <private/qv4context_p.h>
#include <private/qv4enginebase_p.h>
#include <private/qv4executablecompilationunit_p.h>
#include <private/qv4global_p.h>
#include <private/qv4stacklimits_p.h>

#include <QtCore/qelapsedtimer.h>
#include <QtCore/qmutex.h>
#include <QtCore/qprocessordetection.h>
#include <QtCore/qset.h>

namespace WTF {
class BumpPointerAllocator;
class PageAllocation;
}

#define V4_DEFINE_EXTENSION(dataclass, datafunction) \
    static inline dataclass *datafunction(QV4::ExecutionEngine *engine) \
    { \
        static int extensionId = -1; \
        if (extensionId == -1) { \
            QV4::ExecutionEngine::registrationMutex()->lock(); \
            if (extensionId == -1) \
                extensionId = QV4::ExecutionEngine::registerExtension(); \
            QV4::ExecutionEngine::registrationMutex()->unlock(); \
        } \
        dataclass *rv = (dataclass *)engine->extensionData(extensionId); \
        if (!rv) { \
            rv = new dataclass(engine); \
            engine->setExtensionData(extensionId, rv); \
        } \
        return rv; \
    } \


QT_BEGIN_NAMESPACE

#if QT_CONFIG(qml_network)
class QNetworkAccessManager;

namespace QV4 {
struct QObjectMethod;
namespace detail {
QNetworkAccessManager *getNetworkAccessManager(ExecutionEngine *engine);
}
}
#else
namespace QV4 { struct QObjectMethod; }
#endif // qml_network

// Used to allow a QObject method take and return raw V4 handles without having to expose
// 48 in the public API.
// Use like this:
//     class MyClass : public QObject {
//         Q_OBJECT
//         ...
//         Q_INVOKABLE void myMethod(QQmlV4FunctionPtr);
//     };
// The QQmlV8Function - and consequently the arguments and return value - only remains
// valid during the call.  If the return value isn't set within myMethod(), the will return
// undefined.

class QQmlV4Function
{
public:
    int length() const { return callData->argc(); }
    QV4::ReturnedValue operator[](int idx) const { return (idx < callData->argc() ? callData->args[idx].asReturnedValue() : QV4::Encode::undefined()); }
    void setReturnValue(QV4::ReturnedValue rv) { *retVal = rv; }
    QV4::ExecutionEngine *v4engine() const { return e; }
private:
    friend struct QV4::QObjectMethod;
    QQmlV4Function();
    QQmlV4Function(const QQmlV4Function &);
    QQmlV4Function &operator=(const QQmlV4Function &);

    QQmlV4Function(QV4::CallData *callData, QV4::Value *retVal, QV4::ExecutionEngine *e)
        : callData(callData), retVal(retVal), e(e)
    {
        callData->thisObject = QV4::Encode::undefined();
    }

    QV4::CallData *callData;
    QV4::Value *retVal;
    QV4::ExecutionEngine *e;
};

class QQmlError;
class QJSEngine;
class QQmlEngine;
class QQmlContextData;
class QQmlTypeLoader;

namespace QV4 {
namespace Debugging {
class Debugger;
} // namespace Debugging
namespace Profiling {
class Profiler;
} // namespace Profiling
namespace CompiledData {
struct CompilationUnit;
}

namespace Heap {
struct Module;
};

struct Function;

namespace Promise {
class ReactionHandler;
};

struct Q_QML_EXPORT ExecutionEngine : public EngineBase
{
private:
    friend struct ExecutionContextSaver;
    friend struct ExecutionContext;
    friend struct Heap::ExecutionContext;
public:
    enum class DiskCache : quint8 {
        Disabled    = 0,
        AotByteCode = 1 << 0,
        AotNative   = 1 << 1,
        QmlcRead    = 1 << 2,
        QmlcWrite   = 1 << 3,
        Aot         = AotByteCode | AotNative,
        Qmlc        = QmlcRead | QmlcWrite,
        Enabled     = Aot | Qmlc,

    };

    Q_DECLARE_FLAGS(DiskCacheOptions, DiskCache);

    ExecutableAllocator *executableAllocator = nullptr;
    ExecutableAllocator *regExpAllocator = nullptr;

    WTF::BumpPointerAllocator *bumperPointerAllocator = nullptr; // Used by Yarr Regex engine.

    WTF::PageAllocation *jsStack = nullptr;

    WTF::PageAllocation *gcStack = nullptr;

    QML_NEARLY_ALWAYS_INLINE Value *jsAlloca(int nValues) {
        Value *ptr = jsStackTop;
        jsStackTop = ptr + nValues;
        return ptr;
    }

    Function *globalCode = nullptr;

    QJSEngine *jsEngine() const { return publicEngine; }
    QQmlEngine *qmlEngine() const { return m_qmlEngine; }
    QJSEngine *publicEngine = nullptr;

    template<typename TypeLoader = QQmlTypeLoader>
    TypeLoader *typeLoader()
    {
        if (m_qmlEngine)
            return TypeLoader::get(m_qmlEngine);
        return nullptr;
    }

    enum JSObjects {
        RootContext,
        ScriptContext,
        IntegerNull, // Has to come after the RootContext to make the context stack safe
        ObjectProto,
        SymbolProto,
        ArrayProto,
        ArrayProtoValues,
        PropertyListProto,
        StringProto,
        NumberProto,
        BooleanProto,
        DateProto,
        FunctionProto,
        GeneratorProto,
        RegExpProto,
        ErrorProto,
        EvalErrorProto,
        RangeErrorProto,
        ReferenceErrorProto,
        SyntaxErrorProto,
        TypeErrorProto,
        URIErrorProto,
        PromiseProto,
        VariantProto,
        VariantAssociationProto,
        SequenceProto,
        SharedArrayBufferProto,
        ArrayBufferProto,
        DataViewProto,
        WeakSetProto,
        SetProto,
        WeakMapProto,
        MapProto,
        IntrinsicTypedArrayProto,
        ValueTypeProto,
        TypeWrapperProto,
        SignalHandlerProto,
        IteratorProto,
        ForInIteratorProto,
        SetIteratorProto,
        MapIteratorProto,
        ArrayIteratorProto,
        StringIteratorProto,
        UrlProto,
        UrlSearchParamsProto,

        Object_Ctor,
        String_Ctor,
        Symbol_Ctor,
        Number_Ctor,
        Boolean_Ctor,
        Array_Ctor,
        Function_Ctor,
        GeneratorFunction_Ctor,
        Date_Ctor,
        RegExp_Ctor,
        Error_Ctor,
        EvalError_Ctor,
        RangeError_Ctor,
        ReferenceError_Ctor,
        SyntaxError_Ctor,
        TypeError_Ctor,
        URIError_Ctor,
        SharedArrayBuffer_Ctor,
        Promise_Ctor,
        ArrayBuffer_Ctor,
        DataView_Ctor,
        WeakSet_Ctor,
        Set_Ctor,
        WeakMap_Ctor,
        Map_Ctor,
        IntrinsicTypedArray_Ctor,
        Url_Ctor,
        UrlSearchParams_Ctor,

        GetSymbolSpecies,

        Eval_Function,
        GetStack_Function,
        ThrowerObject,
        NJSObjects
    };
    Value *jsObjects = nullptr;
    enum { NTypedArrayTypes = 9 }; // == TypedArray::NValues, avoid header dependency

    ExecutionContext *rootContext() const { return reinterpret_cast<ExecutionContext *>(jsObjects + RootContext); }
    ExecutionContext *scriptContext() const { return reinterpret_cast<ExecutionContext *>(jsObjects + ScriptContext); }
    void setScriptContext(ReturnedValue c) { jsObjects[ScriptContext] = c; }
    FunctionObject *objectCtor() const { return reinterpret_cast<FunctionObject *>(jsObjects + Object_Ctor); }
    FunctionObject *stringCtor() const { return reinterpret_cast<FunctionObject *>(jsObjects + String_Ctor); }
    FunctionObject *symbolCtor() const { return reinterpret_cast<FunctionObject *>(jsObjects + Symbol_Ctor); }
    FunctionObject *numberCtor() const { return reinterpret_cast<FunctionObject *>(jsObjects + Number_Ctor); }
    FunctionObject *booleanCtor() const { return reinterpret_cast<FunctionObject *>(jsObjects + Boolean_Ctor); }
    FunctionObject *arrayCtor() const { return reinterpret_cast<FunctionObject *>(jsObjects + Array_Ctor); }
    FunctionObject *functionCtor() const { return reinterpret_cast<FunctionObject *>(jsObjects + Function_Ctor); }
    FunctionObject *generatorFunctionCtor() const { return reinterpret_cast<FunctionObject *>(jsObjects + GeneratorFunction_Ctor); }
    FunctionObject *dateCtor() const { return reinterpret_cast<FunctionObject *>(jsObjects + Date_Ctor); }
    FunctionObject *regExpCtor() const { return reinterpret_cast<FunctionObject *>(jsObjects + RegExp_Ctor); }
    FunctionObject *errorCtor() const { return reinterpret_cast<FunctionObject *>(jsObjects + Error_Ctor); }
    FunctionObject *evalErrorCtor() const { return reinterpret_cast<FunctionObject *>(jsObjects + EvalError_Ctor); }
    FunctionObject *rangeErrorCtor() const { return reinterpret_cast<FunctionObject *>(jsObjects + RangeError_Ctor); }
    FunctionObject *referenceErrorCtor() const { return reinterpret_cast<FunctionObject *>(jsObjects + ReferenceError_Ctor); }
    FunctionObject *syntaxErrorCtor() const { return reinterpret_cast<FunctionObject *>(jsObjects + SyntaxError_Ctor); }
    FunctionObject *typeErrorCtor() const { return reinterpret_cast<FunctionObject *>(jsObjects + TypeError_Ctor); }
    FunctionObject *uRIErrorCtor() const { return reinterpret_cast<FunctionObject *>(jsObjects + URIError_Ctor); }
    FunctionObject *sharedArrayBufferCtor() const { return reinterpret_cast<FunctionObject *>(jsObjects + SharedArrayBuffer_Ctor); }
    FunctionObject *promiseCtor() const { return reinterpret_cast<FunctionObject *>(jsObjects + Promise_Ctor); }
    FunctionObject *arrayBufferCtor() const { return reinterpret_cast<FunctionObject *>(jsObjects + ArrayBuffer_Ctor); }
    FunctionObject *dataViewCtor() const { return reinterpret_cast<FunctionObject *>(jsObjects + DataView_Ctor); }
    FunctionObject *weakSetCtor() const { return reinterpret_cast<FunctionObject *>(jsObjects + WeakSet_Ctor); }
    FunctionObject *setCtor() const { return reinterpret_cast<FunctionObject *>(jsObjects + Set_Ctor); }
    FunctionObject *weakMapCtor() const { return reinterpret_cast<FunctionObject *>(jsObjects + WeakMap_Ctor); }
    FunctionObject *mapCtor() const { return reinterpret_cast<FunctionObject *>(jsObjects + Map_Ctor); }
    FunctionObject *intrinsicTypedArrayCtor() const { return reinterpret_cast<FunctionObject *>(jsObjects + IntrinsicTypedArray_Ctor); }
    FunctionObject *urlCtor() const
    {
        return reinterpret_cast<FunctionObject *>(jsObjects + Url_Ctor);
    }
    FunctionObject *urlSearchParamsCtor() const
    {
        return reinterpret_cast<FunctionObject *>(jsObjects + UrlSearchParams_Ctor);
    }
    FunctionObject *typedArrayCtors = nullptr;

    FunctionObject *getSymbolSpecies() const { return reinterpret_cast<FunctionObject *>(jsObjects + GetSymbolSpecies); }

    Object *objectPrototype() const { return reinterpret_cast<Object *>(jsObjects + ObjectProto); }
    Object *symbolPrototype() const { return reinterpret_cast<Object *>(jsObjects + SymbolProto); }
    Object *arrayPrototype() const { return reinterpret_cast<Object *>(jsObjects + ArrayProto); }
    Object *arrayProtoValues() const { return reinterpret_cast<Object *>(jsObjects + ArrayProtoValues); }
    Object *propertyListPrototype() const { return reinterpret_cast<Object *>(jsObjects + PropertyListProto); }
    Object *stringPrototype() const { return reinterpret_cast<Object *>(jsObjects + StringProto); }
    Object *numberPrototype() const { return reinterpret_cast<Object *>(jsObjects + NumberProto); }
    Object *booleanPrototype() const { return reinterpret_cast<Object *>(jsObjects + BooleanProto); }
    Object *datePrototype() const { return reinterpret_cast<Object *>(jsObjects + DateProto); }
    Object *functionPrototype() const { return reinterpret_cast<Object *>(jsObjects + FunctionProto); }
    Object *generatorPrototype() const { return reinterpret_cast<Object *>(jsObjects + GeneratorProto); }
    Object *regExpPrototype() const { return reinterpret_cast<Object *>(jsObjects + RegExpProto); }
    Object *errorPrototype() const { return reinterpret_cast<Object *>(jsObjects + ErrorProto); }
    Object *evalErrorPrototype() const { return reinterpret_cast<Object *>(jsObjects + EvalErrorProto); }
    Object *rangeErrorPrototype() const { return reinterpret_cast<Object *>(jsObjects + RangeErrorProto); }
    Object *referenceErrorPrototype() const { return reinterpret_cast<Object *>(jsObjects + ReferenceErrorProto); }
    Object *syntaxErrorPrototype() const { return reinterpret_cast<Object *>(jsObjects + SyntaxErrorProto); }
    Object *typeErrorPrototype() const { return reinterpret_cast<Object *>(jsObjects + TypeErrorProto); }
    Object *uRIErrorPrototype() const { return reinterpret_cast<Object *>(jsObjects + URIErrorProto); }
    Object *promisePrototype() const { return reinterpret_cast<Object *>(jsObjects + PromiseProto); }
    Object *variantPrototype() const { return reinterpret_cast<Object *>(jsObjects + VariantProto); }
    Object *variantAssociationPrototype() const { return reinterpret_cast<Object *>(jsObjects + VariantAssociationProto); }
    Object *sequencePrototype() const { return reinterpret_cast<Object *>(jsObjects + SequenceProto); }

    Object *sharedArrayBufferPrototype() const { return reinterpret_cast<Object *>(jsObjects + SharedArrayBufferProto); }
    Object *arrayBufferPrototype() const { return reinterpret_cast<Object *>(jsObjects + ArrayBufferProto); }
    Object *dataViewPrototype() const { return reinterpret_cast<Object *>(jsObjects + DataViewProto); }
    Object *weakSetPrototype() const { return reinterpret_cast<Object *>(jsObjects + WeakSetProto); }
    Object *setPrototype() const { return reinterpret_cast<Object *>(jsObjects + SetProto); }
    Object *weakMapPrototype() const { return reinterpret_cast<Object *>(jsObjects + WeakMapProto); }
    Object *mapPrototype() const { return reinterpret_cast<Object *>(jsObjects + MapProto); }
    Object *intrinsicTypedArrayPrototype() const { return reinterpret_cast<Object *>(jsObjects + IntrinsicTypedArrayProto); }
    Object *typedArrayPrototype = nullptr;

    Object *valueTypeWrapperPrototype() const { return reinterpret_cast<Object *>(jsObjects + ValueTypeProto); }
    Object *signalHandlerPrototype() const { return reinterpret_cast<Object *>(jsObjects + SignalHandlerProto); }
    Object *typeWrapperPrototype() const { return reinterpret_cast<Object *>(jsObjects + TypeWrapperProto); }
    Object *iteratorPrototype() const { return reinterpret_cast<Object *>(jsObjects + IteratorProto); }
    Object *forInIteratorPrototype() const { return reinterpret_cast<Object *>(jsObjects + ForInIteratorProto); }
    Object *setIteratorPrototype() const { return reinterpret_cast<Object *>(jsObjects + SetIteratorProto); }
    Object *mapIteratorPrototype() const { return reinterpret_cast<Object *>(jsObjects + MapIteratorProto); }
    Object *arrayIteratorPrototype() const { return reinterpret_cast<Object *>(jsObjects + ArrayIteratorProto); }
    Object *stringIteratorPrototype() const { return reinterpret_cast<Object *>(jsObjects + StringIteratorProto); }
    Object *urlPrototype() const { return reinterpret_cast<Object *>(jsObjects + UrlProto); }
    Object *urlSearchParamsPrototype() const { return reinterpret_cast<Object *>(jsObjects + UrlSearchParamsProto); }

    EvalFunction *evalFunction() const { return reinterpret_cast<EvalFunction *>(jsObjects + Eval_Function); }
    FunctionObject *getStackFunction() const { return reinterpret_cast<FunctionObject *>(jsObjects + GetStack_Function); }
    FunctionObject *thrower() const { return reinterpret_cast<FunctionObject *>(jsObjects + ThrowerObject); }

#if QT_CONFIG(qml_network)
    QNetworkAccessManager* (*networkAccessManager)(ExecutionEngine*)  = detail::getNetworkAccessManager;
#endif

    enum JSStrings {
        String_Empty,
        String_undefined,
        String_null,
        String_true,
        String_false,
        String_boolean,
        String_number,
        String_string,
        String_default,
        String_symbol,
        String_object,
        String_function,
        String_length,
        String_prototype,
        String_constructor,
        String_arguments,
        String_caller,
        String_callee,
        String_this,
        String___proto__,
        String_enumerable,
        String_configurable,
        String_writable,
        String_value,
        String_get,
        String_set,
        String_eval,
        String_uintMax,
        String_name,
        String_index,
        String_input,
        String_toString,
        String_toLocaleString,
        String_destroy,
        String_valueOf,
        String_byteLength,
        String_byteOffset,
        String_buffer,
        String_lastIndex,
        String_next,
        String_done,
        String_return,
        String_throw,
        String_global,
        String_ignoreCase,
        String_multiline,
        String_unicode,
        String_sticky,
        String_source,
        String_flags,

        NJSStrings
    };
    Value *jsStrings = nullptr;

    enum JSSymbols {
        Symbol_hasInstance,
        Symbol_isConcatSpreadable,
        Symbol_iterator,
        Symbol_match,
        Symbol_replace,
        Symbol_search,
        Symbol_species,
        Symbol_split,
        Symbol_toPrimitive,
        Symbol_toStringTag,
        Symbol_unscopables,
        Symbol_revokableProxy,
        NJSSymbols
    };
    Value *jsSymbols = nullptr;

    String *id_empty() const { return reinterpret_cast<String *>(jsStrings + String_Empty); }
    String *id_undefined() const { return reinterpret_cast<String *>(jsStrings + String_undefined); }
    String *id_null() const { return reinterpret_cast<String *>(jsStrings + String_null); }
    String *id_true() const { return reinterpret_cast<String *>(jsStrings + String_true); }
    String *id_false() const { return reinterpret_cast<String *>(jsStrings + String_false); }
    String *id_boolean() const { return reinterpret_cast<String *>(jsStrings + String_boolean); }
    String *id_number() const { return reinterpret_cast<String *>(jsStrings + String_number); }
    String *id_string() const { return reinterpret_cast<String *>(jsStrings + String_string); }
    String *id_default() const { return reinterpret_cast<String *>(jsStrings + String_default); }
    String *id_symbol() const { return reinterpret_cast<String *>(jsStrings + String_symbol); }
    String *id_object() const { return reinterpret_cast<String *>(jsStrings + String_object); }
    String *id_function() const { return reinterpret_cast<String *>(jsStrings + String_function); }
    String *id_length() const { return reinterpret_cast<String *>(jsStrings + String_length); }
    String *id_prototype() const { return reinterpret_cast<String *>(jsStrings + String_prototype); }
    String *id_constructor() const { return reinterpret_cast<String *>(jsStrings + String_constructor); }
    String *id_arguments() const { return reinterpret_cast<String *>(jsStrings + String_arguments); }
    String *id_caller() const { return reinterpret_cast<String *>(jsStrings + String_caller); }
    String *id_callee() const { return reinterpret_cast<String *>(jsStrings + String_callee); }
    String *id_this() const { return reinterpret_cast<String *>(jsStrings + String_this); }
    String *id___proto__() const { return reinterpret_cast<String *>(jsStrings + String___proto__); }
    String *id_enumerable() const { return reinterpret_cast<String *>(jsStrings + String_enumerable); }
    String *id_configurable() const { return reinterpret_cast<String *>(jsStrings + String_configurable); }
    String *id_writable() const { return reinterpret_cast<String *>(jsStrings + String_writable); }
    String *id_value() const { return reinterpret_cast<String *>(jsStrings + String_value); }
    String *id_get() const { return reinterpret_cast<String *>(jsStrings + String_get); }
    String *id_set() const { return reinterpret_cast<String *>(jsStrings + String_set); }
    String *id_eval() const { return reinterpret_cast<String *>(jsStrings + String_eval); }
    String *id_uintMax() const { return reinterpret_cast<String *>(jsStrings + String_uintMax); }
    String *id_name() const { return reinterpret_cast<String *>(jsStrings + String_name); }
    String *id_index() const { return reinterpret_cast<String *>(jsStrings + String_index); }
    String *id_input() const { return reinterpret_cast<String *>(jsStrings + String_input); }
    String *id_toString() const { return reinterpret_cast<String *>(jsStrings + String_toString); }
    String *id_toLocaleString() const { return reinterpret_cast<String *>(jsStrings + String_toLocaleString); }
    String *id_destroy() const { return reinterpret_cast<String *>(jsStrings + String_destroy); }
    String *id_valueOf() const { return reinterpret_cast<String *>(jsStrings + String_valueOf); }
    String *id_byteLength() const { return reinterpret_cast<String *>(jsStrings + String_byteLength); }
    String *id_byteOffset() const { return reinterpret_cast<String *>(jsStrings + String_byteOffset); }
    String *id_buffer() const { return reinterpret_cast<String *>(jsStrings + String_buffer); }
    String *id_lastIndex() const { return reinterpret_cast<String *>(jsStrings + String_lastIndex); }
    String *id_next() const { return reinterpret_cast<String *>(jsStrings + String_next); }
    String *id_done() const { return reinterpret_cast<String *>(jsStrings + String_done); }
    String *id_return() const { return reinterpret_cast<String *>(jsStrings + String_return); }
    String *id_throw() const { return reinterpret_cast<String *>(jsStrings + String_throw); }
    String *id_global() const { return reinterpret_cast<String *>(jsStrings + String_global); }
    String *id_ignoreCase() const { return reinterpret_cast<String *>(jsStrings + String_ignoreCase); }
    String *id_multiline() const { return reinterpret_cast<String *>(jsStrings + String_multiline); }
    String *id_unicode() const { return reinterpret_cast<String *>(jsStrings + String_unicode); }
    String *id_sticky() const { return reinterpret_cast<String *>(jsStrings + String_sticky); }
    String *id_source() const { return reinterpret_cast<String *>(jsStrings + String_source); }
    String *id_flags() const { return reinterpret_cast<String *>(jsStrings + String_flags); }

    Symbol *symbol_hasInstance() const { return reinterpret_cast<Symbol *>(jsSymbols + Symbol_hasInstance); }
    Symbol *symbol_isConcatSpreadable() const { return reinterpret_cast<Symbol *>(jsSymbols + Symbol_isConcatSpreadable); }
    Symbol *symbol_iterator() const { return reinterpret_cast<Symbol *>(jsSymbols + Symbol_iterator); }
    Symbol *symbol_match() const { return reinterpret_cast<Symbol *>(jsSymbols + Symbol_match); }
    Symbol *symbol_replace() const { return reinterpret_cast<Symbol *>(jsSymbols + Symbol_replace); }
    Symbol *symbol_search() const { return reinterpret_cast<Symbol *>(jsSymbols + Symbol_search); }
    Symbol *symbol_species() const { return reinterpret_cast<Symbol *>(jsSymbols + Symbol_species); }
    Symbol *symbol_split() const { return reinterpret_cast<Symbol *>(jsSymbols + Symbol_split); }
    Symbol *symbol_toPrimitive() const { return reinterpret_cast<Symbol *>(jsSymbols + Symbol_toPrimitive); }
    Symbol *symbol_toStringTag() const { return reinterpret_cast<Symbol *>(jsSymbols + Symbol_toStringTag); }
    Symbol *symbol_unscopables() const { return reinterpret_cast<Symbol *>(jsSymbols + Symbol_unscopables); }
    Symbol *symbol_revokableProxy() const { return reinterpret_cast<Symbol *>(jsSymbols + Symbol_revokableProxy); }

    quint32 m_engineId = 0;

    RegExpCache *regExpCache = nullptr;

    // Scarce resources are "exceptionally high cost" QVariant types where allowing the
    // normal JavaScript GC to clean them up is likely to lead to out-of-memory or other
    // out-of-resource situations.  When such a resource is passed into JavaScript we
    // add it to the scarceResources list and it is destroyed when we return from the
    // JavaScript execution that created it.  The user can prevent this behavior by
    // calling preserve() on the object which removes it from this scarceResource list.
    class ScarceResourceData {
    public:
        ScarceResourceData() = default;
        ScarceResourceData(const QMetaType type, const void *data) : data(type, data) {}
        QVariant data;
        QIntrusiveListNode node;
    };
    QIntrusiveList<ScarceResourceData, &ScarceResourceData::node> scarceResources;

    // Normally the JS wrappers for QObjects are stored in the QQmlData/QObjectPrivate,
    // but any time a QObject is wrapped a second time in another engine, we have to do
    // bookkeeping.
    MultiplyWrappedQObjectMap *m_multiplyWrappedQObjects = nullptr;
#if QT_CONFIG(qml_jit)
    const bool m_canAllocateExecutableMemory = false;
#endif

    quintptr protoIdCount = 1;

    ExecutionEngine(QJSEngine *jsEngine = nullptr);
    ~ExecutionEngine();

#if !QT_CONFIG(qml_debug)
    QV4::Debugging::Debugger *debugger() const { return nullptr; }
    QV4::Profiling::Profiler *profiler() const { return nullptr; }

    void setDebugger(Debugging::Debugger *) {}
    void setProfiler(Profiling::Profiler *) {}
    static void setPreviewing(bool) {}
#else
    QV4::Debugging::Debugger *debugger() const { return m_debugger.data(); }
    QV4::Profiling::Profiler *profiler() const { return m_profiler.data(); }

    void setDebugger(Debugging::Debugger *debugger);
    void setProfiler(Profiling::Profiler *profiler);
    static void setPreviewing(bool enabled);
#endif // QT_CONFIG(qml_debug)

    // We don't want to #include <private/qv4stackframe_p.h> here, but we still want
    // currentContext() to be inline. Therefore we shift the requirement to provide the
    // complete type of CppStackFrame to the caller by making this a template.
    template<typename StackFrame = CppStackFrame>
    ExecutionContext *currentContext() const
    {
        return static_cast<const StackFrame *>(currentStackFrame)->context();
    }

    // ensure we always get odd prototype IDs. This helps make marking in QV4::Lookup fast
    quintptr newProtoId() { return (protoIdCount += 2); }

    Heap::InternalClass *newInternalClass(const VTable *vtable, Object *prototype);

    Heap::Object *newObject();
    Heap::Object *newObject(Heap::InternalClass *internalClass);

    Heap::String *newString(char16_t c) { return newString(QChar(c)); }
    Heap::String *newString(const QString &s = QString());
    Heap::String *newIdentifier(const QString &text);

    Heap::Object *newStringObject(const String *string);
    Heap::Object *newSymbolObject(const Symbol *symbol);
    Heap::Object *newNumberObject(double value);
    Heap::Object *newBooleanObject(bool b);

    Heap::ArrayObject *newArrayObject(int count = 0);
    Heap::ArrayObject *newArrayObject(const Value *values, int length);
    Heap::ArrayObject *newArrayObject(const QStringList &list);
    Heap::ArrayObject *newArrayObject(Heap::InternalClass *ic);

    Heap::ArrayBuffer *newArrayBuffer(const QByteArray &array);
    Heap::ArrayBuffer *newArrayBuffer(size_t length);

    Heap::DateObject *newDateObject(double dateTime);
    Heap::DateObject *newDateObject(const QDateTime &dateTime);
    Heap::DateObject *newDateObject(QDate date, Heap::Object *parent, int index, uint flags);
    Heap::DateObject *newDateObject(QTime time, Heap::Object *parent, int index, uint flags);
    Heap::DateObject *newDateObject(QDateTime dateTime, Heap::Object *parent, int index, uint flags);

    Heap::RegExpObject *newRegExpObject(const QString &pattern, int flags);
    Heap::RegExpObject *newRegExpObject(RegExp *re);
#if QT_CONFIG(regularexpression)
    Heap::RegExpObject *newRegExpObject(const QRegularExpression &re);
#endif

    Heap::UrlObject *newUrlObject();
    Heap::UrlObject *newUrlObject(const QUrl &url);
    Heap::UrlSearchParamsObject *newUrlSearchParamsObject();

    Heap::Object *newErrorObject(const Value &value);
    Heap::Object *newErrorObject(const QString &message);
    Heap::Object *newSyntaxErrorObject(const QString &message, const QString &fileName, int line, int column);
    Heap::Object *newSyntaxErrorObject(const QString &message);
    Heap::Object *newReferenceErrorObject(const QString &message);
    Heap::Object *newReferenceErrorObject(const QString &message, const QString &fileName, int line, int column);
    Heap::Object *newTypeErrorObject(const QString &message);
    Heap::Object *newRangeErrorObject(const QString &message);
    Heap::Object *newURIErrorObject(const QString &message);
    Heap::Object *newURIErrorObject(const Value &message);
    Heap::Object *newEvalErrorObject(const QString &message);

    Heap::PromiseObject *newPromiseObject();
    Heap::Object *newPromiseObject(const QV4::FunctionObject *thisObject, const QV4::PromiseCapability *capability);
    Promise::ReactionHandler *getPromiseReactionHandler();

    Heap::Object *newVariantObject(const QMetaType type, const void *data);

    Heap::Object *newForInIteratorObject(Object *o);
    Heap::Object *newSetIteratorObject(Object *o);
    Heap::Object *newMapIteratorObject(Object *o);
    Heap::Object *newArrayIteratorObject(Object *o);

    static Heap::ExecutionContext *qmlContext(Heap::ExecutionContext *ctx)
    {
        Heap::ExecutionContext *outer = ctx->outer;

        if (ctx->type != Heap::ExecutionContext::Type_QmlContext && !outer)
            return nullptr;

        while (outer && outer->type != Heap::ExecutionContext::Type_GlobalContext) {
            ctx = outer;
            outer = ctx->outer;
        }

        Q_ASSERT(ctx);
        if (ctx->type != Heap::ExecutionContext::Type_QmlContext)
            return nullptr;

        return ctx;
    }

    Heap::QmlContext *qmlContext() const;
    QObject *qmlScopeObject() const;
    QQmlRefPointer<QQmlContextData> callingQmlContext() const;


    StackTrace stackTrace(int frameLimit = -1) const;
    QUrl resolvedUrl(const QString &file);

    void markObjects(MarkStack *markStack);

    void initRootContext();

    Heap::InternalClass *newClass(Heap::InternalClass *other);

    StackTrace exceptionStackTrace;

    ReturnedValue throwError(const Value &value);
    ReturnedValue catchException(StackTrace *trace = nullptr);

    ReturnedValue throwError(const QString &message);
    ReturnedValue throwSyntaxError(const QString &message);
    ReturnedValue throwSyntaxError(const QString &message, const QString &fileName, int lineNumber, int column);
    ReturnedValue throwTypeError();
    ReturnedValue throwTypeError(const QString &message);
    ReturnedValue throwReferenceError(const Value &value);
    ReturnedValue throwReferenceError(const QString &name);
    ReturnedValue throwReferenceError(const QString &value, const QString &fileName, int lineNumber, int column);
    ReturnedValue throwRangeError(const Value &value);
    ReturnedValue throwRangeError(const QString &message);
    ReturnedValue throwURIError(const Value &msg);
    ReturnedValue throwUnimplemented(const QString &message);

    // Use only inside catch(...) -- will re-throw if no JS exception
    QQmlError catchExceptionAsQmlError();

    void amendException();

    // variant conversions
    static QVariant toVariant(
        const QV4::Value &value, QMetaType typeHint, bool createJSValueForObjectsAndSymbols = true);
    static QVariant toVariantLossy(const QV4::Value &value);
    QV4::ReturnedValue fromVariant(const QVariant &);
    QV4::ReturnedValue fromVariant(
            const QVariant &variant, Heap::Object *parent, int property, uint flags);

    static QVariantMap variantMapFromJS(const QV4::Object *o);
    static QVariantHash variantHashFromJS(const QV4::Object *o);

    static bool metaTypeFromJS(const Value &value, QMetaType type, void *data);
    QV4::ReturnedValue metaTypeToJS(QMetaType type, const void *data);

    int maxJSStackSize() const;
    int maxGCStackSize() const;

    bool checkStackLimits();
    int safeForAllocLength(qint64 len64);

    template<typename Jittable>
    bool canJIT(Jittable *jittable) const
    {
#if QT_CONFIG(qml_jit)
        return m_canAllocateExecutableMemory
                && jittable->isJittable()
                && jittable->interpreterCallCount >= s_jitCallCountThreshold;
#else
        Q_UNUSED(jittable);
        return false;
#endif
    }

    QV4::ReturnedValue global();
    void initQmlGlobalObject();
    void initializeGlobal();
    void createQtObject();

    void freezeObject(const QV4::Value &value);
    void lockObject(const QV4::Value &value);

#if QT_CONFIG(qml_xml_http_request)
    void setupXmlHttpRequestExtension();
    void *xmlHttpRequestData() const { return m_xmlHttpRequestData; }
#endif

    void setQmlEngine(QQmlEngine *engine);

    QQmlDelayedCallQueue *delayedCallQueue() { return &m_delayedCallQueue; }

    // used for console.time(), console.timeEnd()
    void startTimer(const QString &timerName);
    qint64 stopTimer(const QString &timerName, bool *wasRunning);

    // used for console.count()
    int consoleCountHelper(const QString &file, quint16 line, quint16 column);

    struct Deletable {
        virtual ~Deletable() {}
    };

    static QMutex *registrationMutex();
    static int registerExtension();

    void setExtensionData(int, Deletable *);
    Deletable *extensionData(int index) const
    {
        if (index < m_extensionData.size())
            return m_extensionData[index];
        else
            return nullptr;
    }

    double localTZA = 0.0; // local timezone, initialized at startup

    QQmlRefPointer<ExecutableCompilationUnit> compileModule(const QUrl &url);
    QQmlRefPointer<ExecutableCompilationUnit> compileModule(
            const QUrl &url, const QString &sourceCode, const QDateTime &sourceTimeStamp);

    QQmlRefPointer<ExecutableCompilationUnit> compilationUnitForUrl(const QUrl &url) const;

    QQmlRefPointer<ExecutableCompilationUnit> executableCompilationUnit(
            QQmlRefPointer<QV4::CompiledData::CompilationUnit> &&unit);

    QQmlRefPointer<ExecutableCompilationUnit> insertCompilationUnit(
            QQmlRefPointer<QV4::CompiledData::CompilationUnit> &&unit);

    QMultiHash<QUrl, QQmlRefPointer<ExecutableCompilationUnit>> compilationUnits() const
    {
        return m_compilationUnits;
    }

    void trimCompilationUnits();
    void trimCompilationUnitsForUrl(const QUrl &url);

    using Module = QQmlRefPointer<ExecutableCompilationUnit>;

    Module registerNativeModule(const QUrl &url, const QV4::Value &value);
    Module moduleForUrl(const QUrl &_url, const ExecutableCompilationUnit *referrer = nullptr) const;
    Module loadModule(const QUrl &_url, const ExecutableCompilationUnit *referrer = nullptr);

    DiskCacheOptions diskCacheOptions() const;

    void callInContext(QV4::Function *function, QObject *self, QV4::ExecutionContext *ctxt,
                       int argc, void **args, QMetaType *types);
    QV4::ReturnedValue callInContext(QV4::Function *function, QObject *self,
                                     QV4::ExecutionContext *ctxt, int argc, const QV4::Value *argv);

    QV4::ReturnedValue fromData(
            QMetaType type, const void *ptr,
            Heap::Object *parent = nullptr, int property = -1, uint flags = 0);


    static void setMaxCallDepth(int maxCallDepth) { s_maxCallDepth = maxCallDepth; }
    static int maxCallDepth() { return s_maxCallDepth; }

    template<typename Value>
    static QJSPrimitiveValue createPrimitive(const Value &v)
    {
        if (v->isUndefined())
            return QJSPrimitiveValue(QJSPrimitiveUndefined());
        if (v->isNull())
            return QJSPrimitiveValue(QJSPrimitiveNull());
        if (v->isBoolean())
            return QJSPrimitiveValue(v->toBoolean());
        if (v->isInteger())
            return QJSPrimitiveValue(v->integerValue());
        if (v->isDouble())
            return QJSPrimitiveValue(v->doubleValue());
        bool ok;
        const QString result = v->toQString(&ok);
        return ok ? QJSPrimitiveValue(result) : QJSPrimitiveValue(QJSPrimitiveUndefined());
    }

private:
    template<int Frames>
    friend struct ExecutionEngineCallDepthRecorder;

    static void initializeStaticMembers();

    bool inStack(const void *current) const
    {
#if Q_STACK_GROWTH_DIRECTION > 0
        return current < cppStackLimit && current >= cppStackBase;
#else
        return current > cppStackLimit && current <= cppStackBase;
#endif
    }

    void setCppStackProperties()
    {
        const StackProperties stack = stackProperties();
        cppStackBase = stack.base;
        if (s_stackSizeSoftLimit == -1)
            cppStackLimit = stack.softLimit;
        else
            cppStackLimit = incrementStackPointer(stack.base, s_stackSizeSoftLimit);
    }

    bool hasCppStackOverflow()
    {
        if (s_maxCallDepth >= 0)
            return callDepth >= s_maxCallDepth;

        if (inStack(currentStackPointer()))
            return false;

        // Double check the stack limits on failure.
        // We may have moved to a different thread.
        setCppStackProperties();

        return !inStack(currentStackPointer());
    }

    bool hasJsStackOverflow() const
    {
        return jsStackTop > jsStackLimit;
    }

    bool hasStackOverflow()
    {
        return hasJsStackOverflow() || hasCppStackOverflow();
    }

    static int s_maxCallDepth;
    static int s_jitCallCountThreshold;
    static int s_maxJSStackSize;
    static int s_maxGCStackSize;
    static int s_stackSizeSoftLimit;

#if QT_CONFIG(qml_debug)
    QScopedPointer<QV4::Debugging::Debugger> m_debugger;
    QScopedPointer<QV4::Profiling::Profiler> m_profiler;
#endif

    // used by generated Promise objects to handle 'then' events
    QScopedPointer<QV4::Promise::ReactionHandler> m_reactionHandler;

#if QT_CONFIG(qml_xml_http_request)
    void *m_xmlHttpRequestData = nullptr;
#endif

    QQmlEngine *m_qmlEngine = nullptr;

    QQmlDelayedCallQueue m_delayedCallQueue;

    QElapsedTimer m_time;
    QHash<QString, qint64> m_startedTimers;

    QHash<QString, quint32> m_consoleCount;

    QVector<Deletable *> m_extensionData;

    QMultiHash<QUrl, QQmlRefPointer<ExecutableCompilationUnit>> m_compilationUnits;
};

#define CHECK_STACK_LIMITS(v4) \
    if (v4->checkStackLimits()) \
        return Encode::undefined(); \
    ExecutionEngineCallDepthRecorder _executionEngineCallDepthRecorder(v4);

template<int Frames = 1>
struct ExecutionEngineCallDepthRecorder
{
    ExecutionEngine *ee;

    ExecutionEngineCallDepthRecorder(ExecutionEngine *e): ee(e)
    {
        if (ExecutionEngine::s_maxCallDepth >= 0)
            ee->callDepth += Frames;
    }

    ~ExecutionEngineCallDepthRecorder()
    {
        if (ExecutionEngine::s_maxCallDepth >= 0)
            ee->callDepth -= Frames;
    }

    bool hasOverflow() const
    {
        return ee->hasCppStackOverflow();
    }
};

inline bool ExecutionEngine::checkStackLimits()
{
    if (Q_UNLIKELY(hasStackOverflow())) {
        throwRangeError(QStringLiteral("Maximum call stack size exceeded."));
        return true;
    }

    return false;
}

Q_DECLARE_OPERATORS_FOR_FLAGS(ExecutionEngine::DiskCacheOptions);

} // namespace QV4

QT_END_NAMESPACE

#endif // QV4ENGINE_H
