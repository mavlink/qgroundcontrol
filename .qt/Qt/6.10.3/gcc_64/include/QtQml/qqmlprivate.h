// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLPRIVATE_H
#define QQMLPRIVATE_H

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

#include <QtQml/qjsprimitivevalue.h>
#include <QtQml/qjsvalue.h>
#include <QtQml/qqmllist.h>
#include <QtQml/qqmlparserstatus.h>
#include <QtQml/qqmlpropertyvaluesource.h>
#include <QtQml/qtqmlglobal.h>

#include <QtCore/qdatetime.h>
#include <QtCore/qdebug.h>
#include <QtCore/qglobal.h>
#include <QtCore/qmetacontainer.h>
#include <QtCore/qmetaobject.h>
#include <QtCore/qpointer.h>
#include <QtCore/qurl.h>
#include <QtCore/qvariant.h>
#include <QtCore/qversionnumber.h>

#include <functional>
#include <limits>
#include <type_traits>

QT_BEGIN_NAMESPACE

class QQmlPropertyValueInterceptor;
class QQmlContextData;
class QQmlFinalizerHook;

namespace QQmlPrivate {
struct CachedQmlUnit;
template<typename A>
using QQmlAttachedPropertiesFunc = A *(*)(QObject *);
}

namespace QV4 {
struct ExecutionEngine;
struct MarkStack;
class ExecutableCompilationUnit;
namespace CompiledData {
struct Unit;
}
}
namespace QmlIR {
struct Document;
typedef void (*IRLoaderFunction)(Document *, const QQmlPrivate::CachedQmlUnit *);
}

using QQmlAttachedPropertiesFunc = QQmlPrivate::QQmlAttachedPropertiesFunc<QObject>;

inline size_t qHash(QQmlAttachedPropertiesFunc func, size_t seed = 0)
{
    return qHash(quintptr(func), seed);
}

template <typename TYPE>
class QQmlTypeInfo
{
public:
    enum {
        hasAttachedProperties = 0
    };
};


class QJSEngine;
class QQmlEngine;
class QQmlCustomParser;
class QQmlTypeNotAvailable;

class QQmlV4Function;
using QQmlV4FunctionPtr = QQmlV4Function *;
using QQmlV4ExecutionEnginePtr = QV4::ExecutionEngine *;

template<class T>
QQmlCustomParser *qmlCreateCustomParser()
{
    return nullptr;
}

namespace QQmlPrivate
{
    void Q_QML_EXPORT qdeclarativeelement_destructor(QObject *);
    template<typename T>
    class QQmlElement final : public T
    {
    public:
        ~QQmlElement() override {
            QQmlPrivate::qdeclarativeelement_destructor(this);
        }
        static void operator delete(void *ptr) {
            // We allocate memory from this class in QQmlType::create
            // along with some additional memory.
            // So we override the operator delete in order to avoid the
            // sized operator delete to be called with a different size than
            // the size that was allocated.
            ::operator delete (ptr);
        }
#ifdef Q_CC_MSVC
        static void operator delete(void *, void *) {
            // Deliberately empty placement delete operator.
            // Silences MSVC warning C4291: no matching operator delete found
            // On MinGW it causes -Wmismatched-new-delete, though.
        }
#endif
    };

    enum class SingletonConstructionMode
    {
        None,
        Constructor,
        Factory,
        FactoryWrapper
    };

    template<typename T, typename WrapperT = T, typename = std::void_t<>>
    struct HasSingletonFactory
    {
        static constexpr bool value = false;
    };

    template<typename T, typename WrapperT>
    struct HasSingletonFactory<T, WrapperT, std::void_t<decltype(WrapperT::create(
                                                               static_cast<QQmlEngine *>(nullptr),
                                                               static_cast<QJSEngine *>(nullptr)))>>
    {
        static constexpr bool value = std::is_same_v<
            decltype(WrapperT::create(static_cast<QQmlEngine *>(nullptr),
                               static_cast<QJSEngine *>(nullptr))), T *>;
    };

    template<typename T, typename WrapperT>
    constexpr SingletonConstructionMode singletonConstructionMode()
    {
        if constexpr (!std::is_base_of<QObject, T>::value)
            return SingletonConstructionMode::None;
        if constexpr (!std::is_same_v<T, WrapperT> && HasSingletonFactory<T, WrapperT>::value)
            return SingletonConstructionMode::FactoryWrapper;
        if constexpr (std::is_default_constructible<T>::value)
            return SingletonConstructionMode::Constructor;
        if constexpr (HasSingletonFactory<T>::value)
            return SingletonConstructionMode::Factory;

        return SingletonConstructionMode::None;
    }

    template<typename>
    struct QmlMarkerFunction;

    template<typename Ret, typename Class>
    struct QmlMarkerFunction<Ret (Class::*)()>
    {
        using ClassType = Class;
    };

    template<typename T, typename Marker>
    using QmlTypeHasMarker = std::is_same<T, typename QmlMarkerFunction<Marker>::ClassType>;

    template<typename T>
    void createInto(void *memory, void *) { new (memory) QQmlElement<T>; }

