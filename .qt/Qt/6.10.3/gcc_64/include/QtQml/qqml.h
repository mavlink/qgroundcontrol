// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQML_H
#define QQML_H

#include <QtQml/qqmlprivate.h>
#include <QtQml/qjsvalue.h>
#include <QtQml/qqmlregistration.h>

#include <QtCore/qbytearray.h>
#include <QtCore/qmetaobject.h>
#include <QtCore/qmetacontainer.h>
#include <QtCore/qversionnumber.h>

#define QML_VERSION     0x020000
#define QML_VERSION_STR "2.0"

#define QML_DECLARE_TYPE(TYPE) \
    Q_DECLARE_METATYPE(TYPE*) \
    Q_DECLARE_METATYPE(QQmlListProperty<TYPE>)

#define QML_DECLARE_TYPE_HASMETATYPE(TYPE) \
    Q_DECLARE_METATYPE(QQmlListProperty<TYPE>)

#define QML_DECLARE_INTERFACE(INTERFACE) \
    QML_DECLARE_TYPE(INTERFACE)

#define QML_DECLARE_INTERFACE_HASMETATYPE(INTERFACE) \
    QML_DECLARE_TYPE_HASMETATYPE(INTERFACE)

enum { /* TYPEINFO flags */
    QML_HAS_ATTACHED_PROPERTIES = 0x01
};

#define QML_DECLARE_TYPEINFO(TYPE, FLAGS) \
QT_BEGIN_NAMESPACE \
template <> \
class QQmlTypeInfo<TYPE > \
{ \
public: \
    enum { \
        hasAttachedProperties = (((FLAGS) & QML_HAS_ATTACHED_PROPERTIES) == QML_HAS_ATTACHED_PROPERTIES) \
    }; \
}; \
QT_END_NAMESPACE

QT_BEGIN_NAMESPACE

void Q_QML_EXPORT qmlClearTypeRegistrations();

template<class T>
QQmlCustomParser *qmlCreateCustomParser();

template<typename T>
int qmlRegisterAnonymousType(const char *uri, int versionMajor)
{
    QQmlPrivate::RegisterType type = {
        QQmlPrivate::RegisterType::CurrentVersion,
        QQmlPrivate::QmlMetaType<T>::self(),
        QQmlPrivate::QmlMetaType<T>::list(),
        0,
        nullptr, nullptr,
        QString(),
        QQmlPrivate::ValueType<T, void>::create,

        uri, QTypeRevision::fromVersion(versionMajor, 0), nullptr,
        QQmlPrivate::StaticMetaObject<T>::staticMetaObject(),

        QQmlPrivate::attachedPropertiesFunc<T>(),
        QQmlPrivate::attachedPropertiesMetaObject<T>(),

        QQmlPrivate::StaticCastSelector<T,QQmlParserStatus>::cast(),
        QQmlPrivate::StaticCastSelector<T,QQmlPropertyValueSource>::cast(),
        QQmlPrivate::StaticCastSelector<T,QQmlPropertyValueInterceptor>::cast(),

        nullptr, nullptr,

        nullptr,
        QTypeRevision::zero(),
        QQmlPrivate::StaticCastSelector<T,QQmlFinalizerHook>::cast(),
        QQmlPrivate::ValueTypeCreationMethod::None,
    };

    return QQmlPrivate::qmlregister(QQmlPrivate::TypeRegistration, &type);
}

//! \internal
template<typename T, int metaObjectRevisionMinor>
int qmlRegisterAnonymousType(const char *uri, int versionMajor)
{
    QQmlPrivate::RegisterType type = {
        QQmlPrivate::RegisterType::CurrentVersion,
        QQmlPrivate::QmlMetaType<T>::self(),
        QQmlPrivate::QmlMetaType<T>::list(),
        0,
        nullptr,
        nullptr,
        QString(),
        QQmlPrivate::ValueType<T, void>::create,

        uri,
        QTypeRevision::fromVersion(versionMajor, 0),
        nullptr,
        QQmlPrivate::StaticMetaObject<T>::staticMetaObject(),

        QQmlPrivate::attachedPropertiesFunc<T>(),
        QQmlPrivate::attachedPropertiesMetaObject<T>(),

        QQmlPrivate::StaticCastSelector<T, QQmlParserStatus>::cast(),
        QQmlPrivate::StaticCastSelector<T, QQmlPropertyValueSource>::cast(),
        QQmlPrivate::StaticCastSelector<T, QQmlPropertyValueInterceptor>::cast(),

        nullptr,
        nullptr,

        nullptr,
        QTypeRevision::fromMinorVersion(metaObjectRevisionMinor),
        QQmlPrivate::StaticCastSelector<T, QQmlFinalizerHook>::cast(),
        QQmlPrivate::ValueTypeCreationMethod::None,
    };

    return QQmlPrivate::qmlregister(QQmlPrivate::TypeRegistration, &type);
}

//! \internal
template<typename T>
void qmlRegisterAnonymousTypesAndRevisions(const char *uri, int versionMajor)
{
    // Anonymous types are not creatable, no need to warn about missing acceptable constructors.
    QQmlPrivate::qmlRegisterTypeAndRevisions<T, void>(
            uri, versionMajor, QQmlPrivate::StaticMetaObject<T>::staticMetaObject(), nullptr,
            nullptr, true);
}

class QQmlTypeNotAvailable : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(TypeNotAvailable)
    QML_ADDED_IN_VERSION(2, 15)
    QML_UNCREATABLE("Type not available.")
};

int Q_QML_EXPORT qmlRegisterTypeNotAvailable(const char *uri, int versionMajor, int versionMinor,
                                             const char *qmlName, const QString &message);

