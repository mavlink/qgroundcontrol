// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant
#ifndef QV4JSCALL_H
#define QV4JSCALL_H

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

#include <private/qqmllistwrapper_p.h>
#include <private/qqmlvaluetypewrapper_p.h>

#include <private/qv4alloca_p.h>
#include <private/qv4dateobject_p.h>
#include <private/qv4function_p.h>
#include <private/qv4functionobject_p.h>
#include <private/qv4qobjectwrapper_p.h>
#include <private/qv4regexpobject_p.h>
#include <private/qv4scopedvalue_p.h>
#include <private/qv4sequenceobject_p.h>
#include <private/qv4urlobject_p.h>
#include <private/qv4variantobject_p.h>

#if QT_CONFIG(regularexpression)
#include <QtCore/qregularexpression.h>
#endif

QT_BEGIN_NAMESPACE

namespace QV4 {

template<typename Args>
CallData *callDatafromJS(const Scope &scope, const Args *args, const FunctionObject *f = nullptr)
{
    int size = int(offsetof(QV4::CallData, args)/sizeof(QV4::Value)) + args->argc;
    CallData *ptr = reinterpret_cast<CallData *>(scope.alloc<Scope::Uninitialized>(size));
    ptr->function = Encode::undefined();
    ptr->context = Encode::undefined();
    ptr->accumulator = Encode::undefined();
    ptr->thisObject = args->thisObject ? args->thisObject->asReturnedValue() : Encode::undefined();
    ptr->newTarget = Encode::undefined();
    ptr->setArgc(args->argc);
    if (args->argc)
        memcpy(ptr->args, args->args, args->argc*sizeof(Value));
    if (f)
        ptr->function = f->asReturnedValue();
    return ptr;
}

struct JSCallArguments
{
    JSCallArguments(const Scope &scope, int argc = 0)
        : thisObject(scope.alloc()), args(scope.alloc(argc)), argc(argc)
    {
    }

    CallData *callData(const Scope &scope, const FunctionObject *f = nullptr) const
    {
        return callDatafromJS(scope, this, f);
    }

    Value *thisObject;
    Value *args;
    const int argc;
};

struct JSCallData
{
    JSCallData(const Value *thisObject, const Value *argv, int argc)
        : thisObject(thisObject), args(argv), argc(argc)
    {
    }

    Q_IMPLICIT JSCallData(const JSCallArguments &args)
        : thisObject(args.thisObject), args(args.args), argc(args.argc)
    {
    }

    CallData *callData(const Scope &scope, const FunctionObject *f = nullptr) const
    {
        return callDatafromJS(scope, this, f);
    }

