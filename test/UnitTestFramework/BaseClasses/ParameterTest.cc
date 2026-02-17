#include "ParameterTest.h"

#include <QtTest/QSignalSpy>

#include "Fact.h"
#include "ParameterManager.h"
#include "Vehicle.h"

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
        return nullptr;
    }

    if (componentId < 0) {
        componentId = ParameterManager::defaultComponentId;
    }

    if (!parameterManager()->parameterExists(componentId, name)) {
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
        return false;
    }

    if (timeoutMs <= 0) {
        timeoutMs = TestTimeout::mediumMs();
    }

    Fact* fact = getFact(name);
    if (!fact) {
        return false;
    }

    QSignalSpy spy(fact, &Fact::rawValueChanged);
    return spy.wait(timeoutMs);
}
