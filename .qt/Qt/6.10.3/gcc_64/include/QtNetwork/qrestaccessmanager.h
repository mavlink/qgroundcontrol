// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QRESTACCESSMANAGER_H
#define QRESTACCESSMANAGER_H

#if 0
#pragma qt_class(QRestAccessManager)
#endif

#include <QtNetwork/qnetworkaccessmanager.h>

QT_BEGIN_NAMESPACE

class QDebug;
class QRestReply;

#define QREST_METHOD_WITH_DATA(METHOD, DATA)                                                     \
public:                                                                                          \
template <typename Functor, if_compatible_callback<Functor> = true>                              \
QNetworkReply *METHOD(const QNetworkRequest &request, DATA data,                                 \
       const ContextTypeForFunctor<Functor> *context,                                            \
       Functor &&callback)                                                                       \
{                                                                                                \
    return METHOD##WithDataImpl(request, data, context,                                          \
           QtPrivate::makeCallableObject<CallbackPrototype>(std::forward<Functor>(callback)));   \
}                                                                                                \
QNetworkReply *METHOD(const QNetworkRequest &request, DATA data)                                 \
{                                                                                                \
    return METHOD##WithDataImpl(request, data, nullptr, nullptr);                                \
}                                                                                                \
private:                                                                                         \
QNetworkReply *METHOD##WithDataImpl(const QNetworkRequest &request, DATA data,                   \
                                 const QObject *context, QtPrivate::QSlotObjectBase *slot);      \
/* end */

#define QREST_METHOD_NO_DATA(METHOD)                                                             \
public:                                                                                          \
template <typename Functor, if_compatible_callback<Functor> = true>                              \
QNetworkReply *METHOD(const QNetworkRequest &request,                                            \
       const ContextTypeForFunctor<Functor> *context,                                            \
       Functor &&callback)                                                                       \
{                                                                                                \
    return METHOD##NoDataImpl(request, context,                                                  \
           QtPrivate::makeCallableObject<CallbackPrototype>(std::forward<Functor>(callback)));   \
}                                                                                                \
QNetworkReply *METHOD(const QNetworkRequest &request)                                            \
{                                                                                                \
    return METHOD##NoDataImpl(request, nullptr, nullptr);                                        \
}                                                                                                \
private:                                                                                         \
QNetworkReply *METHOD##NoDataImpl(const QNetworkRequest &request,                                \
                               const QObject *context, QtPrivate::QSlotObjectBase *slot);        \
/* end */

#define QREST_METHOD_CUSTOM_WITH_DATA(DATA)                                                      \
public:                                                                                          \
template <typename Functor, if_compatible_callback<Functor> = true>                              \
QNetworkReply *sendCustomRequest(const QNetworkRequest& request, const QByteArray &method, DATA data, \
       const ContextTypeForFunctor<Functor> *context,                                            \
       Functor &&callback)                                                                       \
{                                                                                                \
    return customWithDataImpl(request, method, data, context,                                    \
           QtPrivate::makeCallableObject<CallbackPrototype>(std::forward<Functor>(callback)));   \
}                                                                                                \
QNetworkReply *sendCustomRequest(const QNetworkRequest& request, const QByteArray &method, DATA data) \
{                                                                                                \
    return customWithDataImpl(request, method, data, nullptr, nullptr);                          \
}                                                                                                \
private:                                                                                         \
QNetworkReply *customWithDataImpl(const QNetworkRequest& request, const QByteArray &method,      \
                               DATA data, const QObject* context,                                \
                               QtPrivate::QSlotObjectBase *slot);                                \
/* end */

class QRestAccessManagerPrivate;
class Q_NETWORK_EXPORT QRestAccessManager : public QObject
{
    Q_OBJECT
    using CallbackPrototype = void(*)(QRestReply&);
    template <typename Functor>
    using ContextTypeForFunctor = typename QtPrivate::ContextTypeForFunctor<Functor>::ContextType;
    template <typename Functor>
    using if_compatible_callback = std::enable_if_t<
                     QtPrivate::AreFunctionsCompatible<CallbackPrototype, Functor>::value, bool>;
public:
    explicit QRestAccessManager(QNetworkAccessManager *manager, QObject *parent = nullptr);
    ~QRestAccessManager() override;

    QNetworkAccessManager *networkAccessManager() const;

    QREST_METHOD_NO_DATA(deleteResource)
    QREST_METHOD_NO_DATA(head)
    QREST_METHOD_NO_DATA(get)
    QREST_METHOD_WITH_DATA(get, const QByteArray &)
    QREST_METHOD_WITH_DATA(get, const QJsonDocument &)
    QREST_METHOD_WITH_DATA(get, QIODevice *)
    QREST_METHOD_WITH_DATA(post, const QJsonDocument &)
    QREST_METHOD_WITH_DATA(post, const QVariantMap &)
    QREST_METHOD_WITH_DATA(post, const QByteArray &)
    QREST_METHOD_WITH_DATA(post, QHttpMultiPart *)
    QREST_METHOD_WITH_DATA(post, QIODevice *)
    QREST_METHOD_WITH_DATA(put, const QJsonDocument &)
    QREST_METHOD_WITH_DATA(put, const QVariantMap &)
    QREST_METHOD_WITH_DATA(put, const QByteArray &)
    QREST_METHOD_WITH_DATA(put, QHttpMultiPart *)
    QREST_METHOD_WITH_DATA(put, QIODevice *)
    QREST_METHOD_WITH_DATA(patch, const QJsonDocument &)
    QREST_METHOD_WITH_DATA(patch, const QVariantMap &)
    QREST_METHOD_WITH_DATA(patch, const QByteArray &)
    QREST_METHOD_WITH_DATA(patch, QIODevice *)
    QREST_METHOD_CUSTOM_WITH_DATA(const QByteArray &)
    QREST_METHOD_CUSTOM_WITH_DATA(QIODevice *)
    QREST_METHOD_CUSTOM_WITH_DATA(QHttpMultiPart *)

private:
    Q_DECLARE_PRIVATE(QRestAccessManager)
    Q_DISABLE_COPY(QRestAccessManager)
};

#undef QREST_METHOD_NO_DATA
#undef QREST_METHOD_WITH_DATA
#undef QREST_METHOD_CUSTOM_WITH_DATA

QT_END_NAMESPACE

#endif
