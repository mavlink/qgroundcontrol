// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant
#ifndef QV4RUNTIMEAPI_P_H
#define QV4RUNTIMEAPI_P_H

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

#include <private/qv4global_p.h>
#include <private/qv4staticvalue_p.h>

QT_BEGIN_NAMESPACE

namespace QV4 {

typedef uint Bool;

struct Q_QML_EXPORT Runtime {
    typedef ReturnedValue (*UnaryOperation)(const Value &value);
    typedef ReturnedValue (*BinaryOperation)(const Value &left, const Value &right);
    typedef ReturnedValue (*BinaryOperationContext)(ExecutionEngine *, const Value &left, const Value &right);

    enum class Throws { No, Yes };
    enum class ChangesContext { No, Yes };
    enum class Pure { No, Yes };
    enum class LastArgumentIsOutputValue { No, Yes };

    template<Throws t, ChangesContext c = ChangesContext::No, Pure p = Pure::No,
             LastArgumentIsOutputValue out = LastArgumentIsOutputValue::No>
    struct Method
    {
        static constexpr bool throws = t == Throws::Yes;
        static constexpr bool changesContext = c == ChangesContext::Yes;
        static constexpr bool pure = p == Pure::Yes;
        static constexpr bool lastArgumentIsOutputValue = out == LastArgumentIsOutputValue::Yes;
    };
    using PureMethod = Method<Throws::No, ChangesContext::No, Pure::Yes>;
    using IteratorMethod = Method<Throws::No, ChangesContext::No, Pure::No,
                                  LastArgumentIsOutputValue::Yes>;

    /* call */
    struct Q_QML_EXPORT CallGlobalLookup : Method<Throws::Yes>
    {
        static ReturnedValue call(ExecutionEngine *, uint, Value[], int);
    };
    struct Q_QML_EXPORT CallQmlContextPropertyLookup : Method<Throws::Yes>
    {
        static ReturnedValue call(ExecutionEngine *, uint, Value[], int);
    };
    struct Q_QML_EXPORT CallName : Method<Throws::Yes>
    {
        static ReturnedValue call(ExecutionEngine *, int, Value[], int);
    };
    struct Q_QML_EXPORT CallProperty : Method<Throws::Yes>
    {
        static ReturnedValue call(ExecutionEngine *, const Value &, int, Value[], int);
    };
    struct Q_QML_EXPORT CallPropertyLookup : Method<Throws::Yes>
    {
        static ReturnedValue call(ExecutionEngine *, const Value &, uint, Value[], int);
    };
    struct Q_QML_EXPORT CallValue : Method<Throws::Yes>
    {
        static ReturnedValue call(ExecutionEngine *, const Value &, Value[], int);
    };
    struct Q_QML_EXPORT CallWithReceiver : Method<Throws::Yes>
    {
        static ReturnedValue call(ExecutionEngine *, const Value &, const Value &, Value[], int);
    };
    struct Q_QML_EXPORT CallPossiblyDirectEval : Method<Throws::Yes>
    {
        static ReturnedValue call(ExecutionEngine *, Value[], int);
    };
    struct Q_QML_EXPORT CallWithSpread : Method<Throws::Yes>
    {
        static ReturnedValue call(ExecutionEngine *, const Value &, const Value &, Value[], int);
    };
    struct Q_QML_EXPORT TailCall : Method<Throws::Yes>
    {
        static ReturnedValue call(JSTypesStackFrame *, ExecutionEngine *engine);
    };

    /* construct */
    struct Q_QML_EXPORT Construct : Method<Throws::Yes>
    {
        static ReturnedValue call(ExecutionEngine *, const Value &, const Value &, Value[], int);
    };
    struct Q_QML_EXPORT ConstructWithSpread : Method<Throws::Yes>
    {
        static ReturnedValue call(ExecutionEngine *, const Value &, const Value &, Value[], int);
    };

