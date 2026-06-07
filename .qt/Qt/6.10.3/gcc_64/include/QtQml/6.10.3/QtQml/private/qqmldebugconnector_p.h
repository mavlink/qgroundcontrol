// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLDEBUGCONNECTOR_H
#define QQMLDEBUGCONNECTOR_H

#include <QtQml/qtqmlglobal.h>
#include <QtQml/qjsengine.h>
#include <QtCore/QVariantList>

#if QT_CONFIG(qml_debug)
#include <private/qqmldebugservice_p.h>
#endif

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

QT_BEGIN_NAMESPACE

#if !QT_CONFIG(qml_debug)

class Q_QML_EXPORT QQmlDebugConnector
{
    virtual ~QQmlDebugConnector() = default; // don't break 'override' on ~QQmlDebugServer
public:
    static QQmlDebugConnector *instance() { return nullptr; }

    template<class Service>
    static Service *service() { return nullptr; }

    bool hasEngine(QJSEngine *) const { return false; }
    void addEngine(QJSEngine *) {}
    void removeEngine(QJSEngine *) {}

    bool open(const QVariantHash &configuration = QVariantHash())
    {
        Q_UNUSED(configuration);
        return false;
    }
};

#else

class QQmlDebugService;
class Q_QML_EXPORT QQmlDebugConnector : public QObject
{
    Q_OBJECT
public:
    static void setPluginKey(const QString &key);
    static void setServices(const QStringList &services);
    static QQmlDebugConnector *instance();
    static int dataStreamVersion()
    {
        return s_dataStreamVersion;
    }

    virtual bool blockingMode() const = 0;

    virtual QQmlDebugService *service(const QString &name) const = 0;

    virtual void addEngine(QJSEngine *engine) = 0;
    virtual void removeEngine(QJSEngine *engine) = 0;
    virtual bool hasEngine(QJSEngine *engine) const = 0;

    virtual bool addService(const QString &name, QQmlDebugService *service) = 0;
    virtual bool removeService(const QString &name) = 0;

    virtual bool open(const QVariantHash &configuration = QVariantHash()) = 0;

    template<class Service>
    static Service *service()
    {
        QQmlDebugConnector *inst = instance();
        return inst ? static_cast<Service *>(inst->service(Service::s_key)) : nullptr;
    }

protected:
    static QString commandLineArguments();
    static int s_dataStreamVersion;
};

class Q_QML_EXPORT QQmlDebugConnectorFactory : public QObject {
    Q_OBJECT
public:
    virtual QQmlDebugConnector *create(const QString &key) = 0;
    ~QQmlDebugConnectorFactory() override;
};

#define QQmlDebugConnectorFactory_iid "org.qt-project.Qt.QQmlDebugConnectorFactory"

#endif

QT_END_NAMESPACE

#endif // QQMLDEBUGCONNECTOR_H
