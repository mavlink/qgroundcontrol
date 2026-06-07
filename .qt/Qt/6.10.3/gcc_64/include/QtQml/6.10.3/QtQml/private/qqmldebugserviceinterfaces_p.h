// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLDEBUGSERVICEINTERFACES_P_H
#define QQMLDEBUGSERVICEINTERFACES_P_H

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

#include <QtCore/qstring.h>
#include <private/qtqmlglobal_p.h>
#if QT_CONFIG(qml_debug)
#include <private/qqmldebugservice_p.h>
#endif
#include <private/qqmldebugstatesdelegate_p.h>
#include <private/qqmlboundsignal_p.h>
#include <private/qqmltranslation_p.h>

#include <limits>

QT_BEGIN_NAMESPACE

class QWindow;
class QQuickWindow;


#if !QT_CONFIG(qml_debug)

class TranslationBindingInformation;

class QV4DebugService
{
public:
    void signalEmitted(const QString &) {}
};

class QQmlProfilerService
{
public:
    void startProfiling(QJSEngine *engine, quint64 features = std::numeric_limits<quint64>::max())
    {
        Q_UNUSED(engine);
        Q_UNUSED(features);
    }

    void stopProfiling(QJSEngine *) {}
};

class QQmlEngineDebugService
{
public:
    void objectCreated(QJSEngine *, QObject *) {}
    static void setStatesDelegateFactory(QQmlDebugStatesDelegate *(*)()) {}
};

class QQmlInspectorService {
public:
    void addWindow(QQuickWindow *) {}
    void setParentWindow(QQuickWindow *, QWindow *) {}
    void removeWindow(QQuickWindow *) {}
};

class QDebugMessageService {};
class QQmlEngineControlService {};
class QQmlNativeDebugService {};
class QQmlDebugTranslationService {
public:
    virtual void foundTranslationBinding(const TranslationBindingInformation &) {}
};

#else

class Q_QML_EXPORT QV4DebugService : public QQmlDebugService
{
    Q_OBJECT
public:
    ~QV4DebugService() override;

    static const QString s_key;

    virtual void signalEmitted(const QString &signal) = 0;

protected:
    friend class QQmlDebugConnector;

    explicit QV4DebugService(float version, QObject *parent = nullptr) :
        QQmlDebugService(s_key, version, parent) {}
};

class QQmlAbstractProfilerAdapter;
class Q_QML_EXPORT QQmlProfilerService : public QQmlDebugService
{
    Q_OBJECT
public:
    ~QQmlProfilerService() override;

    static const QString s_key;

    virtual void addGlobalProfiler(QQmlAbstractProfilerAdapter *profiler) = 0;
    virtual void removeGlobalProfiler(QQmlAbstractProfilerAdapter *profiler) = 0;

    virtual void startProfiling(QJSEngine *engine,
                                quint64 features = std::numeric_limits<quint64>::max()) = 0;
    virtual void stopProfiling(QJSEngine *engine) = 0;

    virtual void dataReady(QQmlAbstractProfilerAdapter *profiler) = 0;

protected:
    friend class QQmlDebugConnector;

    explicit QQmlProfilerService(float version, QObject *parent = nullptr) :
        QQmlDebugService(s_key, version, parent) {}
};

class Q_QML_EXPORT QQmlEngineDebugService : public QQmlDebugService
{
    Q_OBJECT
public:
    ~QQmlEngineDebugService() override;

    static const QString s_key;

    virtual void objectCreated(QJSEngine *engine, QObject *object) = 0;
    static void setStatesDelegateFactory(QQmlDebugStatesDelegate *(*factory)());
    static QQmlDebugStatesDelegate *createStatesDelegate();

protected:
    friend class QQmlDebugConnector;

    explicit QQmlEngineDebugService(float version, QObject *parent = nullptr) :
        QQmlDebugService(s_key, version, parent) {}

    QQmlBoundSignal *nextSignal(QQmlBoundSignal *prev) { return prev->m_nextSignal; }
};

#if QT_CONFIG(translation)
struct TranslationBindingInformation
{
    static const TranslationBindingInformation
    create(const QQmlRefPointer<QV4::ExecutableCompilationUnit> &compilationUnit,
           const QV4::CompiledData::Binding *binding, QObject *scopeObject,
           QQmlRefPointer<QQmlContextData> ctxt);

    QQmlRefPointer<QV4::ExecutableCompilationUnit> compilationUnit;
    QObject *scopeObject;
    QQmlRefPointer<QQmlContextData> ctxt;

    QString propertyName;
    QQmlTranslation translation;

    quint32 line;
    quint32 column;
};

class Q_QML_EXPORT QQmlDebugTranslationService : public QQmlDebugService
{
    Q_OBJECT
public:
    ~QQmlDebugTranslationService() override;

    static const QString s_key;

    virtual void foundTranslationBinding(const TranslationBindingInformation &translationBindingInformation) = 0;
protected:
    friend class QQmlDebugConnector;

    explicit QQmlDebugTranslationService(float version, QObject *parent = nullptr) :
        QQmlDebugService(s_key, version, parent) {}

};
#endif //QT_CONFIG(translation)

class Q_QML_EXPORT QQmlInspectorService : public QQmlDebugService
{
    Q_OBJECT
public:
    ~QQmlInspectorService() override;

    static const QString s_key;

    virtual void addWindow(QQuickWindow *) = 0;
    virtual void setParentWindow(QQuickWindow *, QWindow *) = 0;
    virtual void removeWindow(QQuickWindow *) = 0;

protected:
    friend class QQmlDebugConnector;

    explicit QQmlInspectorService(float version, QObject *parent = nullptr) :
        QQmlDebugService(s_key, version, parent) {}
};

class Q_QML_EXPORT QDebugMessageService : public QQmlDebugService
{
    Q_OBJECT
public:
    ~QDebugMessageService() override;

    static const QString s_key;

    virtual void synchronizeTime(const QElapsedTimer &otherTimer) = 0;

protected:
    friend class QQmlDebugConnector;

    explicit QDebugMessageService(float version, QObject *parent = nullptr) :
        QQmlDebugService(s_key, version, parent) {}
};

class Q_QML_EXPORT QQmlEngineControlService : public QQmlDebugService
{
    Q_OBJECT
public:
    ~QQmlEngineControlService() override;

    static const QString s_key;

protected:
    friend class QQmlDebugConnector;

    QQmlEngineControlService(float version, QObject *parent = nullptr) :
        QQmlDebugService(s_key, version, parent) {}

};

class Q_QML_EXPORT QQmlNativeDebugService : public QQmlDebugService
{
    Q_OBJECT
public:
    ~QQmlNativeDebugService() override;

    static const QString s_key;

protected:
    friend class QQmlDebugConnector;

    explicit QQmlNativeDebugService(float version, QObject *parent = nullptr)
        : QQmlDebugService(s_key, version,  parent) {}
};

#endif

QT_END_NAMESPACE

#endif // QQMLDEBUGSERVICEINTERFACES_P_H