template<typename T>
int qmlRegisterUncreatableType(const char *uri, int versionMajor, int versionMinor, const char *qmlName, const QString& reason)
{
    QQmlPrivate::RegisterType type = {
        QQmlPrivate::RegisterType::CurrentVersion,
        QQmlPrivate::QmlMetaType<T>::self(),
        QQmlPrivate::QmlMetaType<T>::list(),
        0,
        nullptr,
        nullptr,
        reason,
        QQmlPrivate::ValueType<T, void>::create,

        uri, QTypeRevision::fromVersion(versionMajor, versionMinor), qmlName,
        QQmlPrivate::StaticMetaObject<T>::staticMetaObject(),

        QQmlPrivate::attachedPropertiesFunc<T>(),
        QQmlPrivate::attachedPropertiesMetaObject<T>(),

        QQmlPrivate::StaticCastSelector<T,QQmlParserStatus>::cast(),
        QQmlPrivate::StaticCastSelector<T,QQmlPropertyValueSource>::cast(),
        QQmlPrivate::StaticCastSelector<T,QQmlPropertyValueInterceptor>::cast(),

        nullptr, nullptr,

        nullptr,
        QTypeRevision::zero(),
        QQmlPrivate::StaticCastSelector<T,QQmlFinalizerHook>::cast(),
        QQmlPrivate::ValueTypeCreationMethod::None,
    };

    return QQmlPrivate::qmlregister(QQmlPrivate::TypeRegistration, &type);
}

template<typename T, int metaObjectRevision>
int qmlRegisterUncreatableType(const char *uri, int versionMajor, int versionMinor, const char *qmlName, const QString& reason)
{
    QQmlPrivate::RegisterType type = {
        QQmlPrivate::RegisterType::CurrentVersion,
        QQmlPrivate::QmlMetaType<T>::self(),
        QQmlPrivate::QmlMetaType<T>::list(),
        0,
        nullptr,
        nullptr,
        reason,
        QQmlPrivate::ValueType<T, void>::create,

        uri, QTypeRevision::fromVersion(versionMajor, versionMinor), qmlName,
        QQmlPrivate::StaticMetaObject<T>::staticMetaObject(),

        QQmlPrivate::attachedPropertiesFunc<T>(),
        QQmlPrivate::attachedPropertiesMetaObject<T>(),

        QQmlPrivate::StaticCastSelector<T,QQmlParserStatus>::cast(),
        QQmlPrivate::StaticCastSelector<T,QQmlPropertyValueSource>::cast(),
        QQmlPrivate::StaticCastSelector<T,QQmlPropertyValueInterceptor>::cast(),

        nullptr, nullptr,

        nullptr,
        QTypeRevision::fromMinorVersion(metaObjectRevision),
        QQmlPrivate::StaticCastSelector<T,QQmlFinalizerHook>::cast(),
        QQmlPrivate::ValueTypeCreationMethod::None,
    };

    return QQmlPrivate::qmlregister(QQmlPrivate::TypeRegistration, &type);
}

template<typename T, typename E>
int qmlRegisterExtendedUncreatableType(const char *uri, int versionMajor, int versionMinor, const char *qmlName, const QString& reason)
{
    QQmlAttachedPropertiesFunc attached = QQmlPrivate::attachedPropertiesFunc<E>();
    const QMetaObject * attachedMetaObject = QQmlPrivate::attachedPropertiesMetaObject<E>();
    if (!attached) {
        attached = QQmlPrivate::attachedPropertiesFunc<T>();
        attachedMetaObject = QQmlPrivate::attachedPropertiesMetaObject<T>();
    }

    QQmlPrivate::RegisterType type = {
        QQmlPrivate::RegisterType::CurrentVersion,
        QQmlPrivate::QmlMetaType<T>::self(),
        QQmlPrivate::QmlMetaType<T>::list(),
        0,
        nullptr,
        nullptr,
        reason,
        QQmlPrivate::ValueType<T, E>::create,

        uri, QTypeRevision::fromVersion(versionMajor, versionMinor), qmlName,
        QQmlPrivate::StaticMetaObject<T>::staticMetaObject(),

        attached,
        attachedMetaObject,

        QQmlPrivate::StaticCastSelector<T,QQmlParserStatus>::cast(),
        QQmlPrivate::StaticCastSelector<T,QQmlPropertyValueSource>::cast(),
        QQmlPrivate::StaticCastSelector<T,QQmlPropertyValueInterceptor>::cast(),

        QQmlPrivate::ExtendedType<E>::createParent, QQmlPrivate::ExtendedType<E>::staticMetaObject(),

        nullptr,
        QTypeRevision::zero(),
        QQmlPrivate::StaticCastSelector<T,QQmlFinalizerHook>::cast(),
        QQmlPrivate::ValueTypeCreationMethod::None,
    };

    return QQmlPrivate::qmlregister(QQmlPrivate::TypeRegistration, &type);
}

