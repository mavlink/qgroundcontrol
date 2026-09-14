#pragma once

#include <QtQml/QQmlAbstractUrlInterceptor>

#include "QGCCorePlugin.h"

class DigiviewManager;
class SVBackend;

class CustomPlugin final : public QGCCorePlugin
{
    Q_OBJECT
    Q_PROPERTY(int svInitialWelcomePromptId MEMBER kSVInitialWelcomePromptId CONSTANT)

public:
    explicit CustomPlugin(QObject* parent = nullptr);

    static QGCCorePlugin* instance();

    SVBackend* backend() const { return _backend; }
    const QVariantList& toolBarIndicators() final { return _toolBarIndicators; }
    QList<int> firstRunPromptCustomIds() final { return {}; }
    QString firstRunPromptResource(int id) const final;
    QQmlApplicationEngine* createQmlApplicationEngine(QObject* parent) final;
    void destroyQmlApplicationEngine(QQmlApplicationEngine* qmlEngine) final;

    static constexpr int kSVInitialWelcomePromptId = kFirstRunPromptIdsFirstCustomId + 1;

private:
    DigiviewManager* const _digiviewManager;
    SVBackend* const _backend;
    QVariantList _toolBarIndicators;
    QQmlApplicationEngine* _qmlEngine = nullptr;
    class CustomOverrideInterceptor* _urlInterceptor = nullptr;
};

class CustomOverrideInterceptor final : public QQmlAbstractUrlInterceptor
{
public:
    QUrl intercept(const QUrl& url, DataType type) final;
};