    /* load & store */
    struct Q_QML_EXPORT StoreNameStrict : Method<Throws::Yes>
    {
        static void call(ExecutionEngine *, int, const Value &);
    };
    struct Q_QML_EXPORT StoreNameSloppy : Method<Throws::Yes>
    {
        static void call(ExecutionEngine *, int, const Value &);
    };
    struct Q_QML_EXPORT StoreProperty : Method<Throws::Yes>
    {
        static void call(ExecutionEngine *, const Value &, int, const Value &);
    };
    struct Q_QML_EXPORT StoreElement : Method<Throws::Yes>
    {
        static void call(ExecutionEngine *, const Value &, const Value &, const Value &);
    };
    struct Q_QML_EXPORT LoadProperty : Method<Throws::Yes>
    {
        static ReturnedValue call(ExecutionEngine *, const Value &, int);
    };
    struct Q_QML_EXPORT LoadName : Method<Throws::Yes>
    {
        static ReturnedValue call(ExecutionEngine *, int);
    };
    struct Q_QML_EXPORT LoadElement : Method<Throws::Yes>
    {
        static ReturnedValue call(ExecutionEngine *, const Value &, const Value &);
    };
    struct Q_QML_EXPORT LoadSuperProperty : Method<Throws::Yes>
    {
        static ReturnedValue call(ExecutionEngine *, const Value &);
    };
    struct Q_QML_EXPORT StoreSuperProperty : Method<Throws::Yes>
    {
        static void call(ExecutionEngine *, const Value &, const Value &);
    };
    struct Q_QML_EXPORT LoadSuperConstructor : Method<Throws::Yes>
    {
        static ReturnedValue call(ExecutionEngine *, const Value &);
    };
    struct Q_QML_EXPORT LoadGlobalLookup : Method<Throws::Yes>
    {
        static ReturnedValue call(ExecutionEngine *, Function *, int);
    };
    struct Q_QML_EXPORT LoadQmlContextPropertyLookup : Method<Throws::Yes>
    {
        static ReturnedValue call(ExecutionEngine *, uint);
    };
    struct Q_QML_EXPORT GetLookup : Method<Throws::Yes>
    {
        static ReturnedValue call(ExecutionEngine *, Function *, const Value &, int);
    };
    struct Q_QML_EXPORT SetLookupStrict : Method<Throws::Yes>
    {
        static void call(Function *, const Value &, int, const Value &);
    };
    struct Q_QML_EXPORT SetLookupSloppy : Method<Throws::Yes>
    {
        static void call(Function *, const Value &, int, const Value &);
    };

    /* typeof */
    struct Q_QML_EXPORT TypeofValue : PureMethod
    {
        static ReturnedValue call(ExecutionEngine *, const Value &);
    };
    struct Q_QML_EXPORT TypeofName : Method<Throws::No>
    {
        static ReturnedValue call(ExecutionEngine *, int);
    };

    /* delete */
    struct Q_QML_EXPORT DeleteProperty_NoThrow : Method<Throws::No>
    {
        static Bool call(ExecutionEngine *, const Value &, const Value &);
    };
    struct Q_QML_EXPORT DeleteProperty : Method<Throws::Yes>
    {
        static ReturnedValue call(ExecutionEngine *, Function *, const Value &, const Value &);
    };
    struct Q_QML_EXPORT DeleteName_NoThrow : Method<Throws::No>
    {
        static Bool call(ExecutionEngine *, int);
    };
    struct Q_QML_EXPORT DeleteName : Method<Throws::Yes>
    {
        static ReturnedValue call(ExecutionEngine *, Function *, int);
    };

    /* exceptions & scopes */
    struct Q_QML_EXPORT ThrowException : Method<Throws::Yes>
    {
        static void call(ExecutionEngine *, const Value &);
    };
    struct Q_QML_EXPORT PushCallContext : Method<Throws::No, ChangesContext::Yes>
    {
        static void call(JSTypesStackFrame *);
    };
    struct Q_QML_EXPORT PushWithContext : Method<Throws::Yes, ChangesContext::Yes>
    {
        static ReturnedValue call(ExecutionEngine *, const Value &);
    };
    struct Q_QML_EXPORT PushCatchContext : Method<Throws::No, ChangesContext::Yes>
    {
        static void call(ExecutionEngine *, int, int);
    };
    struct Q_QML_EXPORT PushBlockContext : Method<Throws::No, ChangesContext::Yes>
    {
        static void call(ExecutionEngine *, int);
    };
    struct Q_QML_EXPORT CloneBlockContext : Method<Throws::No, ChangesContext::Yes>
    {
        static void call(ExecutionEngine *);
    };
    struct Q_QML_EXPORT PushScriptContext : Method<Throws::No, ChangesContext::Yes>
    {
        static void call(ExecutionEngine *, int);
    };
    struct Q_QML_EXPORT PopScriptContext : Method<Throws::No, ChangesContext::Yes>
    {
        static void call(ExecutionEngine *);
    };
    struct Q_QML_EXPORT ThrowReferenceError : Method<Throws::Yes>
    {
        static void call(ExecutionEngine *, int);
    };
    struct Q_QML_EXPORT ThrowOnNullOrUndefined : Method<Throws::Yes>
    {
        static void call(ExecutionEngine *, const Value &);
    };