template<typename T, typename E, int metaObjectRevision>
int qmlRegisterExtendedUncreatableType(const char *uri, int versionMajor, int versionMinor, const char *qmlName, const QString& reason)
{
    QQmlAttachedPropertiesFunc attached = QQmlPrivate::attachedPropertiesFunc<E>();
    const QMetaObject * attachedMetaObject = QQmlPrivate::attachedPropertiesMetaObject<E>();
    if (!attached) {
        attached = QQmlPrivate::attachedPropertiesFunc<T>();
        attachedMetaObject = QQmlPrivate::attachedPropertiesMetaObject<T>();
    }

    QQmlPrivate::RegisterType type = {
        QQmlPrivate::RegisterType::CurrentVersion,
        QQmlPrivate::QmlMetaType<T>::self(),
        QQmlPrivate::QmlMetaType<T>::list(),
        0,
        nullptr,
        nullptr,
        reason,
        QQmlPrivate::ValueType<T, E>::create,

        uri, QTypeRevision::fromVersion(versionMajor, versionMinor), qmlName,
        QQmlPrivate::StaticMetaObject<T>::staticMetaObject(),

        attached,
        attachedMetaObject,

        QQmlPrivate::StaticCastSelector<T,QQmlParserStatus>::cast(),
        QQmlPrivate::StaticCastSelector<T,QQmlPropertyValueSource>::cast(),
        QQmlPrivate::StaticCastSelector<T,QQmlPropertyValueInterceptor>::cast(),

        QQmlPrivate::ExtendedType<E>::createParent, QQmlPrivate::ExtendedType<E>::staticMetaObject(),

        nullptr,
        QTypeRevision::fromMinorVersion(metaObjectRevision),
        QQmlPrivate::StaticCastSelector<T,QQmlFinalizerHook>::cast(),
        QQmlPrivate::ValueTypeCreationMethod::None,
    };

    return QQmlPrivate::qmlregister(QQmlPrivate::TypeRegistration, &type);
}

Q_QML_EXPORT int qmlRegisterUncreatableMetaObject(const QMetaObject &staticMetaObject, const char *uri, int versionMajor, int versionMinor, const char *qmlName, const QString& reason);

template<typename T>
int qmlRegisterType(const char *uri, int versionMajor, int versionMinor, const char *qmlName)
{
    static_assert(!std::is_abstract_v<T>,
        "It is not possible to register an abstract type with qmlRegisterType. "
        "Maybe you wanted qmlRegisterUncreatableType or qmlRegisterInterface?");

    QQmlPrivate::RegisterType type = {
        QQmlPrivate::RegisterType::CurrentVersion,
        QQmlPrivate::QmlMetaType<T>::self(),
        QQmlPrivate::QmlMetaType<T>::list(),
        sizeof(T), QQmlPrivate::Constructors<T>::createInto, nullptr,
        QString(),
        QQmlPrivate::ValueType<T, void>::create,

        uri, QTypeRevision::fromVersion(versionMajor, versionMinor), qmlName,
        QQmlPrivate::StaticMetaObject<T>::staticMetaObject(),

        QQmlPrivate::attachedPropertiesFunc<T>(),
        QQmlPrivate::attachedPropertiesMetaObject<T>(),

        QQmlPrivate::StaticCastSelector<T,QQmlParserStatus>::cast(),
        QQmlPrivate::StaticCastSelector<T,QQmlPropertyValueSource>::cast(),
        QQmlPrivate::StaticCastSelector<T,QQmlPropertyValueInterceptor>::cast(),

        nullptr, nullptr,

        nullptr,
        QTypeRevision::zero(),
        QQmlPrivate::StaticCastSelector<T,QQmlFinalizerHook>::cast(),
        QQmlPrivate::ValueTypeCreationMethod::None,
    };

    return QQmlPrivate::qmlregister(QQmlPrivate::TypeRegistration, &type);
}

template<typename T, int metaObjectRevision>
int qmlRegisterType(const char *uri, int versionMajor, int versionMinor, const char *qmlName)
{
    static_assert(!std::is_abstract_v<T>,
        "It is not possible to register an abstract type with qmlRegisterType. "
        "Maybe you wanted qmlRegisterUncreatableType or qmlRegisterInterface?");

    QQmlPrivate::RegisterType type = {
        QQmlPrivate::RegisterType::CurrentVersion,
        QQmlPrivate::QmlMetaType<T>::self(),
        QQmlPrivate::QmlMetaType<T>::list(),
        sizeof(T), QQmlPrivate::Constructors<T>::createInto, nullptr,
        QString(),
        QQmlPrivate::ValueType<T, void>::create,

        uri, QTypeRevision::fromVersion(versionMajor, versionMinor), qmlName,
        QQmlPrivate::StaticMetaObject<T>::staticMetaObject(),

        QQmlPrivate::attachedPropertiesFunc<T>(),
        QQmlPrivate::attachedPropertiesMetaObject<T>(),

        QQmlPrivate::StaticCastSelector<T,QQmlParserStatus>::cast(),
        QQmlPrivate::StaticCastSelector<T,QQmlPropertyValueSource>::cast(),
        QQmlPrivate::StaticCastSelector<T,QQmlPropertyValueInterceptor>::cast(),

        nullptr, nullptr,

        nullptr,
        QTypeRevision::fromMinorVersion(metaObjectRevision),
        QQmlPrivate::StaticCastSelector<T,QQmlFinalizerHook>::cast(),
        QQmlPrivate::ValueTypeCreationMethod::None,
    };

    return QQmlPrivate::qmlregister(QQmlPrivate::TypeRegistration, &type);
}

template<typename T, int metaObjectRevision>
int qmlRegisterRevision(const char *uri, int versionMajor, int versionMinor)
{
    QQmlPrivate::RegisterType type = {
        QQmlPrivate::RegisterType::CurrentVersion,
        QQmlPrivate::QmlMetaType<T>::self(),
        QQmlPrivate::QmlMetaType<T>::list(),
        sizeof(T), QQmlPrivate::Constructors<T>::createInto, nullptr,
        QString(),
        QQmlPrivate::ValueType<T, void>::create,

        uri, QTypeRevision::fromVersion(versionMajor, versionMinor), nullptr,
        QQmlPrivate::StaticMetaObject<T>::staticMetaObject(),

        QQmlPrivate::attachedPropertiesFunc<T>(),
        QQmlPrivate::attachedPropertiesMetaObject<T>(),

        QQmlPrivate::StaticCastSelector<T,QQmlParserStatus>::cast(),
        QQmlPrivate::StaticCastSelector<T,QQmlPropertyValueSource>::cast(),
        QQmlPrivate::StaticCastSelector<T,QQmlPropertyValueInterceptor>::cast(),

        nullptr, nullptr,

        nullptr,
        QTypeRevision::fromMinorVersion(metaObjectRevision),
        QQmlPrivate::StaticCastSelector<T,QQmlFinalizerHook>::cast(),
        QQmlPrivate::ValueTypeCreationMethod::None,
    };

    return QQmlPrivate::qmlregister(QQmlPrivate::TypeRegistration, &type);
}