    const Value *thisObject;
    const Value *args;
    const int argc;
};

inline
ReturnedValue FunctionObject::callAsConstructor(const JSCallData &data) const
{
    return callAsConstructor(data.args, data.argc, this);
}

inline
ReturnedValue FunctionObject::call(const JSCallData &data) const
{
    return call(data.thisObject, data.args, data.argc);
}

void populateJSCallArguments(ExecutionEngine *v4, JSCallArguments &jsCall, int argc,
                             void **args, const QMetaType *types);

template<typename Callable>
ReturnedValue convertAndCall(
        ExecutionEngine *engine, const Function::AOTCompiledFunction *aotFunction,
        const Value *thisObject, const Value *argv, int argc, Callable call)
{
    const qsizetype numFunctionArguments = aotFunction->types.length() - 1;
    Q_ALLOCA_VAR(void *, values, (numFunctionArguments + 1) * sizeof(void *));
    Q_ALLOCA_VAR(QMetaType, types, (numFunctionArguments + 1) * sizeof(QMetaType));

    for (qsizetype i = 0; i < numFunctionArguments; ++i) {
        const QMetaType argumentType = aotFunction->types[i + 1];
        types[i + 1] = argumentType;
        if (const qsizetype argumentSize = argumentType.sizeOf()) {
            Q_ALLOCA_VAR(void, argument, argumentSize);
            if (argumentType.flags() & QMetaType::NeedsConstruction) {
                argumentType.construct(argument);
                if (i < argc)
                    ExecutionEngine::metaTypeFromJS(argv[i], argumentType, argument);
            } else if (i >= argc
                        || !ExecutionEngine::metaTypeFromJS(argv[i], argumentType, argument)) {
                // If we can't convert the argument, we need to default-construct it even if it
                // doesn't formally need construction.
                // E.g. an int doesn't need construction, but we still want it to be 0.
                argumentType.construct(argument);
            }

            values[i + 1] = argument;
        } else {
            values[i + 1] = nullptr;
        }
    }

    Q_ALLOCA_DECLARE(void, returnValue);
    types[0] = aotFunction->types[0];
    if (const qsizetype returnSize = types[0].sizeOf()) {
        Q_ALLOCA_ASSIGN(void, returnValue, returnSize);
        values[0] = returnValue;
        if (types[0].flags() & QMetaType::NeedsConstruction)
            types[0].construct(returnValue);
    } else {
        values[0] = nullptr;
    }

    if (const QV4::QObjectWrapper *cppThisObject = thisObject
                ? thisObject->as<QV4::QObjectWrapper>()
                : nullptr) {
        call(cppThisObject->object(), values, types, argc);
    } else {
        call(nullptr, values, types, argc);
    }

    ReturnedValue result;
    if (values[0]) {
        result = engine->metaTypeToJS(types[0], values[0]);
        if (types[0].flags() & QMetaType::NeedsDestruction)
            types[0].destruct(values[0]);
    } else {
        result = Encode::undefined();
    }

    for (qsizetype i = 1, end = numFunctionArguments + 1; i < end; ++i) {
        if (types[i].flags() & QMetaType::NeedsDestruction)
            types[i].destruct(values[i]);
    }

    return result;
}

template<typename Callable>
bool convertAndCall(ExecutionEngine *engine, QObject *thisObject,
                    void **a, const QMetaType *types, int argc, Callable call)
{
    Scope scope(engine);
    QV4::JSCallArguments jsCallData(scope, argc);

    for (int ii = 0; ii < argc; ++ii)
        jsCallData.args[ii] = engine->metaTypeToJS(types[ii + 1], a[ii + 1]);

    ScopedObject jsThisObject(scope);
    if (thisObject) {
        // The result of wrap() can only be null, undefined, or an object.
        jsThisObject = QV4::QObjectWrapper::wrap(engine, thisObject);
        if (!jsThisObject)
            jsThisObject = engine->globalObject;
    } else {
        jsThisObject = engine->globalObject;
    }

    ScopedValue jsResult(scope, call(jsThisObject, jsCallData.args, argc));
    void *result = a[0];
    if (!result)
        return !jsResult->isUndefined();

    const QMetaType resultType = types[0];
    if (scope.hasException()) {
        // Clear the return value
        resultType.destruct(result);
        resultType.construct(result);
    } else if (resultType == QMetaType::fromType<QVariant>()) {
        // When the return type is QVariant, JS objects are to be returned as
        // QJSValue wrapped in QVariant. metaTypeFromJS unwraps them, unfortunately.
        *static_cast<QVariant *>(result) = ExecutionEngine::toVariant(jsResult, QMetaType {});
    } else if (!ExecutionEngine::metaTypeFromJS(jsResult, resultType, result)) {
        // If we cannot convert, also clear the return value.
        // The caller may have given us an uninitialized QObject*, expecting it to be overwritten.
        resultType.destruct(result);
        resultType.construct(result);
    }
    return !jsResult->isUndefined();
}

inline ReturnedValue coerce(
    ExecutionEngine *engine, const Value &value, const QQmlType &qmlType, bool isList);

inline QObject *coerceQObject(const Value &value, const QQmlType &qmlType)
{
    QObject *o;
    if (const QV4::QObjectWrapper *wrapper = value.as<QV4::QObjectWrapper>())
        o = wrapper->object();
    else if (const QV4::QQmlTypeWrapper *wrapper = value.as<QQmlTypeWrapper>())
        o = wrapper->object();
    else
        return nullptr;

    return (o && qmlobject_can_qml_cast(o, qmlType)) ? o : nullptr;
}

enum CoercionProblem
{
    InsufficientAnnotation,
    InvalidListType
};

Q_QML_EXPORT void warnAboutCoercionToVoid(
        ExecutionEngine *engine, const Value &value, CoercionProblem problem);

inline ReturnedValue coerceListType(
    ExecutionEngine *engine, const Value &value, const QQmlType &qmlType)
{
    QMetaType type = qmlType.qListTypeId();
    const auto metaSequence = [&]() {
        // TODO: We should really add the metasequence to the same QQmlType that holds
        //       all the other type information. Then we can get rid of the extra
        //       QQmlMetaType::qmlListType() here.
        return qmlType.isSequentialContainer()
                ? qmlType.listMetaSequence()
                : QQmlMetaType::qmlListType(type).listMetaSequence();
    };

    if (const QV4::Sequence *sequence = value.as<QV4::Sequence>()) {
        if (sequence->d()->listType() == type)
            return value.asReturnedValue();
    }

    if (const QmlListWrapper *list = value.as<QmlListWrapper>()) {
        if (list->d()->propertyType() == type)
            return value.asReturnedValue();
    }

    QMetaType listValueType = qmlType.typeId();
    if (!listValueType.isValid()) {
        warnAboutCoercionToVoid(engine, value, InvalidListType);
        return value.asReturnedValue();
    }

    QV4::Scope scope(engine);

    const ArrayObject *array = value.as<ArrayObject>();
    if (!array) {
        return (listValueType.flags() & QMetaType::PointerToQObject)
                   ? QmlListWrapper::create(engine, listValueType)
                   : SequencePrototype::fromData(engine, type, metaSequence(), nullptr);
    }

    if (listValueType.flags() & QMetaType::PointerToQObject) {
        QV4::Scoped<QmlListWrapper> newList(scope, QmlListWrapper::create(engine, type));
        QQmlListProperty<QObject> *listProperty = newList->d()->property();

        const qsizetype length = array->getLength();
        qsizetype i = 0;
        ScopedValue v(scope);
        for (; i < length; ++i) {
            v = array->get(i);
            listProperty->append(listProperty, coerceQObject(v, qmlType));
        }

        return newList->asReturnedValue();
    }

    QV4::Scoped<Sequence> sequence(
            scope, SequencePrototype::fromData(engine, type, metaSequence(), nullptr));
    const qsizetype length = array->getLength();
    ScopedValue v(scope);
    for (qsizetype i = 0; i < length; ++i) {
        v =  array->get(i);
        sequence->put(PropertyKey::fromArrayIndex(i), v);
    }
    return sequence->asReturnedValue();
}

inline ReturnedValue coerce(
    ExecutionEngine *engine, const Value &value, const QQmlType &qmlType, bool isList)
{
    // These are all the named non-list, non-QObject builtins. Only those need special handling.
    // Some of them may be wrapped in VariantObject because that is how they are stored in VME
    // properties.
    if (isList)
        return coerceListType(engine, value, qmlType);

    const QMetaType metaType = qmlType.typeId();
    if (!metaType.isValid()) {
        if (!value.isUndefined())
            warnAboutCoercionToVoid(engine, value, InsufficientAnnotation);
        return value.asReturnedValue();
    }

    switch (metaType.id()) {
    case QMetaType::Void:
        return Encode::undefined();
    case QMetaType::QVariant:
        return value.asReturnedValue();
    case QMetaType::Int:
        return Encode(value.toInt32());
    case QMetaType::Double:
        return value.convertedToNumber();
    case QMetaType::QString:
        return value.toString(engine)->asReturnedValue();
    case QMetaType::Bool:
        return Encode(value.toBoolean());
    case QMetaType::QDateTime:
        if (value.as<DateObject>())
            return value.asReturnedValue();
        if (const VariantObject *varObject = value.as<VariantObject>()) {
            const QVariant &var = varObject->d()->data();
            switch (var.metaType().id()) {
            case QMetaType::QDateTime:
                return engine->newDateObject(var.value<QDateTime>())->asReturnedValue();
            case QMetaType::QTime:
                return engine->newDateObject(var.value<QTime>(), nullptr, -1, 0)->asReturnedValue();
            case QMetaType::QDate:
                return engine->newDateObject(var.value<QDate>(), nullptr, -1, 0)->asReturnedValue();
            default:
                break;
            }
        }
        return engine->newDateObject(QDateTime())->asReturnedValue();
    case QMetaType::QUrl:
        if (value.as<UrlObject>())
            return value.asReturnedValue();
        if (const VariantObject *varObject = value.as<VariantObject>()) {
            const QVariant &var = varObject->d()->data();
            return var.metaType() == QMetaType::fromType<QUrl>()
                       ? engine->newUrlObject(var.value<QUrl>())->asReturnedValue()
                       : engine->newUrlObject()->asReturnedValue();
        }
        // Since URL properties are stored as string, we need to support the string conversion here.
        if (const String *string = value.stringValue())
            return engine->newUrlObject(QUrl(string->toQString()))->asReturnedValue();
        return engine->newUrlObject()->asReturnedValue();
#if QT_CONFIG(regularexpression)
    case QMetaType::QRegularExpression:
        if (value.as<RegExpObject>())
            return value.asReturnedValue();
        if (const VariantObject *varObject = value.as<VariantObject>()) {
            const QVariant &var = varObject->d()->data();
            if (var.metaType() == QMetaType::fromType<QRegularExpression>())
                return engine->newRegExpObject(var.value<QRegularExpression>())->asReturnedValue();
        }
        return engine->newRegExpObject(QString(), 0)->asReturnedValue();
#endif
    default:
        break;
    }

    if (metaType.flags() & QMetaType::PointerToQObject) {
        return coerceQObject(value, qmlType)
                ? value.asReturnedValue()
                : Encode::null();
    }

    if (const QQmlValueTypeWrapper *wrapper = value.as<QQmlValueTypeWrapper>()) {
        if (wrapper->type() == metaType)
            return value.asReturnedValue();
    }

    if (void *target = QQmlValueTypeProvider::heapCreateValueType(qmlType, value, engine)) {
        Heap::QQmlValueTypeWrapper *wrapper = engine->memoryManager->allocate<QQmlValueTypeWrapper>(
                nullptr, metaType, qmlType.metaObjectForValueType(),
                nullptr, -1, Heap::ReferenceObject::NoFlag);
        Q_ASSERT(!wrapper->gadgetPtr());
        wrapper->setGadgetPtr(target);
        return wrapper->asReturnedValue();
    }

    return Encode::undefined();
}

template<typename Callable>
ReturnedValue coerceAndCall(
    ExecutionEngine *engine,
    const Function::JSTypedFunction *typedFunction, const CompiledData::Function *compiledFunction,
    const Value *argv, int argc, Callable call)
{
    Scope scope(engine);

    QV4::JSCallArguments jsCallData(scope, typedFunction->types.size() - 1);
    const CompiledData::Parameter *formals = compiledFunction->formalsTable();
    for (qsizetype i = 0; i < jsCallData.argc; ++i) {
        jsCallData.args[i] = coerce(
            engine, i < argc ? argv[i] : QV4::Value::fromReturnedValue(Encode::undefined()),
            typedFunction->types[i + 1], formals[i].type.isList());
    }

    ScopedValue result(scope, call(jsCallData.args, jsCallData.argc));
    return coerce(engine, result, typedFunction->types[0], compiledFunction->returnType.isList());
}

// Note: \a to is unininitialized here! This is in contrast to most other related functions.
inline void coerce(
        ExecutionEngine *engine, QMetaType fromType, const void *from, QMetaType toType, void *to)
{
    if ((fromType.flags() & QMetaType::PointerToQObject)
            && (toType.flags() & QMetaType::PointerToQObject)) {
        QObject *fromObj = *static_cast<QObject * const*>(from);
        *static_cast<QObject **>(to)
                = (fromObj && fromObj->metaObject()->inherits(toType.metaObject()))
                    ? fromObj
                    : nullptr;
        return;
    }

    if (toType == QMetaType::fromType<QVariant>()) {
        new (to) QVariant(fromType, from);
        return;
    }

    if (toType == QMetaType::fromType<QJSPrimitiveValue>()) {
        new (to) QJSPrimitiveValue(fromType, from);
        return;
    }

    if (fromType == QMetaType::fromType<QVariant>()) {
        const QVariant *fromVariant = static_cast<const QVariant *>(from);
        if (fromVariant->metaType() == toType)
            toType.construct(to, fromVariant->data());
        else
            coerce(engine, fromVariant->metaType(), fromVariant->data(), toType, to);
        return;
    }

    if (fromType == QMetaType::fromType<QJSPrimitiveValue>()) {
        const QJSPrimitiveValue *fromPrimitive = static_cast<const QJSPrimitiveValue *>(from);
        if (fromPrimitive->metaType() == toType)
            toType.construct(to, fromPrimitive->data());
        else
            coerce(engine, fromPrimitive->metaType(), fromPrimitive->data(), toType, to);
        return;
    }

    // TODO: This is expensive. We might establish a direct C++-to-C++ type coercion, like we have
    //       for JS-to-JS. However, we shouldn't need this very often. Most of the time the compiler
    //       will generate code that passes the right arguments.
    if (toType.flags() & QMetaType::NeedsConstruction)
        toType.construct(to);
    QV4::Scope scope(engine);
    QV4::ScopedValue value(scope, engine->fromData(fromType, from));
    if (!ExecutionEngine::metaTypeFromJS(value, toType, to))
        QMetaType::convert(fromType, from, toType, to);
}

template<typename TypedFunction, typename Callable>
void coerceAndCall(
        ExecutionEngine *engine, const TypedFunction *typedFunction,
        void **argv, const QMetaType *types, int argc, Callable call)
{
    const qsizetype numFunctionArguments = typedFunction->parameterCount();

    Q_ALLOCA_DECLARE(void *, transformedArguments);
    Q_ALLOCA_DECLARE(void, transformedResult);

    const QMetaType returnType = typedFunction->returnMetaType();
    const QMetaType frameReturn = types[0];
    bool returnsQVariantWrapper = false;
    if (argv[0] && returnType != frameReturn) {
        Q_ALLOCA_ASSIGN(void *, transformedArguments, (numFunctionArguments + 1) * sizeof(void *));
        memcpy(transformedArguments, argv, (argc + 1) * sizeof(void *));

        if (frameReturn == QMetaType::fromType<QVariant>()) {
            QVariant *returnValue = static_cast<QVariant *>(argv[0]);
            *returnValue = QVariant(returnType);
            transformedResult = transformedArguments[0] = returnValue->data();
            returnsQVariantWrapper = true;
        } else if (returnType.sizeOf() > 0) {
            Q_ALLOCA_ASSIGN(void, transformedResult, returnType.sizeOf());
            transformedArguments[0] = transformedResult;
            if (returnType.flags() & QMetaType::NeedsConstruction)
                returnType.construct(transformedResult);
        } else {
            transformedResult = transformedArguments[0] = &argc; // Some non-null marker value
        }
    }

    for (qsizetype i = 0; i < numFunctionArguments; ++i) {
        const bool isValid = argc > i;
        const QMetaType frameType = isValid ? types[i + 1] : QMetaType();

        const QMetaType argumentType = typedFunction->parameterMetaType(i);
        if (isValid && argumentType == frameType)
            continue;

        if (transformedArguments == nullptr) {
            Q_ALLOCA_ASSIGN(void *, transformedArguments, (numFunctionArguments + 1) * sizeof(void *));
            memcpy(transformedArguments, argv, (argc + 1) * sizeof(void *));
        }

        if (argumentType.sizeOf() == 0) {
            transformedArguments[i + 1] = nullptr;
            continue;
        }

        void *frameVal = isValid ? argv[i + 1] : nullptr;
        if (isValid && frameType == QMetaType::fromType<QVariant>()) {
            QVariant *variant = static_cast<QVariant *>(frameVal);

            const QMetaType variantType = variant->metaType();
            if (variantType == argumentType) {
                // Slightly nasty, but we're allowed to do this.
                // We don't want to destruct() the QVariant's data() below.
                transformedArguments[i + 1] = argv[i + 1] = variant->data();
            } else {
                Q_ALLOCA_VAR(void, arg, argumentType.sizeOf());
                coerce(engine, variantType, variant->constData(), argumentType, arg);
                transformedArguments[i + 1] = arg;
            }
            continue;
        }

        Q_ALLOCA_VAR(void, arg, argumentType.sizeOf());

        if (isValid)
            coerce(engine, frameType, frameVal, argumentType, arg);
        else
            argumentType.construct(arg);

        transformedArguments[i + 1] = arg;
    }

    if (!transformedArguments) {
        call(argv, numFunctionArguments);
        return;
    }

    call(transformedArguments, numFunctionArguments);

    if (transformedResult && !returnsQVariantWrapper) {
        if (frameReturn.sizeOf() > 0) {
            if (frameReturn.flags() & QMetaType::NeedsDestruction)
                frameReturn.destruct(argv[0]);
            coerce(engine, returnType, transformedResult, frameReturn, argv[0]);
        }
        if (returnType.flags() & QMetaType::NeedsDestruction)
            returnType.destruct(transformedResult);
    }

    for (qsizetype i = 0; i < numFunctionArguments; ++i) {
        void *arg = transformedArguments[i + 1];
        if (arg == nullptr)
            continue;
        if (i >= argc || arg != argv[i + 1]) {
            const QMetaType argumentType = typedFunction->parameterMetaType(i);
            if (argumentType.flags() & QMetaType::NeedsDestruction)
                argumentType.destruct(arg);
        }
    }
}

} // namespace QV4

QT_END_NAMESPACE

#endif // QV4JSCALL_H
