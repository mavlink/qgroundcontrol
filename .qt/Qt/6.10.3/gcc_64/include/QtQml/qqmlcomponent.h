// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLCOMPONENT_H
#define QQMLCOMPONENT_H

#include <QtCore/qvariant.h>
#include <QtCore/qmap.h>

#include <QtQml/qqml.h>
#include <QtQml/qqmlerror.h>

#include <QtCore/qobject.h>
#include <QtCore/qstring.h>
#include <QtQml/qjsvalue.h>

QT_BEGIN_NAMESPACE


class QByteArray;
class QQmlEngine;
class QQmlComponent;
class QQmlIncubator;
class QQmlComponentPrivate;
class QQmlComponentAttached;

namespace QV4 {
class ExecutableCompilationUnit;
}

class Q_QML_EXPORT QQmlComponent : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQmlComponent)

    Q_PROPERTY(qreal progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(Status status READ status NOTIFY statusChanged)
    Q_PROPERTY(QUrl url READ url CONSTANT)

public:
    enum CompilationMode { PreferSynchronous, Asynchronous };
    Q_ENUM(CompilationMode)

    QQmlComponent(QObject *parent = nullptr);
    QQmlComponent(QQmlEngine *, QObject *parent = nullptr);
    QQmlComponent(QQmlEngine *, const QString &fileName, QObject *parent = nullptr);
    QQmlComponent(QQmlEngine *, const QString &fileName, CompilationMode mode, QObject *parent = nullptr);
    QQmlComponent(QQmlEngine *, const QUrl &url, QObject *parent = nullptr);
    QQmlComponent(QQmlEngine *, const QUrl &url, CompilationMode mode, QObject *parent = nullptr);

    explicit QQmlComponent(QQmlEngine *engine, QAnyStringView uri, QAnyStringView typeName, QObject *parent = nullptr);
    explicit QQmlComponent(QQmlEngine *engine, QAnyStringView uri, QAnyStringView typeName, CompilationMode mode, QObject *parent = nullptr);

    ~QQmlComponent() override;

    enum Status { Null, Ready, Loading, Error };
    Q_ENUM(Status)
    Status status() const;

    bool isNull() const;
    bool isReady() const;
    bool isError() const;
    bool isLoading() const;

    bool isBound() const;

    QList<QQmlError> errors() const;
    Q_INVOKABLE QString errorString() const;

    qreal progress() const;

    QUrl url() const;

    virtual QObject *create(QQmlContext *context = nullptr);
    QObject *createWithInitialProperties(const QVariantMap& initialProperties, QQmlContext *context = nullptr);
    void setInitialProperties(QObject *component, const QVariantMap &properties);
    virtual QObject *beginCreate(QQmlContext *);
    virtual void completeCreate();

    void create(QQmlIncubator &, QQmlContext *context = nullptr,
                QQmlContext *forContext = nullptr);

    QQmlContext *creationContext() const;
    QQmlEngine *engine() const;

    static QQmlComponentAttached *qmlAttachedProperties(QObject *);

public Q_SLOTS:
    void loadUrl(const QUrl &url);
    void loadUrl(const QUrl &url, CompilationMode mode);
    void loadFromModule(QAnyStringView uri, QAnyStringView typeName,
                        QQmlComponent::CompilationMode mode = PreferSynchronous);
    void setData(const QByteArray &, const QUrl &baseUrl);

Q_SIGNALS:
    void statusChanged(QQmlComponent::Status);
    void progressChanged(qreal);

protected:
    QQmlComponent(QQmlComponentPrivate &dd, QObject* parent);

#if QT_DEPRECATED_SINCE(6, 3)
    QT_DEPRECATED_X("Use the overload with proper arguments")
    Q_INVOKABLE void createObject(QQmlV4FunctionPtr);
#endif

    Q_INVOKABLE QObject *createObject(
            QObject *parent = nullptr, const QVariantMap &properties = {});
    Q_INVOKABLE void incubateObject(QQmlV4FunctionPtr);

private:
    QQmlComponent(QQmlEngine *, QV4::ExecutableCompilationUnit *compilationUnit, int,
                  QObject *parent);

    Q_DISABLE_COPY(QQmlComponent)
    friend class QQmlTypeData;
    friend class QQmlObjectCreator;
};


// Don't do this at home.
namespace QQmlPrivate {

// Generally you cannot use QQmlComponentAttached as attached properties object in derived classes.
// It is private.
template<class T>
struct OverridableAttachedType<T, QQmlComponentAttached>
{
    using Type = void;
};

// QQmlComponent itself is allowed to use QQmlComponentAttached, though.
template<>
struct OverridableAttachedType<QQmlComponent, QQmlComponentAttached>
{
    using Type = QQmlComponentAttached;
};

} // namespace QQmlPrivate

QT_END_NAMESPACE

#endif // QQMLCOMPONENT_H