template<typename T, typename E>
int qmlRegisterExtendedType(const char *uri, int versionMajor)
{
    static_assert(!std::is_abstract_v<T>,
        "It is not possible to register an extension to an abstract type with qmlRegisterExtendedType.");

    static_assert(!std::is_abstract_v<E>,
        "It is not possible to register an abstract type with qmlRegisterExtendedType. "
        "Maybe you wanted qmlRegisterExtendedUncreatableType?");

    QQmlPrivate::RegisterType type = {
        QQmlPrivate::RegisterType::CurrentVersion,
        QQmlPrivate::QmlMetaType<T>::self(),
        QQmlPrivate::QmlMetaType<T>::list(),
        0,
        nullptr,
        nullptr,
        QString(),
        QQmlPrivate::ValueType<T, E>::create,

        uri, QTypeRevision::fromVersion(versionMajor, 0), nullptr,
        QQmlPrivate::StaticMetaObject<T>::staticMetaObject(),

        QQmlPrivate::attachedPropertiesFunc<T>(),
        QQmlPrivate::attachedPropertiesMetaObject<T>(),

        QQmlPrivate::StaticCastSelector<T,QQmlParserStatus>::cast(),
        QQmlPrivate::StaticCastSelector<T,QQmlPropertyValueSource>::cast(),
        QQmlPrivate::StaticCastSelector<T,QQmlPropertyValueInterceptor>::cast(),

        QQmlPrivate::ExtendedType<E>::createParent, QQmlPrivate::ExtendedType<E>::staticMetaObject(),

        nullptr,
        QTypeRevision::zero(),
        QQmlPrivate::StaticCastSelector<T,QQmlFinalizerHook>::cast(),
        QQmlPrivate::ValueTypeCreationMethod::None,
    };

    return QQmlPrivate::qmlregister(QQmlPrivate::TypeRegistration, &type);
}

template<typename T, typename E>
int qmlRegisterExtendedType(const char *uri, int versionMajor, int versionMinor,
                            const char *qmlName)
{
    static_assert(!std::is_abstract_v<T>,
        "It is not possible to register an extension to an abstract type with qmlRegisterExtendedType.");

    static_assert(!std::is_abstract_v<E>,
        "It is not possible to register an abstract type with qmlRegisterExtendedType. "
        "Maybe you wanted qmlRegisterExtendedUncreatableType?");

    QQmlAttachedPropertiesFunc attached = QQmlPrivate::attachedPropertiesFunc<E>();
    const QMetaObject * attachedMetaObject = QQmlPrivate::attachedPropertiesMetaObject<E>();
    if (!attached) {
        attached = QQmlPrivate::attachedPropertiesFunc<T>();
        attachedMetaObject = QQmlPrivate::attachedPropertiesMetaObject<T>();
    }

    QQmlPrivate::RegisterType type = {
        QQmlPrivate::RegisterType::CurrentVersion,
        QQmlPrivate::QmlMetaType<T>::self(),
        QQmlPrivate::QmlMetaType<T>::list(),
        sizeof(T), QQmlPrivate::Constructors<T>::createInto, nullptr,
        QString(),
        QQmlPrivate::ValueType<T, E>::create,

        uri, QTypeRevision::fromVersion(versionMajor, versionMinor), qmlName,
        QQmlPrivate::StaticMetaObject<T>::staticMetaObject(),

        attached,
        attachedMetaObject,

        QQmlPrivate::StaticCastSelector<T,QQmlParserStatus>::cast(),
        QQmlPrivate::StaticCastSelector<T,QQmlPropertyValueSource>::cast(),
        QQmlPrivate::StaticCastSelector<T,QQmlPropertyValueInterceptor>::cast(),

        QQmlPrivate::ExtendedType<E>::createParent, QQmlPrivate::ExtendedType<E>::staticMetaObject(),

        nullptr,
        QTypeRevision::zero(),
        QQmlPrivate::StaticCastSelector<T,QQmlFinalizerHook>::cast(),
        QQmlPrivate::ValueTypeCreationMethod::None,
    };

    return QQmlPrivate::qmlregister(QQmlPrivate::TypeRegistration, &type);
}

template<typename T>
int qmlRegisterInterface(const char *uri, int versionMajor)
{
    QQmlPrivate::RegisterInterface qmlInterface = {
        0,
        // An interface is not a QObject itself but is typically casted to one.
        // Therefore, we still want the pointer.
        QMetaType::fromType<T *>(),
        QMetaType::fromType<QQmlListProperty<T> >(),
        qobject_interface_iid<T *>(),

        uri,
        QTypeRevision::fromVersion(versionMajor, 0)
    };

    return QQmlPrivate::qmlregister(QQmlPrivate::InterfaceRegistration, &qmlInterface);
}

