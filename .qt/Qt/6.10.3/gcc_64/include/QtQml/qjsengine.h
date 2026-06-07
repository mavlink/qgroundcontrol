// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QJSENGINE_H
#define QJSENGINE_H

#include <QtCore/qmetatype.h>

#include <QtCore/qvariant.h>
#include <QtCore/qsharedpointer.h>
#include <QtCore/qobject.h>
#include <QtCore/qtimezone.h>
#include <QtQml/qjsvalue.h>
#include <QtQml/qjsmanagedvalue.h>
#include <QtQml/qqmldebug.h>

QT_BEGIN_NAMESPACE


template <typename T>
inline T qjsvalue_cast(const QJSValue &);

class QJSEnginePrivate;
class Q_QML_EXPORT QJSEngine
    : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString uiLanguage READ uiLanguage WRITE setUiLanguage NOTIFY uiLanguageChanged)
public:
    QJSEngine();
    explicit QJSEngine(QObject *parent);
    ~QJSEngine() override;

    QJSValue globalObject() const;

    QJSValue evaluate(const QString &program, const QString &fileName = QString(), int lineNumber = 1, QStringList *exceptionStackTrace = nullptr);

    QJSValue importModule(const QString &fileName);
    bool registerModule(const QString &moduleName, const QJSValue &value);

    QJSValue newObject();
    QJSValue newSymbol(const QString &name);
    QJSValue newArray(uint length = 0);

    QJSValue newQObject(QObject *object);

    QJSValue newQMetaObject(const QMetaObject* metaObject);

    template <typename T>
    QJSValue newQMetaObject()
    {
        return newQMetaObject(&T::staticMetaObject);
    }

    QJSValue newErrorObject(QJSValue::ErrorType errorType, const QString &message = QString());

    template <typename T>
    inline QJSValue toScriptValue(const T &value)
    {
        return create(QMetaType::fromType<T>(), &value);
    }

    template <typename T>
    inline QJSManagedValue toManagedValue(const T &value)
    {
        return createManaged(QMetaType::fromType<T>(), &value);
    }

    template <typename T>
    inline QJSPrimitiveValue toPrimitiveValue(const T &value)
    {
        // In the common case that the argument fits into QJSPrimitiveValue, use it.
        if constexpr (std::disjunction_v<
                std::is_same<T, int>,
                std::is_same<T, bool>,
                std::is_same<T, double>,
                std::is_same<T, QString>>) {
            return QJSPrimitiveValue(value);
        } else {
            return createPrimitive(QMetaType::fromType<T>(), &value);
        }
    }

    template <typename T>
    inline T fromScriptValue(const QJSValue &value)
    {
        return qjsvalue_cast<T>(value);
    }

    template <typename T>
    inline T fromManagedValue(const QJSManagedValue &value)
    {
        return qjsvalue_cast<T>(value);
    }

    template <typename T>
    inline T fromPrimitiveValue(const QJSPrimitiveValue &value)
    {
        if constexpr (std::is_same_v<T, int>)
            return value.toInteger();
        if constexpr (std::is_same_v<T, bool>)
            return value.toBoolean();
        if constexpr (std::is_same_v<T, double>)
            return value.toDouble();
        if constexpr (std::is_same_v<T, QString>)
            return value.toString();
        if constexpr (std::is_same_v<T, QVariant>)
            return value.toVariant();
        if constexpr (std::is_pointer_v<T>)
            return nullptr;
        return qjsvalue_cast<T>(value);
    }

    template <typename T>
    inline T fromVariant(const QVariant &value)
    {
        if constexpr (std::is_same_v<T, QVariant>)
            return value;

        const QMetaType sourceType = value.metaType();
        const QMetaType targetType = QMetaType::fromType<T>();
        if (sourceType == targetType)
            return *reinterpret_cast<const T *>(value.constData());

        if constexpr (std::is_same_v<T,std::remove_const_t<std::remove_pointer_t<T>> const *>) {
            using nonConstT = std::remove_const_t<std::remove_pointer_t<T>> *;
            const QMetaType nonConstTargetType = QMetaType::fromType<nonConstT>();
            if (value.metaType() == nonConstTargetType)
                return *reinterpret_cast<const nonConstT *>(value.constData());
        }

        if constexpr (std::is_same_v<T, QJSValue>)
            return toScriptValue(value);

        if constexpr (std::is_same_v<T, QJSManagedValue>)
            return toManagedValue(value);

        if constexpr (std::is_same_v<T, QJSPrimitiveValue>)
            return toPrimitiveValue(value);

        if constexpr (std::is_same_v<T, QString>) {
            if (sourceType.flags() & QMetaType::PointerToQObject) {
                return convertQObjectToString(
                            *reinterpret_cast<QObject *const *>(value.constData()));
            }
        }

        if constexpr (std::is_same_v<T, double>) {
            if (sourceType == QMetaType::fromType<int>())
                return double(*static_cast<const int *>(value.constData()));
        }

        if constexpr (std::is_same_v<T, int>) {
            if (sourceType == QMetaType::fromType<double>()) {
                return QJSNumberCoercion::toInteger(
                        *static_cast<const double *>(value.constData()));
            }
        }

        if constexpr (std::is_same_v<QObject, std::remove_const_t<std::remove_pointer_t<T>>>) {
            if (sourceType.flags() & QMetaType::PointerToQObject) {
                return *static_cast<QObject *const *>(value.constData());

                // We should not access source->metaObject() here since that may trigger some
                // rather involved code. convertVariant() can do this using property caches.
            }
        }

        if (sourceType == QMetaType::fromType<QJSValue>())
            return fromScriptValue<T>(*reinterpret_cast<const QJSValue *>(value.constData()));

        if (sourceType == QMetaType::fromType<QJSManagedValue>()) {
            return fromManagedValue<T>(
                        *reinterpret_cast<const QJSManagedValue *>(value.constData()));
        }

        if (sourceType == QMetaType::fromType<QJSPrimitiveValue>()) {
            return fromPrimitiveValue<T>(
                        *reinterpret_cast<const QJSPrimitiveValue *>(value.constData()));
        }

        return [&] {
            T t{};
            if (value.metaType() == QMetaType::fromType<QString>()) {
                if (convertString(value.toString(), targetType, &t))
                    return t;
            } else if (convertVariant(value, targetType, &t)) {
                return t;
            }

            QMetaType::convert(value.metaType(), value.constData(), targetType, &t);
            return t;
        }();
    }

    template<typename From, typename To>
    inline To coerceValue(const From &from)
    {
        if constexpr (std::is_base_of_v<To, From>)
            return from;

        if constexpr (std::is_same_v<To, QJSValue>)
            return toScriptValue(from);

        if constexpr (std::is_same_v<From, QJSValue>)
            return fromScriptValue<To>(from);

        if constexpr (std::is_same_v<To, QJSManagedValue>)
            return toManagedValue(from);

        if constexpr (std::is_same_v<From, QJSManagedValue>)
            return fromManagedValue<To>(from);

        if constexpr (std::is_same_v<To, QJSPrimitiveValue>)
            return toPrimitiveValue(from);

        if constexpr (std::is_same_v<From, QJSPrimitiveValue>)
            return fromPrimitiveValue<To>(from);

        if constexpr (std::is_same_v<From, QVariant>)
            return fromVariant<To>(from);

        if constexpr (std::is_same_v<To, QVariant>)
            return QVariant::fromValue(from);

        if constexpr (std::is_same_v<To, QString>) {
            if constexpr (std::is_base_of_v<QObject, std::remove_const_t<std::remove_pointer_t<From>>>)
                return convertQObjectToString(from);
        }

        if constexpr (std::is_same_v<From, QDateTime>) {
            if constexpr (std::is_same_v<To, QDate>)
                return convertDateTimeToDate(from.toLocalTime());
            if constexpr (std::is_same_v<To, QTime>)
                return from.toLocalTime().time();
            if constexpr (std::is_same_v<To, QString>)
                return convertDateTimeToString(from.toLocalTime());
            if constexpr (std::is_same_v<To, double>)
                return convertDateTimeToNumber(from.toLocalTime());
        }

        if constexpr (std::is_same_v<From, QDate>) {
            if constexpr (std::is_same_v<To, QDateTime>)
                return from.startOfDay(QTimeZone::UTC);
            if constexpr (std::is_same_v<To, QTime>) {
                // This is the current time zone offset, for better or worse
                return coerceValue<QDateTime, QTime>(coerceValue<QDate, QDateTime>(from));
            }
            if constexpr (std::is_same_v<To, QString>)
                return convertDateTimeToString(coerceValue<QDate, QDateTime>(from));
            if constexpr (std::is_same_v<To, double>)
                return convertDateTimeToNumber(coerceValue<QDate, QDateTime>(from));
        }

        if constexpr (std::is_same_v<From, QTime>) {
            if constexpr (std::is_same_v<To, QDate>) {
                // Yes. April Fools' 1971. See qv4dateobject.cpp.
                return from.isValid() ? QDate(1971, 4, 1) : QDate();
            }

            if constexpr (std::is_same_v<To, QDateTime>)
                return QDateTime(coerceValue<QTime, QDate>(from), from, QTimeZone::LocalTime);
            if constexpr (std::is_same_v<To, QString>)
                return convertDateTimeToString(coerceValue<QTime, QDateTime>(from));
            if constexpr (std::is_same_v<To, double>)
                return convertDateTimeToNumber(coerceValue<QTime, QDateTime>(from));
        }

        if constexpr (std::is_same_v<To, std::remove_const_t<std::remove_pointer_t<To>> const *>) {
            using nonConstTo = std::remove_const_t<std::remove_pointer_t<To>> *;
            if constexpr (std::is_same_v<From, nonConstTo>)
                return from;
        }

        if constexpr (std::is_same_v<To, double>) {
            if constexpr (std::is_same_v<From, int>)
                return double(from);
        }

        if constexpr (std::is_same_v<To, int>) {
            if constexpr (std::is_same_v<From, double>)
                return QJSNumberCoercion::toInteger(from);
        }

        return [&] {
            const QMetaType sourceType = QMetaType::fromType<From>();
            const QMetaType targetType = QMetaType::fromType<To>();
            To to{};
            if constexpr (std::is_same_v<From, QString>) {
                if (convertString(from, targetType, &to))
                    return to;
            } else if (convertMetaType(sourceType, &from, targetType, &to)) {
                return to;
            }

            QMetaType::convert(sourceType, &from, targetType, &to);
            return to;
        }();
    }

    void collectGarbage();

    enum ObjectOwnership { CppOwnership, JavaScriptOwnership };
    static void setObjectOwnership(QObject *, ObjectOwnership);
    static ObjectOwnership objectOwnership(QObject *);

    enum Extension {
        TranslationExtension = 0x1,
        ConsoleExtension = 0x2,
        GarbageCollectionExtension = 0x4,
        AllExtensions = 0xffffffff
    };
    Q_DECLARE_FLAGS(Extensions, Extension)

    void installExtensions(Extensions extensions, const QJSValue &object = QJSValue());

    void setInterrupted(bool interrupted);
    bool isInterrupted() const;

    QV4::ExecutionEngine *handle() const { return m_v4Engine; }

    void throwError(const QString &message);
    void throwError(QJSValue::ErrorType errorType, const QString &message = QString());
    void throwError(const QJSValue &error);
    bool hasError() const;
    QJSValue catchError();

    QString uiLanguage() const;
    void setUiLanguage(const QString &language);

