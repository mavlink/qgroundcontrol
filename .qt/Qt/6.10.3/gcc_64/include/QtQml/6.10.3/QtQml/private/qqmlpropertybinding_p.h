// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLPROPERTYBINDING_P_H
#define QQMLPROPERTYBINDING_P_H

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

#include <private/qqmljavascriptexpression_p.h>
#include <private/qqmlpropertydata_p.h>
#include <private/qv4alloca_p.h>
#include <private/qqmltranslation_p.h>

#include <QtCore/qproperty.h>

#include <memory>

QT_BEGIN_NAMESPACE

namespace QV4 {
    struct BoundFunction;
}

class QQmlPropertyBinding;
class QQmlScriptString;

class Q_QML_EXPORT QQmlPropertyBindingJS : public QQmlJavaScriptExpression
{
    bool mustCaptureBindableProperty() const final {return false;}

    friend class QQmlPropertyBinding;
    void expressionChanged() override;
    QQmlPropertyBinding *asBinding()
    {
        return const_cast<QQmlPropertyBinding *>(static_cast<const QQmlPropertyBindingJS *>(this)->asBinding());
    }

    inline QQmlPropertyBinding const *asBinding() const;
};

class Q_QML_EXPORT QQmlPropertyBindingJSForBoundFunction : public QQmlPropertyBindingJS
{
public:
    QV4::ReturnedValue evaluate(bool *isUndefined);
    QV4::PersistentValue m_boundFunction;
};

class Q_QML_EXPORT QQmlPropertyBinding : public QPropertyBindingPrivate

{
    friend class QQmlPropertyBindingJS;

    static constexpr std::size_t jsExpressionOffsetLength() {
        struct composite { QQmlPropertyBinding b; QQmlPropertyBindingJS js; };
        QT_WARNING_PUSH QT_WARNING_DISABLE_INVALID_OFFSETOF
        return sizeof (QQmlPropertyBinding) - offsetof(composite, js);
        QT_WARNING_POP
    }

public:

    QQmlPropertyBindingJS *jsExpression()
    {
        return const_cast<QQmlPropertyBindingJS *>(static_cast<const QQmlPropertyBinding *>(this)->jsExpression());
    }

    QQmlPropertyBindingJS const *jsExpression() const
    {
        return std::launder(reinterpret_cast<QQmlPropertyBindingJS const *>(
                                reinterpret_cast<std::byte const*>(this)
                                + QPropertyBindingPrivate::getSizeEnsuringAlignment()
                                + jsExpressionOffsetLength()));
    }

    static QUntypedPropertyBinding create(const QQmlPropertyData *pd, QV4::Function *function,
                                          QObject *obj, const QQmlRefPointer<QQmlContextData> &ctxt,
                                          QV4::ExecutionContext *scope, QObject *target,
                                          QQmlPropertyIndex targetIndex);
    static QUntypedPropertyBinding create(QMetaType propertyType, QV4::Function *function,
                                          QObject *obj, const QQmlRefPointer<QQmlContextData> &ctxt,
                                          QV4::ExecutionContext *scope, QObject *target,
                                          QQmlPropertyIndex targetIndex);
    static QUntypedPropertyBinding createFromCodeString(const QQmlPropertyData *property,
                                                        const QString &str, QObject *obj,
                                                        const QQmlRefPointer<QQmlContextData> &ctxt,
                                                        const QString &url, quint16 lineNumber,
                                                        QObject *target, QQmlPropertyIndex targetIndex);
    static QUntypedPropertyBinding createFromScriptString(const QQmlPropertyData *property,
                                                          const QQmlScriptString& script, QObject *obj,
                                                          QQmlContext *ctxt, QObject *target,
                                                          QQmlPropertyIndex targetIndex);

    static QUntypedPropertyBinding createFromBoundFunction(const QQmlPropertyData *pd, QV4::BoundFunction *function,
                                          QObject *obj, const QQmlRefPointer<QQmlContextData> &ctxt,
                                          QV4::ExecutionContext *scope, QObject *target,
                                          QQmlPropertyIndex targetIndex);