template<typename T>
int qmlRegisterCustomType(const char *uri, int versionMajor, int versionMinor,
                          const char *qmlName, QQmlCustomParser *parser)
{
    static_assert(!std::is_abstract_v<T>,
        "It is not possible to register an abstract type with qmlRegisterCustomType. "
        "Maybe you wanted qmlRegisterUncreatableType or qmlRegisterInterface?");

    QQmlPrivate::RegisterType type = {
        QQmlPrivate::RegisterType::CurrentVersion,
        QQmlPrivate::QmlMetaType<T>::self(),
        QQmlPrivate::QmlMetaType<T>::list(),
        sizeof(T), QQmlPrivate::Constructors<T>::createInto, nullptr,
        QString(),
        QQmlPrivate::ValueType<T, void>::create,

        uri, QTypeRevision::fromVersion(versionMajor, versionMinor), qmlName,
        QQmlPrivate::StaticMetaObject<T>::staticMetaObject(),

        QQmlPrivate::attachedPropertiesFunc<T>(),
        QQmlPrivate::attachedPropertiesMetaObject<T>(),

        QQmlPrivate::StaticCastSelector<T,QQmlParserStatus>::cast(),
        QQmlPrivate::StaticCastSelector<T,QQmlPropertyValueSource>::cast(),
        QQmlPrivate::StaticCastSelector<T,QQmlPropertyValueInterceptor>::cast(),

        nullptr, nullptr,

        parser,
        QTypeRevision::zero(),
        QQmlPrivate::StaticCastSelector<T,QQmlFinalizerHook>::cast(),
        QQmlPrivate::ValueTypeCreationMethod::None,
    };

    return QQmlPrivate::qmlregister(QQmlPrivate::TypeRegistration, &type);
}

template<typename T, int metaObjectRevision>
int qmlRegisterCustomType(const char *uri, int versionMajor, int versionMinor,
                          const char *qmlName, QQmlCustomParser *parser)
{
    static_assert(!std::is_abstract_v<T>,
        "It is not possible to register an abstract type with qmlRegisterCustomType. "
        "Maybe you wanted qmlRegisterUncreatableType or qmlRegisterInterface?");

    QQmlPrivate::RegisterType type = {
        QQmlPrivate::RegisterType::CurrentVersion,
        QQmlPrivate::QmlMetaType<T>::self(),
        QQmlPrivate::QmlMetaType<T>::list(),
        sizeof(T), QQmlPrivate::Constructors<T>::createInto, nullptr,
        QString(),
        QQmlPrivate::ValueType<T, void>::create,

        uri, QTypeRevision::fromVersion(versionMajor, versionMinor), qmlName,
        QQmlPrivate::StaticMetaObject<T>::staticMetaObject(),

        QQmlPrivate::attachedPropertiesFunc<T>(),
        QQmlPrivate::attachedPropertiesMetaObject<T>(),

        QQmlPrivate::StaticCastSelector<T,QQmlParserStatus>::cast(),
        QQmlPrivate::StaticCastSelector<T,QQmlPropertyValueSource>::cast(),
        QQmlPrivate::StaticCastSelector<T,QQmlPropertyValueInterceptor>::cast(),

        nullptr, nullptr,

        parser,
        QTypeRevision::fromMinorVersion(metaObjectRevision),
        QQmlPrivate::StaticCastSelector<T,QQmlFinalizerHook>::cast(),
        QQmlPrivate::ValueTypeCreationMethod::None,
    };

    return QQmlPrivate::qmlregister(QQmlPrivate::TypeRegistration, &type);
}

template<typename T, typename E>
int qmlRegisterCustomExtendedType(const char *uri, int versionMajor, int versionMinor,
                          const char *qmlName, QQmlCustomParser *parser)
{
    static_assert(!std::is_abstract_v<T>,
        "It is not possible to register an extension to an abstract type with qmlRegisterCustomExtendedType.");

    static_assert(!std::is_abstract_v<E>,
        "It is not possible to register an abstract type with qmlRegisterCustomExtendedType.");

    QQmlAttachedPropertiesFunc attached = QQmlPrivate::attachedPropertiesFunc<E>();
    const QMetaObject * attachedMetaObject = QQmlPrivate::attachedPropertiesMetaObject<E>();
    if (!attached) {
        attached = QQmlPrivate::attachedPropertiesFunc<T>();
        attachedMetaObject = QQmlPrivate::attachedPropertiesMetaObject<T>();
    }

    QQmlPrivate::RegisterType type = {
        QQmlPrivate::RegisterType::CurrentVersion,
        QQmlPrivate::QmlMetaType<T>::self(),
        QQmlPrivate::QmlMetaType<T>::list(),
        sizeof(T), QQmlPrivate::Constructors<T>::createInto, nullptr,
        QString(),
        QQmlPrivate::ValueType<T, E>::create,

        uri, QTypeRevision::fromVersion(versionMajor, versionMinor), qmlName,
        QQmlPrivate::StaticMetaObject<T>::staticMetaObject(),

        attached,
        attachedMetaObject,

        QQmlPrivate::StaticCastSelector<T,QQmlParserStatus>::cast(),
        QQmlPrivate::StaticCastSelector<T,QQmlPropertyValueSource>::cast(),
        QQmlPrivate::StaticCastSelector<T,QQmlPropertyValueInterceptor>::cast(),

        QQmlPrivate::ExtendedType<E>::createParent, QQmlPrivate::ExtendedType<E>::staticMetaObject(),

        parser,
        QTypeRevision::zero(),
        QQmlPrivate::StaticCastSelector<T,QQmlFinalizerHook>::cast(),
        QQmlPrivate::ValueTypeCreationMethod::None,
    };

    return QQmlPrivate::qmlregister(QQmlPrivate::TypeRegistration, &type);
}

class QQmlContext;
class QQmlEngine;
class QJSValue;
class QJSEngine;