    /* garbage collection */
    struct Q_QML_EXPORT MarkCustom : PureMethod
    {
        static void call(const Value &toBeMarked);
    };

    /* closures */
    struct Q_QML_EXPORT Closure : Method<Throws::No>
    {
        static ReturnedValue call(ExecutionEngine *, int);
    };

    /* Function header */
    struct Q_QML_EXPORT ConvertThisToObject : Method<Throws::Yes>
    {
        static ReturnedValue call(ExecutionEngine *, const Value &);
    };
    struct Q_QML_EXPORT DeclareVar : Method<Throws::Yes>
    {
        static void call(ExecutionEngine *, Bool, int);
    };
    struct Q_QML_EXPORT CreateMappedArgumentsObject : Method<Throws::No>
    {
        static ReturnedValue call(ExecutionEngine *);
    };
    struct Q_QML_EXPORT CreateUnmappedArgumentsObject : Method<Throws::No>
    {
        static ReturnedValue call(ExecutionEngine *);
    };
    struct Q_QML_EXPORT CreateRestParameter : PureMethod
    {
        static ReturnedValue call(ExecutionEngine *, int);
    };

    /* literals */
    struct Q_QML_EXPORT ArrayLiteral : Method<Throws::Yes>
    {
        static ReturnedValue call(ExecutionEngine *, Value[], uint);
    };
    struct Q_QML_EXPORT ObjectLiteral : Method<Throws::Yes>
    {
        static ReturnedValue call(ExecutionEngine *, int, Value[], int);
    };
    struct Q_QML_EXPORT CreateClass : Method<Throws::Yes>
    {
        static ReturnedValue call(ExecutionEngine *, int, const Value &, Value[]);
    };

    /* for-in, for-of and array destructuring */
    struct Q_QML_EXPORT GetIterator : Method<Throws::Yes>
    {
        static ReturnedValue call(ExecutionEngine *, const Value &, int);
    };
    struct Q_QML_EXPORT IteratorNext : IteratorMethod
    {
        static ReturnedValue call(ExecutionEngine *, const Value &, Value *);
    };
    struct Q_QML_EXPORT IteratorNextForYieldStar : IteratorMethod
    {
        static ReturnedValue call(ExecutionEngine *, const Value &, const Value &, Value *);
    };
    struct Q_QML_EXPORT IteratorClose : Method<Throws::No>
    {
        static ReturnedValue call(ExecutionEngine *, const Value &);
    };
    struct Q_QML_EXPORT DestructureRestElement : Method<Throws::Yes>
    {
        static ReturnedValue call(ExecutionEngine *, const Value &);
    };

    /* conversions */
    struct Q_QML_EXPORT ToObject : Method<Throws::Yes>
    {
        static ReturnedValue call(ExecutionEngine *, const Value &);
    };
    struct Q_QML_EXPORT ToBoolean : PureMethod
    {
        static Bool call(const Value &);
    };
    struct Q_QML_EXPORT ToNumber : Method<Throws::Yes>
    {
        static ReturnedValue call(ExecutionEngine *, const Value &);
    };
    /* unary operators */
    struct Q_QML_EXPORT UMinus : Method<Throws::Yes>
    {
        static ReturnedValue call(const Value &);
    };