    static bool isUndefined(const QUntypedPropertyBinding &binding)
    {
        return isUndefined(QPropertyBindingPrivate::get(binding));
    }

    static bool isUndefined(const QPropertyBindingPrivate *binding)
    {
        if (!(binding && binding->hasCustomVTable()))
            return false;
        return static_cast<const QQmlPropertyBinding *>(binding)->isUndefined();
    }

    template<QMetaType::Type type>
    static bool doEvaluate(QMetaType metaType, QUntypedPropertyData *dataPtr, void *f) {
        auto address = static_cast<std::byte*>(f);
        address -= QPropertyBindingPrivate::getSizeEnsuringAlignment(); // f now points to QPropertyBindingPrivate suboject
        // and that has the same address as QQmlPropertyBinding
        return reinterpret_cast<QQmlPropertyBinding *>(address)->evaluate<type>(metaType, dataPtr);
    }

    bool hasDependencies()
    {
        return (dependencyObserverCount > 0) || !jsExpression()->activeGuards.isEmpty();
    }

private:
    template <QMetaType::Type type>
    bool evaluate(QMetaType metaType, void *dataPtr);

    Q_NEVER_INLINE void handleUndefinedAssignment(QQmlEnginePrivate *ep, void *dataPtr);

    QString createBindingLoopErrorDescription();

    struct TargetData {
        enum BoundFunction : bool {
            WithoutBoundFunction = false,
            HasBoundFunction = true,
        };
        TargetData(QObject *target, QQmlPropertyIndex index, BoundFunction state)
            : target(target), targetIndex(index), hasBoundFunction(state)
        {}
        QObject *target;
        QQmlPropertyIndex targetIndex;
        bool hasBoundFunction;
        bool isUndefined = false;
    };
    QQmlPropertyBinding(QMetaType metaType, QObject *target, QQmlPropertyIndex targetIndex, TargetData::BoundFunction hasBoundFunction);

    QObject *target()
    {
        return std::launder(reinterpret_cast<TargetData *>(&declarativeExtraData))->target;
    }

    QQmlPropertyIndex targetIndex()
    {
        return std::launder(reinterpret_cast<TargetData *>(&declarativeExtraData))->targetIndex;
    }

    bool hasBoundFunction()
    {
        return std::launder(reinterpret_cast<TargetData *>(&declarativeExtraData))->hasBoundFunction;
    }

    bool isUndefined() const
    {
        return std::launder(reinterpret_cast<TargetData const *>(&declarativeExtraData))->isUndefined;
    }

    void setIsUndefined(bool isUndefined)
    {
        std::launder(reinterpret_cast<TargetData *>(&declarativeExtraData))->isUndefined = isUndefined;
    }

    static void bindingErrorCallback(QPropertyBindingPrivate *);
};

template <auto I>
struct Print {};

namespace QtPrivate {
template<QMetaType::Type type>
inline constexpr BindingFunctionVTable bindingFunctionVTableForQQmlPropertyBinding = {
    &QQmlPropertyBinding::doEvaluate<type>,
    [](void *qpropertyBinding){
        QQmlPropertyBinding *binding = reinterpret_cast<QQmlPropertyBinding *>(qpropertyBinding);
        binding->jsExpression()->~QQmlPropertyBindingJS();
        binding->~QQmlPropertyBinding();
        auto address = static_cast<std::byte*>(qpropertyBinding);
        delete[] address;
    },
    [](void *, void *){},
    0
};
}

