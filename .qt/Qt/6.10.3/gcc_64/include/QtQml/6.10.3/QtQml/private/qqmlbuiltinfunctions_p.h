// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLBUILTINFUNCTIONS_P_H
#define QQMLBUILTINFUNCTIONS_P_H

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

#include <private/qjsengine_p.h>
#include <private/qjsvalue_p.h>
#include <private/qjsmanagedvalue_p.h>
#include <private/qqmlengine_p.h>
#include <private/qqmlglobal_p.h>
#include <private/qqmlplatform_p.h>
#include <private/qqmltypewrapper_p.h>
#include <private/qv4functionobject_p.h>

#include <QtCore/qnamespace.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qsize.h>
#include <QtCore/qrect.h>
#include <QtCore/qpoint.h>

#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlengine.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

Q_DECLARE_LOGGING_CATEGORY(lcQml);
Q_DECLARE_LOGGING_CATEGORY(lcJs);

class Q_QML_EXPORT QtObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQmlApplication *application READ application CONSTANT)
    Q_PROPERTY(QQmlPlatform *platform READ platform CONSTANT)
    Q_PROPERTY(QObject *inputMethod READ inputMethod CONSTANT)
    Q_PROPERTY(QObject *styleHints READ styleHints CONSTANT)

#if QT_CONFIG(translation)
    Q_PROPERTY(QString uiLanguage READ uiLanguage WRITE setUiLanguage BINDABLE uiLanguageBindable NOTIFY uiLanguageChanged)
#endif

    QML_NAMED_ELEMENT(Qt)
    QML_SINGLETON
    QML_EXTENDED_NAMESPACE(Qt)

    Q_CLASSINFO("QML.StrictArguments", "true")

public:
    enum LoadingMode { Asynchronous = 0, Synchronous = 1 };
    Q_ENUM(LoadingMode);

    static QtObject *create(QQmlEngine *, QJSEngine *jsEngine);

    Q_INVOKABLE QJSValue include(const QString &url, const QJSValue &callback = QJSValue()) const;
    Q_INVOKABLE bool isQtObject(const QJSValue &value) const;

    Q_INVOKABLE QVariant color(const QString &name) const;
    Q_INVOKABLE QVariant rgba(double r, double g, double b, double a = 1) const;
    Q_INVOKABLE QVariant hsla(double h, double s, double l, double a = 1) const;
    Q_INVOKABLE QVariant hsva(double h, double s, double v, double a = 1) const;
    Q_INVOKABLE bool colorEqual(const QVariant &lhs, const QVariant &rhs) const;

    Q_INVOKABLE QRectF rect(double x, double y, double width, double height) const;
    Q_INVOKABLE QPointF point(double x, double y) const;
    Q_INVOKABLE QSizeF size(double width, double height) const;
    Q_INVOKABLE QVariant vector2d(double x, double y) const;
    Q_INVOKABLE QVariant vector3d(double x, double y, double z) const;
    Q_INVOKABLE QVariant vector4d(double x, double y, double z, double w) const;
    Q_INVOKABLE QVariant quaternion(double scalar, double x, double y, double z) const;

    Q_INVOKABLE QVariant matrix4x4() const;
    Q_INVOKABLE QVariant matrix4x4(double m11, double m12, double m13, double m14,
                                   double m21, double m22, double m23, double m24,
                                   double m31, double m32, double m33, double m34,
                                   double m41, double m42, double m43, double m44) const;
    Q_INVOKABLE QVariant matrix4x4(const QJSValue &value) const;

    Q_INVOKABLE QVariant lighter(const QJSValue &color, double factor = 1.5) const;
    Q_INVOKABLE QVariant darker(const QJSValue &color, double factor = 2.0) const;
    Q_INVOKABLE QVariant alpha(const QJSValue &baseColor, double value) const;
    Q_INVOKABLE QVariant tint(const QJSValue &baseColor, const QJSValue &tintColor) const;

    Q_INVOKABLE QString formatDate(QDate date, const QString &format) const;
    Q_INVOKABLE QString formatDate(const QDateTime &dateTime, const QString &format) const;
    Q_INVOKABLE QString formatDate(const QString &string, const QString &format) const;
    Q_INVOKABLE QString formatDate(QDate date, Qt::DateFormat format) const;
    Q_INVOKABLE QString formatDate(const QDateTime &dateTime, Qt::DateFormat format) const;
    Q_INVOKABLE QString formatDate(const QString &string, Qt::DateFormat format) const;

    Q_INVOKABLE QString formatTime(QTime time, const QString &format) const;
    Q_INVOKABLE QString formatTime(const QDateTime &dateTime, const QString &format) const;
    Q_INVOKABLE QString formatTime(const QString &time, const QString &format) const;
    Q_INVOKABLE QString formatTime(QTime time, Qt::DateFormat format) const;
    Q_INVOKABLE QString formatTime(const QDateTime &dateTime, Qt::DateFormat format) const;
    Q_INVOKABLE QString formatTime(const QString &time, Qt::DateFormat format) const;

    Q_INVOKABLE QString formatDateTime(const QDateTime &date, const QString &format) const;
    Q_INVOKABLE QString formatDateTime(const QString &string, const QString &format) const;
    Q_INVOKABLE QString formatDateTime(const QDateTime &date, Qt::DateFormat format) const;
    Q_INVOKABLE QString formatDateTime(const QString &string, Qt::DateFormat format) const;

