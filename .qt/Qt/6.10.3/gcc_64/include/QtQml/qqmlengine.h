// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLENGINE_H
#define QQMLENGINE_H

#include <QtCore/qurl.h>
#include <QtCore/qobject.h>
#include <QtCore/qmap.h>
#include <QtQml/qjsengine.h>
#include <QtQml/qqml.h>
#include <QtQml/qqmlerror.h>
#include <QtQml/qqmlabstracturlinterceptor.h>

QT_BEGIN_NAMESPACE

class Q_QML_EXPORT QQmlImageProviderBase : public QObject
{
    Q_OBJECT
public:
    enum ImageType : int {
        Invalid = 0,
        Image,
        Pixmap,
        Texture,
        ImageResponse,
    };

    enum Flag {
        ForceAsynchronousImageLoading  = 0x01
    };
    Q_DECLARE_FLAGS(Flags, Flag)

    virtual ~QQmlImageProviderBase();

    virtual ImageType imageType() const = 0;
    virtual Flags flags() const = 0;

private:
    friend class QQuickImageProvider;
    QQmlImageProviderBase();
};
Q_DECLARE_OPERATORS_FOR_FLAGS(QQmlImageProviderBase::Flags)

class QQmlComponent;
class QQmlEnginePrivate;
class QQmlExpression;
class QQmlContext;
class QQmlType;
class QUrl;
#if QT_CONFIG(qml_network)
class QNetworkAccessManager;
class QQmlNetworkAccessManagerFactory;
#endif
class QQmlIncubationController;
class Q_QML_EXPORT QQmlEngine : public QJSEngine
{
    Q_PROPERTY(QString offlineStoragePath READ offlineStoragePath WRITE setOfflineStoragePath NOTIFY offlineStoragePathChanged)
    Q_OBJECT
public:
    explicit QQmlEngine(QObject *p = nullptr);
    ~QQmlEngine() override;

    QQmlContext *rootContext() const;

    void clearComponentCache();
    void trimComponentCache();
    void clearSingletons();

    QStringList importPathList() const;
    void setImportPathList(const QStringList &paths);
    void addImportPath(const QString& dir);

    QStringList pluginPathList() const;
    void setPluginPathList(const QStringList &paths);
    void addPluginPath(const QString& dir);

#if QT_DEPRECATED_SINCE(6, 0)
    QT_DEPRECATED bool addNamedBundle(const QString &, const QString &) { return false; }
#endif

#if QT_CONFIG(library)
#if QT_DEPRECATED_SINCE(6, 4)
    QT_DEPRECATED_VERSION_X_6_4("Import the module from QML instead")
    bool importPlugin(const QString &filePath, const QString &uri, QList<QQmlError> *errors);
#endif
#endif

#if QT_CONFIG(qml_network)
    void setNetworkAccessManagerFactory(QQmlNetworkAccessManagerFactory *);
    QQmlNetworkAccessManagerFactory *networkAccessManagerFactory() const;

    QNetworkAccessManager *networkAccessManager() const;
#endif

#if QT_DEPRECATED_SINCE(6, 0)
    QT_DEPRECATED void setUrlInterceptor(QQmlAbstractUrlInterceptor* urlInterceptor)
    {
        addUrlInterceptor(urlInterceptor);
    }
    QT_DEPRECATED QQmlAbstractUrlInterceptor *urlInterceptor() const;
#endif

    void addUrlInterceptor(QQmlAbstractUrlInterceptor *urlInterceptor);
    void removeUrlInterceptor(QQmlAbstractUrlInterceptor *urlInterceptor);
    QList<QQmlAbstractUrlInterceptor *> urlInterceptors() const;
    QUrl interceptUrl(const QUrl &url, QQmlAbstractUrlInterceptor::DataType type) const;

    void addImageProvider(const QString &id, QQmlImageProviderBase *);
    QQmlImageProviderBase *imageProvider(const QString &id) const;
    void removeImageProvider(const QString &id);

    void setIncubationController(QQmlIncubationController *);
    QQmlIncubationController *incubationController() const;

    void setOfflineStoragePath(const QString& dir);
    QString offlineStoragePath() const;
    QString offlineStorageDatabaseFilePath(const QString &databaseName) const;

    QUrl baseUrl() const;
    void setBaseUrl(const QUrl &);

    bool outputWarningsToStandardError() const;
    void setOutputWarningsToStandardError(bool);

    void markCurrentFunctionAsTranslationBinding();

    template<typename T>
    T singletonInstance(int qmlTypeId);

    template<typename T>
    T singletonInstance(QAnyStringView moduleName, QAnyStringView typeName);

    void captureProperty(QObject *object, const QMetaProperty &property) const;

public Q_SLOTS:
    void retranslate();

Q_SIGNALS:
    void offlineStoragePathChanged();

public:
    static QQmlContext *contextForObject(const QObject *);
    static void setContextForObject(QObject *, QQmlContext *);

protected:
    QQmlEngine(QQmlEnginePrivate &dd, QObject *p);
    bool event(QEvent *) override;

Q_SIGNALS:
    void quit();
    void exit(int retCode);
    void warnings(const QList<QQmlError> &warnings);

private:
    Q_DISABLE_COPY(QQmlEngine)
    Q_DECLARE_PRIVATE(QQmlEngine)
};

template<>
Q_QML_EXPORT QJSValue QQmlEngine::singletonInstance<QJSValue>(int qmlTypeId);

template<typename T>
T QQmlEngine::singletonInstance(int qmlTypeId) {
    return qobject_cast<T>(singletonInstance<QJSValue>(qmlTypeId).toQObject());
}

template<>
Q_QML_EXPORT QJSValue QQmlEngine::singletonInstance<QJSValue>(QAnyStringView uri, QAnyStringView typeName);

template<typename T>
T QQmlEngine::singletonInstance(QAnyStringView uri, QAnyStringView typeName)
{
    return qobject_cast<T>(singletonInstance<QJSValue>(uri, typeName).toQObject());
}

QT_END_NAMESPACE

#endif // QQMLENGINE_H