    template<typename T, typename WrapperT, SingletonConstructionMode Mode>
    QObject *createSingletonInstance(QQmlEngine *q, QJSEngine *j)
    {
        Q_UNUSED(q);
        Q_UNUSED(j);
        if constexpr (Mode == SingletonConstructionMode::Constructor)
            return new T;
        else if constexpr (Mode == SingletonConstructionMode::Factory)
            return T::create(q, j);
        else if constexpr (Mode == SingletonConstructionMode::FactoryWrapper)
            return WrapperT::create(q, j);
        else
            return nullptr;
    }

    template<typename T>
    QObject *createParent(QObject *p) { return new T(p); }

    using CreateIntoFunction = void (*)(void *, void *);
    using CreateSingletonFunction = QObject *(*)(QQmlEngine *, QJSEngine *);
    using CreateParentFunction = QObject *(*)(QObject *);
    using CreateValueTypeFunction = QVariant (*)(const QJSValue &);

    template<typename T, typename WrapperT = T,
             SingletonConstructionMode Mode = singletonConstructionMode<T, WrapperT>()>
    struct Constructors;

    template<typename T, typename WrapperT>
    struct Constructors<T, WrapperT, SingletonConstructionMode::Constructor>
    {
        static constexpr CreateIntoFunction createInto
                = QQmlPrivate::createInto<T>;
        static constexpr CreateSingletonFunction createSingletonInstance
                = QQmlPrivate::createSingletonInstance<
                    T, WrapperT, SingletonConstructionMode::Constructor>;
    };

    template<typename T, typename WrapperT>
    struct Constructors<T, WrapperT, SingletonConstructionMode::None>
    {
        static constexpr CreateIntoFunction createInto = nullptr;
        static constexpr CreateSingletonFunction createSingletonInstance = nullptr;
    };

    template<typename T, typename WrapperT>
    struct Constructors<T, WrapperT, SingletonConstructionMode::Factory>
    {
        static constexpr CreateIntoFunction createInto = nullptr;
        static constexpr CreateSingletonFunction createSingletonInstance
                = QQmlPrivate::createSingletonInstance<
                    T, WrapperT, SingletonConstructionMode::Factory>;
    };

    template<typename T, typename WrapperT>
    struct Constructors<T, WrapperT, SingletonConstructionMode::FactoryWrapper>
    {
        static constexpr CreateIntoFunction createInto = nullptr;
        static constexpr CreateSingletonFunction createSingletonInstance
                = QQmlPrivate::createSingletonInstance<
                    T, WrapperT, SingletonConstructionMode::FactoryWrapper>;
    };

    template<typename T,
             bool IsObject = std::is_base_of<QObject, T>::value,
             bool IsGadget = QtPrivate::IsGadgetHelper<T>::IsRealGadget>
    struct ExtendedType;

    template<typename T>
    struct ExtendedType<T, false, false>
    {
        static constexpr const CreateParentFunction createParent = nullptr;
        static const QMetaObject *staticMetaObject() { return nullptr; }
    };

    // If it's a QObject, we actually want an error if the ctor or the metaobject is missing.
    template<typename T>
    struct ExtendedType<T, true, false>
    {
        static constexpr const CreateParentFunction createParent = QQmlPrivate::createParent<T>;
        static const QMetaObject *staticMetaObject() { return &T::staticMetaObject; }
    };

    // If it's a Q_GADGET, we don't want the ctor.
    template<typename T>
    struct ExtendedType<T, false, true>
    {
        static constexpr const CreateParentFunction createParent = nullptr;
        static const QMetaObject *staticMetaObject() { return &T::staticMetaObject; }
    };

    template<typename F, typename Result = void>
    struct ValueTypeFactory
    {
        static constexpr const Result (*create)(const QJSValue &) = nullptr;
    };

    template<typename F>
    struct ValueTypeFactory<F, std::void_t<decltype(F::create(QJSValue()))>>
    {
        static decltype(F::create(QJSValue())) create(const QJSValue &params)
        {
            return F::create(params);
        }
    };

    template<typename T, typename F,
             bool HasCtor = std::is_constructible_v<T, QJSValue>,
             bool HasFactory = std::is_constructible_v<
                 QVariant, decltype(ValueTypeFactory<F>::create(QJSValue()))>>
    struct ValueType;

    template<typename T, typename F>
    struct ValueType<T, F, false, false>
    {
        static constexpr const CreateValueTypeFunction create = nullptr;
    };

    template<typename T, typename F, bool HasCtor>
    struct ValueType<T, F, HasCtor, true>
    {
        static QVariant create(const QJSValue &params)
        {
            return F::create(params);
        }
    };

    template<typename T, typename F>
    struct ValueType<T, F, true, false>
    {
        static QVariant create(const QJSValue &params)
        {
            return QVariant::fromValue(T(params));
        }
    };

    template<class From, class To, int N>
    struct StaticCastSelectorClass
    {
        static inline int cast() { return -1; }
    };

    template<class From, class To>
    struct StaticCastSelectorClass<From, To, sizeof(int)>
    {
        static inline int cast() { return int(reinterpret_cast<quintptr>(static_cast<To *>(reinterpret_cast<From *>(0x10000000)))) - 0x10000000; }
    };

    template<class From, class To>
    struct StaticCastSelector
    {
        typedef int yes_type;
        typedef char no_type;

        static yes_type checkType(To *);
        static no_type checkType(...);