#if QT_CONFIG(qml_locale)
    Q_INVOKABLE QString formatDate(QDate date, const QLocale &locale = QLocale(),
                                   QLocale::FormatType formatType = QLocale::ShortFormat) const;
    Q_INVOKABLE QString formatDate(const QDateTime &dateTime, const QLocale &locale = QLocale(),
                                   QLocale::FormatType formatType = QLocale::ShortFormat) const;
    Q_INVOKABLE QString formatDate(const QString &string, const QLocale &locale = QLocale(),
                                   QLocale::FormatType formatType = QLocale::ShortFormat) const;
    Q_INVOKABLE QString formatTime(QTime time, const QLocale &locale = QLocale(),
                                   QLocale::FormatType formatType = QLocale::ShortFormat) const;
    Q_INVOKABLE QString formatTime(const QDateTime &dateTime, const QLocale &locale = QLocale(),
                                   QLocale::FormatType formatType = QLocale::ShortFormat) const;
    Q_INVOKABLE QString formatTime(const QString &time, const QLocale &locale = QLocale(),
                                   QLocale::FormatType formatType = QLocale::ShortFormat) const;
    Q_INVOKABLE QString formatDateTime(const QDateTime &date, const QLocale &locale = QLocale(),
                                       QLocale::FormatType formatType = QLocale::ShortFormat) const;
    Q_INVOKABLE QString formatDateTime(const QString &string, const QLocale &locale = QLocale(),
                                       QLocale::FormatType formatType = QLocale::ShortFormat) const;
    Q_INVOKABLE QLocale locale() const;
    Q_INVOKABLE QLocale locale(const QString &name) const;
#endif

    Q_INVOKABLE QUrl url(const QUrl &url) const;
    Q_INVOKABLE QUrl resolvedUrl(const QUrl &url) const;
    Q_INVOKABLE QUrl resolvedUrl(const QUrl &url, QObject *context) const;
    Q_INVOKABLE bool openUrlExternally(const QUrl &url) const;

    Q_INVOKABLE QVariant font(const QJSValue &fontSpecifier) const;
    Q_INVOKABLE QStringList fontFamilies() const;

    Q_INVOKABLE QString md5(const QString &data) const;
    Q_INVOKABLE QString btoa(const QString &data) const;
    Q_INVOKABLE QString atob(const QString &data) const;

    Q_INVOKABLE void quit() const;
    Q_INVOKABLE void exit(int retCode) const;

    Q_INVOKABLE QObject *createQmlObject(const QString &qml, QObject *parent,
                                         const QUrl &url = QUrl(QStringLiteral("inline"))) const;
    Q_INVOKABLE QQmlComponent *createComponent(const QUrl &url, QObject *parent) const;
    Q_INVOKABLE QQmlComponent *createComponent(
            const QUrl &url, QQmlComponent::CompilationMode mode = QQmlComponent::PreferSynchronous,
            QObject *parent = nullptr) const;

    Q_INVOKABLE QQmlComponent *createComponent(const QString &moduleUri,
                                               const QString &typeName, QObject *parent) const;
    Q_INVOKABLE QQmlComponent *createComponent(const QString &moduleUri, const QString &typeName,
            QQmlComponent::CompilationMode mode = QQmlComponent::PreferSynchronous,
            QObject *parent = nullptr) const;

    Q_INVOKABLE QJSValue binding(const QJSValue &function) const;
    Q_INVOKABLE void callLater(QQmlV4FunctionPtr args);

    Q_INVOKABLE double enumStringToValue(const QJSManagedValue &enumType, const QString &string);
    Q_INVOKABLE QString enumValueToString(const QJSManagedValue &enumType, double value);
    Q_INVOKABLE QStringList enumValueToStrings(const QJSManagedValue &enumType, double value);

