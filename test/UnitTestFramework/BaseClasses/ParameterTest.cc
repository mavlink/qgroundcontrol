#include "ParameterTest.h"

#include <QtTest/QSignalSpy>

#include "Fact.h"
#include "ParameterManager.h"
#include "QGCLoggingCategory.h"
#include "Vehicle.h"

QGC_LOGGING_CATEGORY(ParameterTestLog, "Test.ParameterTest")

ParameterTest::ParameterTest(QObject* parent) : VehicleTest(parent)
{
    setWaitForParameters(true);
}

ParameterManager* ParameterTest::parameterManager() const
{
    return _vehicle ? _vehicle->parameterManager() : nullptr;
}

Fact* ParameterTest::getFact(const QString& name, int componentId) const
{
    if (!parameterManager()) {
        qCWarning(ParameterTestLog) << "getFact: no parameter manager (vehicle not connected?)";
        return nullptr;
    }

    if (componentId < 0) {
        componentId = ParameterManager::defaultComponentId;
    }

    if (!parameterManager()->parameterExists(componentId, name)) {
        qCWarning(ParameterTestLog) << "getFact: parameter not found:" << name << "compId:" << componentId;
        return nullptr;
    }

    return parameterManager()->getParameter(componentId, name);
}

bool ParameterTest::parameterExists(const QString& name, int componentId) const
{
    if (!parameterManager()) {
        return false;
    }

    if (componentId < 0) {
        componentId = ParameterManager::defaultComponentId;
    }

    return parameterManager()->parameterExists(componentId, name);
}

bool ParameterTest::waitForParameterUpdate(const QString& name, int timeoutMs)
{
    if (!parameterManager()) {
        qCWarning(ParameterTestLog) << "waitForParameterUpdate: no parameter manager";
        return false;
    }

    if (timeoutMs <= 0) {
        timeoutMs = TestTimeout::mediumMs();
    }

    Fact* fact = getFact(name);
    if (!fact) {
        qCWarning(ParameterTestLog) << "waitForParameterUpdate: parameter not found:" << name;
        return false;
    }

    QSignalSpy spy(fact, &Fact::rawValueChanged);
    return UnitTest::waitForSignal(spy, timeoutMs, QStringLiteral("Fact::rawValueChanged"));
}
