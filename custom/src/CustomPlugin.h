#pragma once

#include <QtQml/QQmlAbstractUrlInterceptor>

#include <QtCore/QHash>

#include "QGCCorePlugin.h"

class DigiviewManager;
class SVBackend;
class Vehicle;

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
    void prepareForClose() final;

    static constexpr int kSVInitialWelcomePromptId = kFirstRunPromptIdsFirstCustomId + 1;

private:
    void _connectVehicle(Vehicle* vehicle);
    void _completePrepareForClose();

    DigiviewManager* const _digiviewManager;
    SVBackend* const _backend;
    QVariantList _toolBarIndicators;
    QQmlApplicationEngine* _qmlEngine = nullptr;
    class CustomOverrideInterceptor* _urlInterceptor = nullptr;
    QHash<Vehicle*, QMetaObject::Connection> _vehicleConnections;
    bool _closePreparationStarted = false;
    bool _closePreparationCompleted = false;
};

class CustomOverrideInterceptor final : public QQmlAbstractUrlInterceptor
{
public:
    QUrl intercept(const QUrl& url, DataType type) final;
};