Q_SIGNALS:
    void uiLanguageChanged();

private:
    QJSPrimitiveValue createPrimitive(QMetaType type, const void *ptr);
    QJSManagedValue createManaged(QMetaType type, const void *ptr);
    QJSValue create(QMetaType type, const void *ptr);
#if QT_QML_REMOVED_SINCE(6, 5)
    QJSValue create(int id, const void *ptr); // only there for BC reasons
#endif

    static bool convertPrimitive(const QJSPrimitiveValue &value, QMetaType type, void *ptr);
    static bool convertManaged(const QJSManagedValue &value, int type, void *ptr);
    static bool convertManaged(const QJSManagedValue &value, QMetaType type, void *ptr);
#if QT_QML_REMOVED_SINCE(6, 5)
    static bool convertV2(const QJSValue &value, int type, void *ptr); // only there for BC reasons
#endif
    static bool convertV2(const QJSValue &value, QMetaType metaType, void *ptr);
    static bool convertString(const QString &string, QMetaType metaType, void *ptr);

    bool convertVariant(const QVariant &value, QMetaType metaType, void *ptr);
    bool convertMetaType(QMetaType fromType, const void *from, QMetaType toType, void *to);

    QString convertQObjectToString(QObject *object);
    QString convertDateTimeToString(const QDateTime &dateTime);
    double convertDateTimeToNumber(const QDateTime &dateTime);
    static QDate convertDateTimeToDate(const QDateTime &dateTime);

    template<typename T>
    friend inline T qjsvalue_cast(const QJSValue &);

    template<typename T>
    friend inline T qjsvalue_cast(const QJSManagedValue &);

    template<typename T>
    friend inline T qjsvalue_cast(const QJSPrimitiveValue &);