        static inline int cast()
        {
            return StaticCastSelectorClass<From, To, sizeof(checkType(reinterpret_cast<From *>(0)))>::cast();
        }
    };

    // You can prevent subclasses from using the same attached type by specialzing this.
    // This is reserved for internal types, though.
    template<class T, class A>
    struct OverridableAttachedType
    {
        using Type = A;
    };

    template<class T, class = std::void_t<>, bool OldStyle = QQmlTypeInfo<T>::hasAttachedProperties>
    struct QmlAttached
    {
        using Type = void;
        using Func = QQmlAttachedPropertiesFunc<QObject>;
        static const QMetaObject *staticMetaObject() { return nullptr; }
        static Func attachedPropertiesFunc() { return nullptr; }
    };

    // Defined inline via QML_ATTACHED
    template<class T>
    struct QmlAttached<T, std::void_t<typename OverridableAttachedType<T, typename T::QmlAttachedType>::Type>, false>
    {
        // Normal attached properties
        template <typename Parent, typename Attached>
        struct Properties
        {
            using Func = QQmlAttachedPropertiesFunc<Attached>;
            static const QMetaObject *staticMetaObject() { return &Attached::staticMetaObject; }
            static Func attachedPropertiesFunc() { return Parent::qmlAttachedProperties; }
        };

        // Disabled via OverridableAttachedType
        template<typename Parent>
        struct Properties<Parent, void>
        {
            using Func = QQmlAttachedPropertiesFunc<QObject>;
            static const QMetaObject *staticMetaObject() { return nullptr; }
            static Func attachedPropertiesFunc() { return nullptr; }
        };

        using Type = typename std::conditional<
                QmlTypeHasMarker<T, decltype(&T::qt_qmlMarker_attached)>::value,
                typename OverridableAttachedType<T, typename T::QmlAttachedType>::Type, void>::type;
        using Func = typename Properties<T, Type>::Func;

        static const QMetaObject *staticMetaObject()
        {
            return Properties<T, Type>::staticMetaObject();
        }

        static Func attachedPropertiesFunc()
        {
            return Properties<T, Type>::attachedPropertiesFunc();
        }
    };

    // Separately defined via QQmlTypeInfo
    template<class T>
    struct QmlAttached<T, std::void_t<decltype(T::qmlAttachedProperties)>, true>
    {
        using Type = typename std::remove_pointer<decltype(T::qmlAttachedProperties(nullptr))>::type;
        using Func = QQmlAttachedPropertiesFunc<Type>;

        static const QMetaObject *staticMetaObject() { return &Type::staticMetaObject; }
        static Func attachedPropertiesFunc() { return T::qmlAttachedProperties; }
    };

    // This is necessary because both the type containing a default template parameter and the type
    // instantiating the template need to have access to the default template parameter type. In
    // this case that's T::QmlAttachedType. The QML_FOREIGN macro needs to befriend specific other
    // types. Therefore we need some kind of "accessor". Because of compiler bugs in gcc and clang,
    // we cannot befriend attachedPropertiesFunc() directly. Wrapping the actual access into another
    // struct "fixes" that. For convenience we still want the free standing functions in addition.
    template<class T>
    struct QmlAttachedAccessor
    {
        static QQmlAttachedPropertiesFunc<QObject> attachedPropertiesFunc()
        {
            return QQmlAttachedPropertiesFunc<QObject>(QmlAttached<T>::attachedPropertiesFunc());
        }

        static const QMetaObject *staticMetaObject()
        {
            return QmlAttached<T>::staticMetaObject();
        }
    };

    template<typename T>
    inline QQmlAttachedPropertiesFunc<QObject> attachedPropertiesFunc()
    {
        return QmlAttachedAccessor<T>::attachedPropertiesFunc();
    }

    template<typename T>
    inline const QMetaObject *attachedPropertiesMetaObject()
    {
        return QmlAttachedAccessor<T>::staticMetaObject();
    }

    enum AutoParentResult { Parented, IncompatibleObject, IncompatibleParent };
    typedef AutoParentResult (*AutoParentFunction)(QObject *object, QObject *parent);

    enum class ValueTypeCreationMethod { None, Construct, Structured };

    struct RegisterType {
        enum StructVersion: int {
            Base = 0,
            FinalizerCast = 1,
            CreationMethod = 2,
            CurrentVersion = CreationMethod,
        };

        bool has(StructVersion v) const { return structVersion >= int(v); }

        int structVersion;

        QMetaType typeId;
        QMetaType listId;
        int objectSize;
        // The second parameter of create is for userdata
        void (*create)(void *, void *);
        void *userdata;
        QString noCreationReason;

        // ### Qt7: Get rid of this. It can be covered by creationMethod below.
        QVariant (*createValueType)(const QJSValue &);

        const char *uri;
        QTypeRevision version;
        const char *elementName;
        const QMetaObject *metaObject;

        QQmlAttachedPropertiesFunc<QObject> attachedPropertiesFunction;
        const QMetaObject *attachedPropertiesMetaObject;

        int parserStatusCast;
        int valueSourceCast;
        int valueInterceptorCast;

        QObject *(*extensionObjectCreate)(QObject *);
        const QMetaObject *extensionMetaObject;

        QQmlCustomParser *customParser;

        QTypeRevision revision;
        int finalizerCast;