Q_QML_EXPORT void qmlExecuteDeferred(QObject *);
Q_QML_EXPORT QQmlContext *qmlContext(const QObject *);
Q_QML_EXPORT QQmlEngine *qmlEngine(const QObject *);
Q_QML_EXPORT QQmlAttachedPropertiesFunc qmlAttachedPropertiesFunction(QObject *,
                                                                      const QMetaObject *);
Q_QML_EXPORT QObject *qmlAttachedPropertiesObject(QObject *, QQmlAttachedPropertiesFunc func,
                                                  bool create = true);
Q_QML_EXPORT QObject *qmlExtendedObject(QObject *);

//The C++ version of protected namespaces in qmldir
Q_QML_EXPORT bool qmlProtectModule(const char* uri, int majVersion);
Q_QML_EXPORT void qmlRegisterModule(const char *uri, int versionMajor, int versionMinor);

enum QQmlModuleImportSpecialVersions: int {
    QQmlModuleImportModuleAny = -1,
    QQmlModuleImportLatest = -1,
    QQmlModuleImportAuto = -2
};

Q_QML_EXPORT void qmlRegisterModuleImport(const char *uri, int moduleMajor,
                                          const char *import,
                                          int importMajor = QQmlModuleImportLatest,
                                          int importMinor = QQmlModuleImportLatest);
Q_QML_EXPORT void qmlUnregisterModuleImport(const char *uri, int moduleMajor,
                                            const char *import,
                                            int importMajor = QQmlModuleImportLatest,
                                            int importMinor = QQmlModuleImportLatest);

template<typename T>
QObject *qmlAttachedPropertiesObject(const QObject *obj, bool create = true)
{
    // We don't need a concrete object to resolve the function. As T is a C++ type, it and all its
    // super types should be registered as CppType (or not at all). We only need the object and its
    // QML engine to resolve composite types. Therefore, the function is actually a static property
    // of the C++ type system and we can cache it here for improved performance on further lookups.
    if (const auto func = QQmlPrivate::attachedPropertiesFunc<T>())
        return qmlAttachedPropertiesObject(const_cast<QObject *>(obj), func, create);

    // Usually the above func should not be nullptr. However, to be safe, keep this fallback
    // via the metaobject.
    static const auto func = qmlAttachedPropertiesFunction(nullptr, &T::staticMetaObject);
    return qmlAttachedPropertiesObject(const_cast<QObject *>(obj), func, create);
}

#ifdef Q_QDOC
int qmlRegisterSingletonType(
        const char *uri, int versionMajor, int versionMinor, const char *typeName,
        std::function<QJSValue(QQmlEngine *, QJSEngine *)> callback)
#else
template<typename F, typename std::enable_if<std::is_convertible<F, std::function<QJSValue(QQmlEngine *, QJSEngine *)>>::value, void>::type* = nullptr>
int qmlRegisterSingletonType(
        const char *uri, int versionMajor, int versionMinor, const char *typeName, F &&callback)
#endif
{
    QQmlPrivate::RegisterSingletonType api = {
        0,
        uri,
        QTypeRevision::fromVersion(versionMajor, versionMinor),
        typeName,
        std::forward<F>(callback),
        nullptr,
        nullptr,
        QMetaType(),
        nullptr, nullptr,
        QTypeRevision::zero()
    };

    return QQmlPrivate::qmlregister(QQmlPrivate::SingletonRegistration, &api);
}

#ifdef Q_QDOC
template <typename T>
int qmlRegisterSingletonType(
        const char *uri, int versionMajor, int versionMinor, const char *typeName,
        std::function<QObject *(QQmlEngine *, QJSEngine *)> callback)
#else
template<typename T, typename F, typename std::enable_if<std::is_convertible<F, std::function<QObject *(QQmlEngine *, QJSEngine *)>>::value, void>::type* = nullptr>
int qmlRegisterSingletonType(
        const char *uri, int versionMajor, int versionMinor, const char *typeName, F &&callback)
#endif
{
    QQmlPrivate::RegisterSingletonType api = {
        0,
        uri,
        QTypeRevision::fromVersion(versionMajor, versionMinor),
        typeName,
        nullptr,
        std::forward<F>(callback),
        QQmlPrivate::StaticMetaObject<T>::staticMetaObject(),
        QQmlPrivate::QmlMetaType<T>::self(),
        nullptr, nullptr,
        QTypeRevision::zero()
    };

    return QQmlPrivate::qmlregister(QQmlPrivate::SingletonRegistration, &api);
}

#ifdef Q_QDOC
int qmlRegisterSingletonInstance(const char *uri, int versionMajor, int versionMinor, const char *typeName, QObject *cppObject)
#else
template<typename T>
inline auto qmlRegisterSingletonInstance(const char *uri, int versionMajor, int versionMinor,
                                         const char *typeName, T *cppObject) -> typename std::enable_if<std::is_base_of<QObject, T>::value, int>::type
#endif
{
    QQmlPrivate::SingletonInstanceFunctor registrationFunctor;
    registrationFunctor.m_object = cppObject;
    return qmlRegisterSingletonType<T>(uri, versionMajor, versionMinor, typeName, registrationFunctor);
}

inline int qmlRegisterSingletonType(const QUrl &url, const char *uri, int versionMajor, int versionMinor, const char *qmlName)
{
    if (url.isRelative()) {
        // User input check must go here, because QQmlPrivate::qmlregister is also used internally for composite types
        qWarning("qmlRegisterSingletonType requires absolute URLs.");
        return 0;
    }

    QQmlPrivate::RegisterCompositeSingletonType type = {
        0,
        url,
        uri,
        QTypeRevision::fromVersion(versionMajor, versionMinor),
        qmlName
    };

    return QQmlPrivate::qmlregister(QQmlPrivate::CompositeSingletonRegistration, &type);
}