inline const QtPrivate::BindingFunctionVTable *bindingFunctionVTableForQQmlPropertyBinding(QMetaType type)
{
#define FOR_TYPE(TYPE) \
    case TYPE: return &QtPrivate::bindingFunctionVTableForQQmlPropertyBinding<TYPE>
    switch (type.id()) {
        FOR_TYPE(QMetaType::Int);
        FOR_TYPE(QMetaType::QString);
        FOR_TYPE(QMetaType::Double);
        FOR_TYPE(QMetaType::Float);
        FOR_TYPE(QMetaType::Bool);
    default:
        if (type.flags() & QMetaType::PointerToQObject)
            return &QtPrivate::bindingFunctionVTableForQQmlPropertyBinding<QMetaType::QObjectStar>;
        return &QtPrivate::bindingFunctionVTableForQQmlPropertyBinding<QMetaType::UnknownType>;
    }
#undef FOR_TYPE
}

class QQmlTranslationPropertyBinding
{
public:
    static QUntypedPropertyBinding Q_QML_EXPORT create(const QQmlPropertyData *pd,
                                          const QQmlRefPointer<QV4::ExecutableCompilationUnit> &compilationUnit,
                                          const QV4::CompiledData::Binding *binding);
    static QUntypedPropertyBinding Q_QML_EXPORT
    create(const QMetaType &pd,
           const QQmlRefPointer<QV4::ExecutableCompilationUnit> &compilationUnit,
           const QQmlTranslation &translationData);
};

inline const QQmlPropertyBinding *QQmlPropertyBindingJS::asBinding() const
{
    return std::launder(reinterpret_cast<QQmlPropertyBinding const *>(
                            reinterpret_cast<std::byte const*>(this)
                            - QPropertyBindingPrivate::getSizeEnsuringAlignment()
                            - QQmlPropertyBinding::jsExpressionOffsetLength()));
}

static_assert(sizeof(QQmlPropertyBinding) == sizeof(QPropertyBindingPrivate)); // else the whole offset computatation will break
template<typename T>
bool compareAndAssign(void *dataPtr, const void *result)
{
    if (*static_cast<const T *>(result) == *static_cast<const T *>(dataPtr))
        return false;
    *static_cast<T *>(dataPtr) = *static_cast<const T *>(result);
    return true;
}