        ValueTypeCreationMethod creationMethod;
        // If this is extended ensure "version" is bumped!!!
    };

    struct RegisterTypeAndRevisions {
        int structVersion;

        QMetaType typeId;
        QMetaType listId;
        int objectSize;
        void (*create)(void *, void *);
        void *userdata;

        QVariant (*createValueType)(const QJSValue &);

        const char *uri;
        QTypeRevision version;

        const QMetaObject *metaObject;
        const QMetaObject *classInfoMetaObject;

        QQmlAttachedPropertiesFunc<QObject> attachedPropertiesFunction;
        const QMetaObject *attachedPropertiesMetaObject;

        int parserStatusCast;
        int valueSourceCast;
        int valueInterceptorCast;

        QObject *(*extensionObjectCreate)(QObject *);
        const QMetaObject *extensionMetaObject;

        QQmlCustomParser *(*customParserFactory)();
        QVector<int> *qmlTypeIds;
        int finalizerCast;

        bool forceAnonymous;
        QMetaSequence listMetaSequence;
    };

    struct RegisterInterface {
        int structVersion;

        QMetaType typeId;
        QMetaType listId;

        const char *iid;

        const char *uri;
        QTypeRevision version;
    };

    struct RegisterAutoParent {
        int structVersion;

        AutoParentFunction function;
    };

    struct RegisterSingletonType {
        int structVersion;

        const char *uri;
        QTypeRevision version;
        const char *typeName;

        std::function<QJSValue(QQmlEngine *, QJSEngine *)> scriptApi;
        std::function<QObject*(QQmlEngine *, QJSEngine *)> qObjectApi;

        const QMetaObject *instanceMetaObject;
        QMetaType typeId;

        QObject *(*extensionObjectCreate)(QObject *);
        const QMetaObject *extensionMetaObject;

        QTypeRevision revision;
    };

    struct RegisterSingletonTypeAndRevisions {
        int structVersion;
        const char *uri;
        QTypeRevision version;

        std::function<QObject*(QQmlEngine *, QJSEngine *)> qObjectApi;

        const QMetaObject *instanceMetaObject;
        const QMetaObject *classInfoMetaObject;

        QMetaType typeId;

        QObject *(*extensionObjectCreate)(QObject *);
        const QMetaObject *extensionMetaObject;

        QVector<int> *qmlTypeIds;
    };

    struct RegisterCompositeType {
        int structVersion;
        QUrl url;
        const char *uri;
        QTypeRevision version;
        const char *typeName;
    };

    struct RegisterCompositeSingletonType {
        int structVersion;
        QUrl url;
        const char *uri;
        QTypeRevision version;
        const char *typeName;
    };

    struct RegisterSequentialContainer {
        int structVersion;
        const char *uri;
        QTypeRevision version;

        // ### Qt7: Remove typeName. It's ignored because the only valid name is "list",
        //          and that's automatic.
        const char *typeName;

        QMetaType typeId;
        QMetaSequence metaSequence;
        QTypeRevision revision;
    };

    struct RegisterSequentialContainerAndRevisions {
        int structVersion;
        const char *uri;
        QTypeRevision version;

        const QMetaObject *classInfoMetaObject;
        QMetaType typeId;
        QMetaSequence metaSequence;

        QVector<int> *qmlTypeIds;
    };

    struct AOTTrackedLocalsStorage
    {
        virtual ~AOTTrackedLocalsStorage() = default;
        virtual void markObjects(QV4::MarkStack *markStack) const = 0;
    };

    struct Q_QML_EXPORT AOTCompiledContext {
        enum: uint { InvalidStringId = (std::numeric_limits<uint>::max)() };

        QQmlContextData *qmlContext;
        QObject *qmlScopeObject;
        QJSEngine *engine;
        union {
            QV4::ExecutableCompilationUnit *compilationUnit;
            qintptr extraData;
        };

        QObject *thisObject() const;
        QQmlEngine *qmlEngine() const;

        QJSValue jsMetaType(int index) const;
        void setInstructionPointer(int offset) const;
        void setLocals(const AOTTrackedLocalsStorage *locals) const;
        void setReturnValueUndefined() const;

        static void mark(QObject *object, QV4::MarkStack *markStack);
        static void mark(const QVariant &variant, QV4::MarkStack *markStack);
        template<typename T>
        static void mark(T, QV4::MarkStack *) {}

        // Run QQmlPropertyCapture::captureProperty() without retrieving the value.
        bool captureLookup(uint index, QObject *object) const;
        bool captureQmlContextPropertyLookup(uint index) const;
        void captureTranslation() const;
        QString translationContext() const;
        QMetaType lookupResultMetaType(uint index) const;
        void storeNameSloppy(uint nameIndex, void *value, QMetaType type) const;
        QJSValue javaScriptGlobalProperty(uint nameIndex) const;

        const QLoggingCategory *resolveLoggingCategory(QObject *wrapper, bool *ok) const;

        void writeToConsole(
                QtMsgType type, const QString &message,
                const QLoggingCategory *loggingCategory) const;

        QVariant constructValueType(
                QMetaType resultMetaType, const QMetaObject *resultMetaObject,
                int ctorIndex, void **args) const;
#if QT_QML_REMOVED_SINCE(6, 9)
        QVariant constructValueType(
                QMetaType resultMetaType, const QMetaObject *resultMetaObject,
                int ctorIndex, void *ctorArg) const;
#endif