    /* binary operators */
    struct Q_QML_EXPORT Instanceof : Method<Throws::Yes>
    {
        static ReturnedValue call(ExecutionEngine *, const Value &, const Value &);
    };
    struct Q_QML_EXPORT As : Method<Throws::No>
    {
        static ReturnedValue call(ExecutionEngine *, const Value &, const Value &);
    };
    struct Q_QML_EXPORT In : Method<Throws::Yes>
    {
        static ReturnedValue call(ExecutionEngine *, const Value &, const Value &);
    };
    struct Q_QML_EXPORT Add : Method<Throws::Yes>
    {
        static ReturnedValue call(ExecutionEngine *, const Value &, const Value &);
    };
    struct Q_QML_EXPORT Sub : Method<Throws::Yes>
    {
        static ReturnedValue call(const Value &, const Value &);
    };
    struct Q_QML_EXPORT Mul : Method<Throws::Yes>
    {
        static ReturnedValue call(const Value &, const Value &);
    };
    struct Q_QML_EXPORT Div : Method<Throws::Yes>
    {
        static ReturnedValue call(const Value &, const Value &);
    };
    struct Q_QML_EXPORT Mod : Method<Throws::Yes>
    {
        static ReturnedValue call(const Value &, const Value &);
    };
    struct Q_QML_EXPORT Exp : Method<Throws::Yes>
    {
        static ReturnedValue call(const Value &, const Value &);
    };
    struct Q_QML_EXPORT BitAnd : Method<Throws::Yes>
    {
        static ReturnedValue call(const Value &, const Value &);
    };
    struct Q_QML_EXPORT BitOr : Method<Throws::Yes>
    {
        static ReturnedValue call(const Value &, const Value &);
    };
    struct Q_QML_EXPORT BitXor : Method<Throws::Yes>
    {
        static ReturnedValue call(const Value &, const Value &);
    };
    struct Q_QML_EXPORT Shl : Method<Throws::Yes>
    {
        static ReturnedValue call(const Value &, const Value &);
    };
    struct Q_QML_EXPORT Shr : Method<Throws::Yes>
    {
        static ReturnedValue call(const Value &, const Value &);
    };
    struct Q_QML_EXPORT UShr : Method<Throws::Yes>
    {
        static ReturnedValue call(const Value &, const Value &);
    };
    struct Q_QML_EXPORT GreaterThan : Method<Throws::Yes>
    {
        static ReturnedValue call(const Value &, const Value &);
    };
    struct Q_QML_EXPORT LessThan : Method<Throws::Yes>
    {
        static ReturnedValue call(const Value &, const Value &);
    };
    struct Q_QML_EXPORT GreaterEqual : Method<Throws::Yes>
    {
        static ReturnedValue call(const Value &, const Value &);
    };
    struct Q_QML_EXPORT LessEqual : Method<Throws::Yes>
    {
        static ReturnedValue call(const Value &, const Value &);
    };
    struct Q_QML_EXPORT Equal : Method<Throws::Yes>
    {
        static ReturnedValue call(const Value &, const Value &);
    };
    struct Q_QML_EXPORT NotEqual : Method<Throws::Yes>
    {
        static ReturnedValue call(const Value &, const Value &);
    };
    struct Q_QML_EXPORT StrictEqual : Method<Throws::Yes>
    {
        static ReturnedValue call(const Value &, const Value &);
    };
    struct Q_QML_EXPORT StrictNotEqual : Method<Throws::Yes>
    {
        static ReturnedValue call(const Value &, const Value &);
    };

    /* comparisons */
    struct Q_QML_EXPORT CompareGreaterThan : Method<Throws::Yes>
    {
        static Bool call(const Value &, const Value &);
    };
    struct Q_QML_EXPORT CompareLessThan : Method<Throws::Yes>
    {
        static Bool call(const Value &, const Value &);
    };
    struct Q_QML_EXPORT CompareGreaterEqual : Method<Throws::Yes>
    {
        static Bool call(const Value &, const Value &);
    };
    struct Q_QML_EXPORT CompareLessEqual : Method<Throws::Yes>
    {
        static Bool call(const Value &, const Value &);
    };
    struct Q_QML_EXPORT CompareEqual : Method<Throws::Yes>
    {
        static Bool call(const Value &, const Value &);
    };
    struct Q_QML_EXPORT CompareNotEqual : Method<Throws::Yes>
    {
        static Bool call(const Value &, const Value &);
    };
    struct Q_QML_EXPORT CompareStrictEqual : Method<Throws::Yes>
    {
        static Bool call(const Value &, const Value &);
    };
    struct Q_QML_EXPORT CompareStrictNotEqual : Method<Throws::Yes>
    {
        static Bool call(const Value &, const Value &);
    };

    struct Q_QML_EXPORT CompareInstanceof : Method<Throws::Yes>
    {
        static Bool call(ExecutionEngine *, const Value &, const Value &);
    };
    struct Q_QML_EXPORT CompareIn : Method<Throws::Yes>
    {
        static Bool call(ExecutionEngine *, const Value &, const Value &);
    };

    struct Q_QML_EXPORT RegexpLiteral : PureMethod
    {
        static ReturnedValue call(ExecutionEngine *, int);
    };
    struct Q_QML_EXPORT GetTemplateObject : PureMethod
    {
        static ReturnedValue call(Function *, int);
    };

    struct StackOffsets {
        static const int tailCall_function   = -1;
        static const int tailCall_thisObject = -2;
        static const int tailCall_argv       = -3;
        static const int tailCall_argc       = -4;
    };

    static QHash<const void *, const char *> symbolTable();
};

static_assert(std::is_standard_layout<Runtime>::value, "Runtime needs to be standard layout in order for us to be able to use offsetof");
static_assert(sizeof(Runtime::BinaryOperation) == sizeof(void*), "JIT expects a function pointer to fit into a regular pointer, for cross-compilation offset translation");

} // namespace QV4

QT_END_NAMESPACE

#endif // QV4RUNTIMEAPI_P_H