template <QMetaType::Type type>
bool QQmlPropertyBinding::evaluate(QMetaType metaType, void *dataPtr)
{
    const auto ctxt = jsExpression()->context();
    QQmlEngine *engine = ctxt ? ctxt->engine() : nullptr;
    if (!engine) {
        QPropertyBindingError error(QPropertyBindingError::EvaluationError);
        if (auto currentBinding = QPropertyBindingPrivate::currentlyEvaluatingBinding())
            currentBinding->setError(std::move(error));
        return false;
    }
    QQmlEnginePrivate *ep = QQmlEnginePrivate::get(engine);
    ep->referenceScarceResources();

    const auto handleErrorAndUndefined = [&](bool evaluatedToUndefined) {
        ep->dereferenceScarceResources();
        if (jsExpression()->hasError()) {
            QPropertyBindingError error(QPropertyBindingError::UnknownError,
                                        jsExpression()->delayedError()->error().description());
            QPropertyBindingPrivate::currentlyEvaluatingBinding()->setError(std::move(error));
            bindingErrorCallback(this);
            return false;
        }

        if (evaluatedToUndefined) {
            handleUndefinedAssignment(ep, dataPtr);
            // if property has been changed due to reset, reset is responsible for
            // notifying observers
            return false;
        } else if (isUndefined()) {
            setIsUndefined(false);
        }

        return true;
    };

    if (!hasBoundFunction()) {
        Q_ASSERT(metaType.sizeOf() > 0);

        using Tuple = std::tuple<qsizetype, bool, bool>;
        const auto [size, needsConstruction, needsDestruction] = [&]() -> Tuple {
            switch (type) {
            case QMetaType::QObjectStar: return Tuple(sizeof(QObject *), false, false);
            case QMetaType::Bool:        return Tuple(sizeof(bool), false, false);
            case QMetaType::Int:         return Tuple(sizeof(int), false, false);
            case QMetaType::Double:      return Tuple(sizeof(double), false, false);
            case QMetaType::Float:       return Tuple(sizeof(float), false, false);
            case QMetaType::QString:     return Tuple(sizeof(QString), true, true);
            default: {
                const auto flags = metaType.flags();
                return Tuple(
                        metaType.sizeOf(),
                        flags & QMetaType::NeedsConstruction,
                        flags & QMetaType::NeedsDestruction);
            }
            }
        }();
        Q_ALLOCA_VAR(void, result, size);
        if (needsConstruction)
            metaType.construct(result);

        const bool evaluatedToUndefined = !jsExpression()->evaluate(&result, &metaType, 0);
        if (!handleErrorAndUndefined(evaluatedToUndefined))
            return false;

        switch (type) {
        case QMetaType::QObjectStar:
            return compareAndAssign<QObject *>(dataPtr, result);
        case QMetaType::Bool:
            return compareAndAssign<bool>(dataPtr, result);
        case QMetaType::Int:
            return compareAndAssign<int>(dataPtr, result);
        case QMetaType::Double:
            return compareAndAssign<double>(dataPtr, result);
        case QMetaType::Float:
            return compareAndAssign<float>(dataPtr, result);
        case QMetaType::QString: {
            const bool hasChanged = compareAndAssign<QString>(dataPtr, result);
            static_cast<QString *>(result)->~QString();
            return hasChanged;
        }
        default:
            break;
        }

        const bool hasChanged = !metaType.equals(result, dataPtr);
        if (hasChanged) {
            if (needsDestruction)
                metaType.destruct(dataPtr);
            metaType.construct(dataPtr, result);
        }
        if (needsDestruction)
            metaType.destruct(result);
        return hasChanged;
    }

    bool evaluatedToUndefined = false;
    QV4::Scope scope(engine->handle());
    QV4::ScopedValue result(scope, static_cast<QQmlPropertyBindingJSForBoundFunction *>(
                                jsExpression())->evaluate(&evaluatedToUndefined));

    if (!handleErrorAndUndefined(evaluatedToUndefined))
        return false;

    switch (type) {
    case QMetaType::Bool: {
        bool b;
        if (result->isBoolean())
            b = result->booleanValue();
        else
            b = result->toBoolean();
        if (b == *static_cast<bool *>(dataPtr))
            return false;
        *static_cast<bool *>(dataPtr) = b;
        return true;
    }
    case QMetaType::Int: {
        int i;
        if (result->isInteger())
            i = result->integerValue();
        else if (result->isNumber()) {
            i = QV4::StaticValue::toInteger(result->doubleValue());
        } else {
            break;
        }
        if (i == *static_cast<int *>(dataPtr))
            return false;
        *static_cast<int *>(dataPtr) = i;
        return true;
    }
    case QMetaType::Double:
        if (result->isNumber()) {
            double d = result->asDouble();
            if (d == *static_cast<double *>(dataPtr))
                return false;
            *static_cast<double *>(dataPtr) = d;
            return true;
        }
        break;
    case QMetaType::Float:
        if (result->isNumber()) {
            float d = float(result->asDouble());
            if (d == *static_cast<float *>(dataPtr))
                return false;
            *static_cast<float *>(dataPtr) = d;
            return true;
        }
        break;
    case QMetaType::QString:
        if (result->isString()) {
            QString s = result->toQStringNoThrow();
            if (s == *static_cast<QString *>(dataPtr))
                return false;
            *static_cast<QString *>(dataPtr) = s;
            return true;
        }
        break;
    default:
        break;
    }

    QVariant resultVariant(QV4::ExecutionEngine::toVariant(result, metaType));
    resultVariant.convert(metaType);
    const bool hasChanged = !metaType.equals(resultVariant.constData(), dataPtr);
    metaType.destruct(dataPtr);
    metaType.construct(dataPtr, resultVariant.constData());
    return hasChanged;
}

QT_END_NAMESPACE

#endif // QQMLPROPERTYBINDING_P_H