        // Those are explicit arguments to the Date() ctor, not implicit coercions.
        QDateTime constructDateTime(double timestamp) const;
        QDateTime constructDateTime(const QString &string) const;
        QDateTime constructDateTime(const QJSPrimitiveValue &arg) const
        {
            return arg.type() == QJSPrimitiveValue::String
                    ? constructDateTime(arg.toString())
                    : constructDateTime(arg.toDouble());
        }

        QDateTime constructDateTime(
                double year, double month, double day = 1,
                double hours = 0, double minutes = 0, double seconds = 0, double msecs = 0) const;

        // All of these lookup functions should be used as follows:
        //
        // while (!fooBarLookup(...)) {
        //     setInstructionPointer(...);
        //     initFooBarLookup(...);
        //     if (engine->hasException()) {
        //         ...
        //         break;
        //     }
        // }
        //
        // The bool-returning *Lookup functions exclusively run the happy path and return false if
        // that fails in any way. The failure may either be in the lookup structs not being
        // initialized or an exception being thrown.
        // The init*Lookup functions initialize the lookup structs and amend any exceptions
        // previously thrown with line numbers. They might also throw their own exceptions. If an
        // exception is present after the initialization there is no way to carry out the lookup and
        // the exception should be propagated. If not, the original lookup can be tried again.

        bool callQmlContextPropertyLookup(uint index, void **args, int argc) const;
        void initCallQmlContextPropertyLookup(uint index, int relativeMethodIndex) const;

#if QT_QML_REMOVED_SINCE(6, 9)
        bool callQmlContextPropertyLookup(
                uint index, void **args, const QMetaType *types, int argc) const;
        void initCallQmlContextPropertyLookup(uint index) const;
#endif

        bool loadContextIdLookup(uint index, void *target) const;
        void initLoadContextIdLookup(uint index) const;

        bool callObjectPropertyLookup(uint index, QObject *object, void **args, int argc) const;
        void initCallObjectPropertyLookup(
                uint index, QObject *object, int relativeMethodIndex) const;
        void initCallObjectPropertyLookupAsVariant(uint index, QObject *object) const;

#if QT_QML_REMOVED_SINCE(6, 9)
        bool callObjectPropertyLookup(
                uint index, QObject *object, void **args, const QMetaType *types, int argc) const;
        void initCallObjectPropertyLookup(uint index) const;

        bool callGlobalLookup(uint index, void **args, const QMetaType *types, int argc) const;
        void initCallGlobalLookup(uint index) const;

        bool loadGlobalLookup(uint index, void *target, QMetaType type) const;
        void initLoadGlobalLookup(uint index) const;
#endif

        bool loadGlobalLookup(uint index, void *target) const;
        void initLoadGlobalLookup(uint index, QMetaType type) const;

        bool loadScopeObjectPropertyLookup(uint index, void *target) const;
        bool writeBackScopeObjectPropertyLookup(uint index, void *source) const;
        void initLoadScopeObjectPropertyLookup(uint index) const;
#if QT_QML_REMOVED_SINCE(6, 9)
        void initLoadScopeObjectPropertyLookup(uint index, QMetaType type) const;
#endif

        bool loadSingletonLookup(uint index, void *target) const;
        void initLoadSingletonLookup(uint index, uint importNamespace) const;

        bool loadAttachedLookup(uint index, QObject *object, void *target) const;
        void initLoadAttachedLookup(uint index, uint importNamespace, QObject *object) const;

        bool loadTypeLookup(uint index, void *target) const;
        void initLoadTypeLookup(uint index, uint importNamespace) const;

        bool getObjectLookup(uint index, QObject *object, void *target) const;
        bool writeBackObjectLookup(uint index, QObject *object, void *source) const;
        void initGetObjectLookup(uint index, QObject *object) const;
        void initGetObjectLookupAsVariant(uint index, QObject *object) const;
#if QT_QML_REMOVED_SINCE(6, 9)
        void initGetObjectLookup(uint index, QObject *object, QMetaType type) const;
#endif

        bool getValueLookup(uint index, void *value, void *target) const;
        bool writeBackValueLookup(uint index, void *value, void *source) const;
        void initGetValueLookup(uint index, const QMetaObject *metaObject) const;
#if QT_QML_REMOVED_SINCE(6, 9)
        void initGetValueLookup(uint index, const QMetaObject *metaObject, QMetaType type) const;
#endif

        bool getEnumLookup(uint index, void *target) const;
#if QT_QML_REMOVED_SINCE(6, 6)
        bool getEnumLookup(uint index, int *target) const;
#endif
        void initGetEnumLookup(uint index, const QMetaObject *metaObject,
                               const char *enumerator, const char *enumValue) const;

        bool setObjectLookup(uint index, QObject *object, void *value) const;
        void initSetObjectLookup(uint index, QObject *object) const;
        void initSetObjectLookupAsVariant(uint index, QObject *object) const;
#if QT_QML_REMOVED_SINCE(6, 9)
        void initSetObjectLookup(uint index, QObject *object, QMetaType type) const;
#endif

