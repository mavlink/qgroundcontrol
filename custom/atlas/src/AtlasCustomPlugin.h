#pragma once

#include "AtlasJetsonClient.h"
#include "QGCCorePlugin.h"

#include <QtQml/QQmlAbstractUrlInterceptor>

class QQmlApplicationEngine;

class AtlasCustomPlugin : public QGCCorePlugin
{
    Q_OBJECT

public:
    explicit AtlasCustomPlugin(QObject *parent = nullptr);

    static QGCCorePlugin *instance();

    QQmlApplicationEngine *createQmlApplicationEngine(
        QObject *parent
    ) final;

    void destroyQmlApplicationEngine(
        QQmlApplicationEngine *qmlEngine
    ) final;

private:
    QQmlApplicationEngine *_qmlEngine = nullptr;

    class AtlasOverrideInterceptor *_urlInterceptor = nullptr;

    AtlasJetsonClient _jetsonClient;
};


class AtlasOverrideInterceptor : public QQmlAbstractUrlInterceptor
{
public:
    AtlasOverrideInterceptor();

    QUrl intercept(
        const QUrl &url,
        QQmlAbstractUrlInterceptor::DataType type
    ) final;
};