protected:
    QJSEngine(QJSEnginePrivate &dd, QObject *parent = nullptr);

private:
    QV4::ExecutionEngine *m_v4Engine;
    Q_DISABLE_COPY(QJSEngine)
    Q_DECLARE_PRIVATE(QJSEngine)
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QJSEngine::Extensions)

template<typename T>
T qjsvalue_cast(const QJSValue &value)
{
    if (T t; QJSEngine::convertV2(value, QMetaType::fromType<T>(), &t))
        return t;
    return qvariant_cast<T>(value.toVariant());
}

template<typename T>
T qjsvalue_cast(const QJSManagedValue &value)
{
    if (T t; QJSEngine::convertManaged(value, QMetaType::fromType<T>(), &t))
        return t;

    return qvariant_cast<T>(value.toVariant());
}

template<typename T>
T qjsvalue_cast(const QJSPrimitiveValue &value)
{
    if (T t; QJSEngine::convertPrimitive(value, QMetaType::fromType<T>(), &t))
        return t;

    return qvariant_cast<T>(value.toVariant());
}

template <>
inline QVariant qjsvalue_cast<QVariant>(const QJSValue &value)
{
    return value.toVariant();
}

template <>
inline QVariant qjsvalue_cast<QVariant>(const QJSManagedValue &value)
{
    return value.toVariant();
}

template <>
inline QVariant qjsvalue_cast<QVariant>(const QJSPrimitiveValue &value)
{
    return value.toVariant();
}

Q_QML_EXPORT QJSEngine *qjsEngine(const QObject *);

QT_END_NAMESPACE

#endif // QJSENGINE_H