        bool setValueLookup(uint index, void *target, void *value) const;
        void initSetValueLookup(uint index, const QMetaObject *metaObject) const;
        void initSetValueLookupAsVariant(uint index, const QMetaObject *metaObject) const;
#if QT_QML_REMOVED_SINCE(6, 9)
        void initSetValueLookup(uint index, const QMetaObject *metaObject, QMetaType type) const;
#endif

        bool callValueLookup(uint index, void *target, void **args, int argc) const;
        void initCallValueLookup(
                uint index, const QMetaObject *metaObject, int relativeMethodIndex) const;

        void setObjectImplicitDestructible(QObject *object) const;
        void setImplicitDestructible(QObject *object) const
        {
            if (object)
                setObjectImplicitDestructible(object);
        }

        void setImplicitDestructible(const QVariant &variant) const
        {
            // This does not cover everything you can possibly wrap into a QVariant.
            // However, since we only _promise_ to handle single objects, this is OK.
            if (!variant.metaType().flags().testFlag(QMetaType::PointerToQObject))
                return;

            if (QObject *object = *static_cast<QObject *const *>(variant.constData()))
                setObjectImplicitDestructible(object);
        }

        template<typename Value>
        void setImplicitDestructible(const Value &value) const { Q_UNUSED(value); }
    };

    struct AOTCompiledFunction {
        int functionIndex;
        int numArguments;
        void (*signature)(QV4::ExecutableCompilationUnit *unit, QMetaType *argTypes);
        void (*functionPtr)(const AOTCompiledContext *context, void **argv);
    };

#if QT_DEPRECATED_SINCE(6, 6)
    QT_DEPRECATED_VERSION_X(6, 6, "Use AOTCompiledFunction instead")
    typedef AOTCompiledFunction TypedFunction;
#endif

    struct CachedQmlUnit {
        const QV4::CompiledData::Unit *qmlData;
        const AOTCompiledFunction *aotCompiledFunctions;
        void *unused2;
    };

    typedef const CachedQmlUnit *(*QmlUnitCacheLookupFunction)(const QUrl &url);
    struct RegisterQmlUnitCacheHook {
        int structVersion;
        QmlUnitCacheLookupFunction lookupCachedQmlUnit;
    };

    enum RegistrationType {
        TypeRegistration       = 0,
        InterfaceRegistration  = 1,
        AutoParentRegistration = 2,
        SingletonRegistration  = 3,
        CompositeRegistration  = 4,
        CompositeSingletonRegistration = 5,
        QmlUnitCacheHookRegistration = 6,
        TypeAndRevisionsRegistration = 7,
        SingletonAndRevisionsRegistration = 8,
        SequentialContainerRegistration = 9,
        SequentialContainerAndRevisionsRegistration = 10,
    };

    int Q_QML_EXPORT qmlregister(RegistrationType, void *);
    void Q_QML_EXPORT qmlunregister(RegistrationType, quintptr);

#if QT_DEPRECATED_SINCE(6, 3)
    struct Q_QML_EXPORT SingletonFunctor
    {
        QT_DEPRECATED QObject *operator()(QQmlEngine *, QJSEngine *);
        QPointer<QObject> m_object;
        bool alreadyCalled = false;
    };
#endif

    struct Q_QML_EXPORT SingletonInstanceFunctor
    {
        QObject *operator()(QQmlEngine *, QJSEngine *);

        QPointer<QObject> m_object;

        // Not a QPointer, so that you cannot assign it to a different
        // engine when the first one is deleted.
        // That would mess up the QML contexts.
        QQmlEngine *m_engine = nullptr;
    };

    static int indexOfOwnClassInfo(const QMetaObject *metaObject, const char *key, int startOffset = -1)
    {
        if (!metaObject || !key)
            return -1;

        const int offset = metaObject->classInfoOffset();
        const int start = (startOffset == -1)
                ? (metaObject->classInfoCount() + offset - 1)
                : startOffset;
        for (int i = start; i >= offset; --i)
            if (qstrcmp(key, metaObject->classInfo(i).name()) == 0) {
                return i;
        }
        return -1;
    }

    inline const char *classInfo(const QMetaObject *metaObject, const char *key)
    {
        return metaObject->classInfo(indexOfOwnClassInfo(metaObject, key)).value();
    }

    inline QTypeRevision revisionClassInfo(const QMetaObject *metaObject, const char *key,
                                       QTypeRevision defaultValue = QTypeRevision())
    {
        const int index = indexOfOwnClassInfo(metaObject, key);
        return (index == -1) ? defaultValue
                             : QTypeRevision::fromEncodedVersion(
                                   QLatin1StringView(metaObject->classInfo(index).value()).toInt());
    }

    Q_QML_EXPORT QList<QTypeRevision> revisionClassInfos(const QMetaObject *metaObject, const char *key);

    inline bool boolClassInfo(const QMetaObject *metaObject, const char *key,
                              bool defaultValue = false)
    {
        const int index = indexOfOwnClassInfo(metaObject, key);
        if (index == -1)
            return defaultValue;
        return qstrcmp(metaObject->classInfo(index).value(), "true") == 0;
    }

    template<class T, class = std::void_t<>>
    struct QmlExtended
    {
        using Type = void;
    };