inline int qmlRegisterType(const QUrl &url, const char *uri, int versionMajor, int versionMinor, const char *qmlName)
{
    if (url.isRelative()) {
        // User input check must go here, because QQmlPrivate::qmlregister is also used internally for composite types
        qWarning("qmlRegisterType requires absolute URLs.");
        return 0;
    }

    QQmlPrivate::RegisterCompositeType type = {
        0,
        url,
        uri,
        QTypeRevision::fromVersion(versionMajor, versionMinor),
        qmlName
    };

    return QQmlPrivate::qmlregister(QQmlPrivate::CompositeRegistration, &type);
}

template<typename Container>
inline int qmlRegisterAnonymousSequentialContainer(const char *uri, int versionMajor)
{
    static_assert(!std::is_abstract_v<Container>,
        "It is not possible to register an abstract container with qmlRegisterAnonymousSequentialContainer.");

    QQmlPrivate::RegisterSequentialContainer type = {
        0,
        uri,
        QTypeRevision::fromMajorVersion(versionMajor),
        nullptr,
        QMetaType::fromType<Container>(),
        QMetaSequence::fromContainer<Container>(),
        QTypeRevision::zero()
    };

    return QQmlPrivate::qmlregister(QQmlPrivate::SequentialContainerRegistration, &type);
}

template<class T, class Resolved, class Extended, bool Singleton, bool Interface, bool Sequence, bool Uncreatable>
struct QmlTypeAndRevisionsRegistration;

template<class T, class Resolved, class Extended>
struct QmlTypeAndRevisionsRegistration<T, Resolved, Extended, false, false, false, false> {
    static void registerTypeAndRevisions(const char *uri, int versionMajor, QList<int> *qmlTypeIds,
                                         const QMetaObject *extension)
    {
#if QT_DEPRECATED_SINCE(6, 4)
        // ### Qt7: Remove the warnings, and leave only the static asserts below.
        if constexpr (!QQmlPrivate::QmlMetaType<Resolved>::hasAcceptableCtors()) {
            QQmlPrivate::qmlRegistrationWarning(QQmlPrivate::UnconstructibleType,
                                                QMetaType::fromType<Resolved>());
        }

        if constexpr (!std::is_base_of_v<QObject, Resolved>
                && QQmlTypeInfo<T>::hasAttachedProperties) {
            QQmlPrivate::qmlRegistrationWarning(QQmlPrivate::NonQObjectWithAtached,
                                                QMetaType::fromType<Resolved>());
        }
#else
        static_assert(QQmlPrivate::QmlMetaType<Resolved>::hasAcceptableCtors(),
                      "This type is neither a default constructible QObject, nor a default- "
                      "and copy-constructible Q_GADGET, nor marked as uncreatable.\n"
                      "You should not use it as a QML type.");
        static_assert(std::is_base_of_v<QObject, Resolved>
                || !QQmlTypeInfo<Resolved>::hasAttachedProperties);
#endif
        QQmlPrivate::qmlRegisterTypeAndRevisions<Resolved, Extended>(
                    uri, versionMajor, QQmlPrivate::StaticMetaObject<T>::staticMetaObject(),
                    qmlTypeIds, extension);
    }
};

template<class T, class Resolved, class Extended>
struct QmlTypeAndRevisionsRegistration<T, Resolved, Extended, false, false, false, true> {
    static void registerTypeAndRevisions(const char *uri, int versionMajor, QList<int> *qmlTypeIds,
                                         const QMetaObject *extension)
    {
#if QT_DEPRECATED_SINCE(6, 4)
        // ### Qt7: Remove the warning, and leave only the static assert below.
        if constexpr (!std::is_base_of_v<QObject, Resolved>) {
            if constexpr (!QQmlPrivate::QmlMetaType<Resolved>::hasAcceptableCtors()) {
                QQmlPrivate::qmlRegistrationWarning(
                        QQmlPrivate::UnconstructibleType, QMetaType::fromType<Resolved>());
            }
            if constexpr (QQmlTypeInfo<Resolved>::hasAttachedProperties) {
                QQmlPrivate::qmlRegistrationWarning(
                        QQmlPrivate::NonQObjectWithAtached, QMetaType::fromType<Resolved>());
            }
        }
#elif QT_DEPRECATED_SINCE(6, 10)
        static_assert(std::is_base_of_v<QObject, Resolved>
                || !QQmlTypeInfo<Resolved>::hasAttachedProperties);

        // ### Qt7: Remove the warning, and leave only the static assert below.
        if constexpr (!std::is_base_of_v<QObject, Resolved>
                && !QQmlPrivate::QmlMetaType<Resolved>::hasAcceptableCtors()) {
            QQmlPrivate::qmlRegistrationWarning(
                    QQmlPrivate::UnconstructibleType, QMetaType::fromType<Resolved>());
        }
#else
        static_assert(std::is_base_of_v<QObject, Resolved>
                || (QQmlPrivate::QmlMetaType<Resolved>::hasAcceptableCtors()
                    && !QQmlTypeInfo<Resolved>::hasAttachedProperties));
#endif
        QQmlPrivate::qmlRegisterTypeAndRevisions<Resolved, Extended>(
                    uri, versionMajor, QQmlPrivate::StaticMetaObject<T>::staticMetaObject(),
                    qmlTypeIds, extension);
    }
};