#if QT_CONFIG(translation)
    QString uiLanguage() const;
    void setUiLanguage(const QString &uiLanguage);
    QBindable<QString> uiLanguageBindable();
    Q_SIGNAL void uiLanguageChanged();
#endif

    // Not const because created on first use, and parented to this.
    QQmlPlatform *platform();
    QQmlApplication *application();

    QObject *inputMethod() const;
    QObject *styleHints() const;

private:
    friend struct QV4::ExecutionEngine;

    QtObject(QV4::ExecutionEngine *engine);

    QQmlEngine *qmlEngine() const { return m_engine->qmlEngine(); }
    QJSEngine *jsEngine() const { return m_engine->jsEngine(); }
    QV4::ExecutionEngine *v4Engine() const { return m_engine; }

    struct Contexts {
        QQmlRefPointer<QQmlContextData> context;
        QQmlRefPointer<QQmlContextData> effectiveContext;
    };
    Contexts getContexts() const;

    template<typename Ret, typename HandleScoped, typename HandleUnscoped>
    Ret retrieveFromEnum(const QJSManagedValue &enumType, HandleScoped &&handleScoped,
                         HandleUnscoped &&handleUnscoped, QV4::ExecutionEngine *engine)
    {
        Q_ASSERT(engine);

        // It's fine to hold a bare pointer to the internals of a QJSManagedValue
        // The managed value keeps a QV4::PersistentValue after all
        // (unless it's default-constructed).
        QV4::Value *internal = QJSManagedValuePrivate::member(&enumType);

        QV4::Heap::QQmlEnumWrapper *enumWrapper = nullptr;
        if (auto *wrapper = internal ? internal->as<QV4::QQmlEnumWrapper>() : nullptr) {
            enumWrapper = wrapper->d();
        } else {
            engine->throwTypeError("Invalid first argument, expected enum"_L1);
            return Ret();
        }

        bool ok;
        const QQmlType type = enumWrapper->type();
        const int enumIndex = enumWrapper->enumIndex;
        auto *typeLoader = m_engine->typeLoader();
        const auto value = enumWrapper->scoped
                ? handleScoped(type, typeLoader, enumIndex, &ok)
                : handleUnscoped(type, typeLoader, enumIndex, &ok);

        if (!ok)
            engine->throwReferenceError("Invalid second argument, entry"_L1);

        return value;
    }

    QQmlPlatform *m_platform = nullptr;
    QQmlApplication *m_application = nullptr;

    QV4::ExecutionEngine *m_engine = nullptr;
};

namespace QV4 {

namespace Heap {

struct ConsoleObject : Object {
    void init();
};

#define QQmlBindingFunctionMembers(class, Member) \
    Member(class, Pointer, JavaScriptFunctionObject *, bindingFunction)
DECLARE_HEAP_OBJECT(QQmlBindingFunction, JavaScriptFunctionObject) {
    DECLARE_MARKOBJECTS(QQmlBindingFunction)
    void init(const QV4::JavaScriptFunctionObject *bindingFunction);
};

}

struct ConsoleObject : Object
{
    V4_OBJECT2(ConsoleObject, Object)

    static ReturnedValue method_error(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_log(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_info(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_profile(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_profileEnd(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_time(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_timeEnd(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_count(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_trace(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_warn(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_assert(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_exception(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc);

};

struct Q_QML_EXPORT GlobalExtensions {
    static void init(Object *globalObject, QJSEngine::Extensions extensions);

#if QT_CONFIG(translation)
    static QString currentTranslationContext(ExecutionEngine *engine);
    static ReturnedValue method_qsTranslate(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_qsTranslateNoOp(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_qsTr(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_qsTrNoOp(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_qsTrId(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_qsTrIdNoOp(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc);
#endif
    static ReturnedValue method_gc(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc);

    // on String:prototype
    static ReturnedValue method_string_arg(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc);

};

struct QQmlBindingFunction : public QV4::JavaScriptFunctionObject
{
    V4_OBJECT2(QQmlBindingFunction, JavaScriptFunctionObject)

    static ReturnedValue virtualCall(
            const FunctionObject *f, const Value *thisObject, const Value *argv, int argc);

    Heap::JavaScriptFunctionObject *bindingFunction() const { return d()->bindingFunction; }
    QQmlSourceLocation currentLocation() const; // from caller stack trace
};

inline bool FunctionObject::isBinding() const
{
    return d()->vtable() == QQmlBindingFunction::staticVTable();
}

}

QT_END_NAMESPACE

#endif // QQMLBUILTINFUNCTIONS_P_H