    template<class T>
    struct QmlExtended<T, std::void_t<typename T::QmlExtendedType>>
    {
        using Type = typename std::conditional<
                QmlTypeHasMarker<T, decltype(&T::qt_qmlMarker_extended)>::value,
                typename T::QmlExtendedType, void>::type;
    };

    template<class T, class = std::void_t<>>
    struct QmlExtendedNamespace
    {
        static constexpr const QMetaObject *metaObject() { return nullptr; }
    };

    template<class T>
    struct QmlExtendedNamespace<T, std::void_t<decltype(T::qmlExtendedNamespace())>>
    {
        static constexpr const QMetaObject *metaObject()
        {
            if constexpr (QmlTypeHasMarker<T, decltype(&T::qt_qmlMarker_extendedNamespace)>::value)
                return T::qmlExtendedNamespace();
            else
                return nullptr;
        }
    };

    template<class T, class = std::void_t<>>
    struct QmlResolved
    {
        using Type = T;
    };

    template<class T>
    struct QmlResolved<T, std::void_t<typename T::QmlForeignType>>
    {
        using Type = typename std::conditional<
                QmlTypeHasMarker<T, decltype(&T::qt_qmlMarker_foreign)>::value,
                typename T::QmlForeignType, T>::type;
    };

    template<class T, class = std::void_t<>>
    struct QmlUncreatable
    {
        static constexpr bool Value = false;
    };

    template<class T>
    struct QmlUncreatable<T, std::void_t<typename T::QmlIsUncreatable>>
    {
        static constexpr bool Value =
                QmlTypeHasMarker<T, decltype(&T::qt_qmlMarker_uncreatable)>::value
                && bool(T::QmlIsUncreatable::yes);
    };

    template<class T, class = std::void_t<>>
    struct QmlAnonymous
    {
        static constexpr bool Value = false;
    };

    template<class T>
    struct QmlAnonymous<T, std::void_t<typename T::QmlIsAnonymous>>
    {
        static constexpr bool Value =
                QmlTypeHasMarker<T, decltype(&T::qt_qmlMarker_anonymous)>::value
                && bool(T::QmlIsAnonymous::yes);
    };


    template<class T, class = std::void_t<>>
    struct QmlSingleton
    {
        static constexpr bool Value = false;
    };

    template<class T>
    struct QmlSingleton<T, std::void_t<typename T::QmlIsSingleton>>
    {
        static constexpr bool Value =
                QmlTypeHasMarker<T, decltype(&T::qt_qmlMarker_singleton)>::value
                && bool(T::QmlIsSingleton::yes);
    };

    template<class T, class = std::void_t<>>
    struct QmlSequence
    {
        static constexpr bool Value = false;
    };

    template<class T>
    struct QmlSequence<T, std::void_t<typename T::QmlIsSequence>>
    {
        Q_STATIC_ASSERT((std::is_same_v<typename T::QmlSequenceValueType,
                                        typename QmlResolved<T>::Type::value_type>));
        static constexpr bool Value = bool(T::QmlIsSequence::yes);
    };

    template<class T, class = std::void_t<>>
    struct QmlInterface
    {
        static constexpr bool Value = false;
    };

    template<class T>
    struct QmlInterface<T, std::void_t<typename T::QmlIsInterface, decltype(qobject_interface_iid<T *>())>>
    {
        static constexpr bool Value = bool(T::QmlIsInterface::yes);
    };

    template<class T, typename = std::void_t<>>
    struct StaticMetaObject
    {
        static const QMetaObject *staticMetaObject() { return nullptr; }
    };

    template<class T>
    struct StaticMetaObject<T, std::void_t<decltype(T::staticMetaObject)>>
    {
        static const QMetaObject *staticMetaObject() { return &T::staticMetaObject; }
    };

    template<class T>
    struct QmlMetaType
    {
        static constexpr bool hasAcceptableCtors()
        {
            if constexpr (!std::is_default_constructible_v<T>)
                return false;
            else if constexpr (std::is_base_of_v<QObject, T>)
                return true;
            else
                return std::is_copy_constructible_v<T>;
        }

        static constexpr QMetaType self()
        {
            if constexpr (std::is_base_of_v<QObject, T>)
                return QMetaType::fromType<T*>();
            else
                return QMetaType::fromType<T>();
        }

        static constexpr QMetaType list()
        {
            if constexpr (std::is_base_of_v<QObject, T>)
                return QMetaType::fromType<QQmlListProperty<T>>();
            else
                return QMetaType::fromType<QList<T>>();
        }

        static constexpr QMetaSequence sequence()
        {
            if constexpr (std::is_base_of_v<QObject, T>)
                return QMetaSequence();
            else
                return QMetaSequence::fromContainer<QList<T>>();
        }

        static constexpr int size()
        {
            return sizeof(T);
        }
    };

    template<>
    struct QmlMetaType<void>
    {
        static constexpr bool hasAcceptableCtors() { return true; }
        static constexpr QMetaType self() { return QMetaType(); }
        static constexpr QMetaType list() { return QMetaType(); }
        static constexpr QMetaSequence sequence() { return QMetaSequence(); }
        static constexpr int size() { return 0; }
    };