template<class T, class Resolved>
struct QmlTypeAndRevisionsRegistration<T, Resolved, void, false, false, true, true> {
    static void registerTypeAndRevisions(const char *uri, int versionMajor, QList<int> *qmlTypeIds,
                                         const QMetaObject *)
    {
        // Sequences have to be anonymous for now, which implies uncreatable.
        QQmlPrivate::qmlRegisterSequenceAndRevisions<Resolved>(
                    uri, versionMajor, QQmlPrivate::StaticMetaObject<T>::staticMetaObject(),
                    qmlTypeIds);
    }
};

template<class T, class Resolved, class Extended>
struct QmlTypeAndRevisionsRegistration<T, Resolved, Extended, true, false, false, false> {
    static void registerTypeAndRevisions(const char *uri, int versionMajor, QList<int> *qmlTypeIds,
                                         const QMetaObject *extension)
    {
#if QT_DEPRECATED_SINCE(6, 4)
        // ### Qt7: Remove the warning, and leave only the static assert below.
        if constexpr (QQmlPrivate::singletonConstructionMode<Resolved, T>()
                == QQmlPrivate::SingletonConstructionMode::None) {
            QQmlPrivate::qmlRegistrationWarning(QQmlPrivate::UnconstructibleSingleton,
                                                QMetaType::fromType<Resolved>());
        }
#else
        static_assert(QQmlPrivate::singletonConstructionMode<Resolved, T>()
                        != QQmlPrivate::SingletonConstructionMode::None,
                      "A singleton needs either a default constructor or, when adding a default "
                      "constructor is infeasible, a public static "
                      "create(QQmlEngine *, QJSEngine *) method");
#endif

        QQmlPrivate::qmlRegisterSingletonAndRevisions<Resolved, Extended, T>(
                    uri, versionMajor, QQmlPrivate::StaticMetaObject<T>::staticMetaObject(),
                    qmlTypeIds, extension);
    }
};

template<class T, class Resolved, class Extended>
struct QmlTypeAndRevisionsRegistration<T, Resolved, Extended, true, false, false, true> {
    static void registerTypeAndRevisions(const char *uri, int versionMajor, QList<int> *qmlTypeIds,
                                         const QMetaObject *extension)
    {
        // An uncreatable singleton makes little sense? OK, you can still use the enums.
        QQmlPrivate::qmlRegisterSingletonAndRevisions<Resolved, Extended, T>(
                    uri, versionMajor, QQmlPrivate::StaticMetaObject<T>::staticMetaObject(),
                    qmlTypeIds, extension);
    }
};

template<class T, class Resolved>
struct QmlTypeAndRevisionsRegistration<T, Resolved, void, false, true, false, false> {
    static void registerTypeAndRevisions(const char *uri, int versionMajor, QList<int> *qmlTypeIds,
                                         const QMetaObject *)
    {
        const int id = qmlRegisterInterface<Resolved>(uri, versionMajor);
        if (qmlTypeIds)
            qmlTypeIds->append(id);
    }
};

template<typename... T>
void qmlRegisterTypesAndRevisions(const char *uri, int versionMajor, QList<int> *qmlTypeIds)
{
    (QmlTypeAndRevisionsRegistration<
            T, typename QQmlPrivate::QmlResolved<T>::Type,
            typename QQmlPrivate::QmlExtended<T>::Type,
            QQmlPrivate::QmlSingleton<T>::Value,
            QQmlPrivate::QmlInterface<T>::Value,
            QQmlPrivate::QmlSequence<T>::Value,
            QQmlPrivate::QmlUncreatable<T>::Value || QQmlPrivate::QmlAnonymous<T>::Value>
            ::registerTypeAndRevisions(uri, versionMajor, qmlTypeIds,
                                       QQmlPrivate::QmlExtendedNamespace<T>::metaObject()), ...);
}

inline void qmlRegisterNamespaceAndRevisions(const QMetaObject *metaObject,
                                             const char *uri, int versionMajor,
                                             QList<int> *qmlTypeIds,
                                             const QMetaObject *classInfoMetaObject,
                                             const QMetaObject *extensionMetaObject)
{
    QQmlPrivate::RegisterTypeAndRevisions type = {
        3,
        QMetaType(),
        QMetaType(),
        0,
        nullptr,
        nullptr,
        nullptr,

        uri,
        QTypeRevision::fromMajorVersion(versionMajor),

        metaObject,
        (classInfoMetaObject ? classInfoMetaObject : metaObject),

        nullptr,
        nullptr,

        -1,
        -1,
        -1,

        nullptr,
        extensionMetaObject,

        &qmlCreateCustomParser<void>,
        qmlTypeIds,
        -1,
        false,
        QMetaSequence()
    };

    qmlregister(QQmlPrivate::TypeAndRevisionsRegistration, &type);
}

inline void qmlRegisterNamespaceAndRevisions(const QMetaObject *metaObject,
                                             const char *uri, int versionMajor,
                                             QList<int> *qmlTypeIds = nullptr,
                                             const QMetaObject *classInfoMetaObject = nullptr)
{
    qmlRegisterNamespaceAndRevisions(metaObject, uri, versionMajor, qmlTypeIds,
                                     classInfoMetaObject, nullptr);
}

template<typename Enum>
void qmlRegisterEnum(const char *name)
{
    const QMetaType metaType = QMetaType::fromType<Enum>();

    // Calling id() generally makes the metatype usable with fromName().
    metaType.id();

    // If the enum was registered with the old Q_ENUMS or Q_FLAGS,
    // we need to manually set up the typedef.
    if constexpr (QtPrivate::IsQEnumHelper<Enum>::Value)
        Q_UNUSED(name)
    else
        QMetaType::registerNormalizedTypedef(name, metaType);
}

int Q_QML_EXPORT qmlTypeId(const char *uri, int versionMajor, int versionMinor, const char *qmlName);

QT_END_NAMESPACE

#endif // QQML_H