    template<typename T, typename E, typename WrapperT = T>
    void qmlRegisterSingletonAndRevisions(const char *uri, int versionMajor,
                                          const QMetaObject *classInfoMetaObject,
                                          QVector<int> *qmlTypeIds, const QMetaObject *extension)
    {
        static_assert(std::is_base_of_v<QObject, T>);
        RegisterSingletonTypeAndRevisions api = {
            0,

            uri,
            QTypeRevision::fromMajorVersion(versionMajor),

            Constructors<T, WrapperT>::createSingletonInstance,

            StaticMetaObject<T>::staticMetaObject(),
            classInfoMetaObject,

            QmlMetaType<T>::self(),

            ExtendedType<E>::createParent,
            extension ? extension : ExtendedType<E>::staticMetaObject(),

            qmlTypeIds
        };

        qmlregister(SingletonAndRevisionsRegistration, &api);
    }

    template<typename T, typename E>
    void qmlRegisterTypeAndRevisions(const char *uri, int versionMajor,
                                     const QMetaObject *classInfoMetaObject,
                                     QVector<int> *qmlTypeIds, const QMetaObject *extension,
                                     bool forceAnonymous = false)
    {
        RegisterTypeAndRevisions type = {
            3,
            QmlMetaType<T>::self(),
            QmlMetaType<T>::list(),
            QmlMetaType<T>::size(),
            Constructors<T>::createInto,
            nullptr,
            ValueType<T, E>::create,

            uri,
            QTypeRevision::fromMajorVersion(versionMajor),

            StaticMetaObject<T>::staticMetaObject(),
            classInfoMetaObject,

            attachedPropertiesFunc<T>(),
            attachedPropertiesMetaObject<T>(),

            StaticCastSelector<T, QQmlParserStatus>::cast(),
            StaticCastSelector<T, QQmlPropertyValueSource>::cast(),
            StaticCastSelector<T, QQmlPropertyValueInterceptor>::cast(),

            ExtendedType<E>::createParent,
            extension ? extension : ExtendedType<E>::staticMetaObject(),

            &qmlCreateCustomParser<T>,
            qmlTypeIds,
            StaticCastSelector<T, QQmlFinalizerHook>::cast(),

            forceAnonymous,
            QmlMetaType<T>::sequence(),
        };

        // Initialize the extension so that we can find it by name or ID.
        qMetaTypeId<E>();

        qmlregister(TypeAndRevisionsRegistration, &type);
    }

    template<typename T>
    void qmlRegisterSequenceAndRevisions(const char *uri, int versionMajor,
                                         const QMetaObject *classInfoMetaObject,
                                         QVector<int> *qmlTypeIds)
    {
        RegisterSequentialContainerAndRevisions type = {
            0,
            uri,
            QTypeRevision::fromMajorVersion(versionMajor),
            classInfoMetaObject,
            QMetaType::fromType<T>(),
            QMetaSequence::fromContainer<T>(),
            qmlTypeIds
        };

        qmlregister(SequentialContainerAndRevisionsRegistration, &type);
    }

    template<>
    void Q_QML_EXPORT qmlRegisterTypeAndRevisions<QQmlTypeNotAvailable, void>(
            const char *uri, int versionMajor, const QMetaObject *classInfoMetaObject,
            QVector<int> *qmlTypeIds, const QMetaObject *, bool);

    constexpr QtPrivate::QMetaTypeInterface metaTypeForNamespace(
            const QtPrivate::QMetaTypeInterface::MetaObjectFn &metaObjectFunction, const char *name)
    {
        return {
            /*.revision=*/ 0,
            /*.alignment=*/ 0,
            /*.size=*/ 0,
            /*.flags=*/ 0,
            /*.typeId=*/ QBasicAtomicInt(),
            /*.metaObject=*/ metaObjectFunction,
            /*.name=*/ name,
            /*.defaultCtr=*/ nullptr,
            /*.copyCtr=*/ nullptr,
            /*.moveCtr=*/ nullptr,
            /*.dtor=*/ nullptr,
            /*.equals*/ nullptr,
            /*.lessThan*/ nullptr,
            /*.debugStream=*/ nullptr,
            /*.dataStreamOut=*/ nullptr,
            /*.dataStreamIn=*/ nullptr,
            /*.legacyRegisterOp=*/ nullptr
        };
    }

    Q_QML_EXPORT QObject *qmlExtendedObject(QObject *, int);

    enum QmlRegistrationWarning {
        UnconstructibleType,
        UnconstructibleSingleton,
        NonQObjectWithAtached,
    };

    Q_QML_EXPORT void qmlRegistrationWarning(QmlRegistrationWarning warning, QMetaType type);

    Q_QML_EXPORT QMetaType compositeMetaType(
            QV4::ExecutableCompilationUnit *unit, int elementNameId);
    Q_QML_EXPORT QMetaType compositeMetaType(
            QV4::ExecutableCompilationUnit *unit, const QString &elementName);
    Q_QML_EXPORT QMetaType compositeListMetaType(
            QV4::ExecutableCompilationUnit *unit, int elementNameId);
    Q_QML_EXPORT QMetaType compositeListMetaType(
            QV4::ExecutableCompilationUnit *unit, const QString &elementName);

} // namespace QQmlPrivate

QT_END_NAMESPACE

Q_DECLARE_OPAQUE_POINTER(QQmlV4FunctionPtr)
Q_DECLARE_OPAQUE_POINTER(QQmlV4ExecutionEnginePtr)

#endif // QQMLPRIVATE_H
