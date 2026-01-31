#include "StateContextTest.h"

#include "MultiSignalSpyV2.h"
#include "QGCStateMachine.h"
#include "StateContext.h"

#include <QtCore/QTimer>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>


void StateContextTest::_testSetAndGet()
{
    StateContext context;

    context.set("intValue", 42);
    context.set("stringValue", QString("hello"));
    context.set("doubleValue", 3.14);

    auto intResult = context.get<int>("intValue");
    QVERIFY(intResult.has_value());
    QCOMPARE(*intResult, 42);

    auto stringResult = context.get<QString>("stringValue");
    QVERIFY(stringResult.has_value());
    QCOMPARE(*stringResult, QString("hello"));

    auto doubleResult = context.get<double>("doubleValue");
    QVERIFY(doubleResult.has_value());
    QCOMPARE(*doubleResult, 3.14);
}

void StateContextTest::_testGetOr()
{
    StateContext context;

    context.set("existing", 100);

    // Existing key
    QCOMPARE(context.getOr<int>("existing", 0), 100);

    // Non-existing key
    QCOMPARE(context.getOr<int>("nonexistent", 999), 999);
}

void StateContextTest::_testContains()
{
    StateContext context;

    QVERIFY(!context.contains("key"));

    context.set("key", 1);
    QVERIFY(context.contains("key"));
}

void StateContextTest::_testContainsType()
{
    StateContext context;

    context.set("intKey", 42);

    QVERIFY(context.containsType<int>("intKey"));
    QVERIFY(!context.containsType<QString>("intKey"));
    QVERIFY(!context.containsType<int>("nonexistent"));
}

void StateContextTest::_testRemove()
{
    StateContext context;

    context.set("key", 1);
    QVERIFY(context.contains("key"));

    bool removed = context.remove("key");
    QVERIFY(removed);
    QVERIFY(!context.contains("key"));

    // Remove non-existent key
    QVERIFY(!context.remove("nonexistent"));
}

void StateContextTest::_testClear()
{
    StateContext context;

    context.set("key1", 1);
    context.set("key2", 2);
    QCOMPARE(context.count(), 2);

    context.clear();
    QCOMPARE(context.count(), 0);
}

void StateContextTest::_testKeys()
{
    StateContext context;

    context.set("alpha", 1);
    context.set("beta", 2);
    context.set("gamma", 3);

    QStringList keys = context.keys();
    QCOMPARE(keys.count(), 3);
    QVERIFY(keys.contains("alpha"));
    QVERIFY(keys.contains("beta"));
    QVERIFY(keys.contains("gamma"));
}

void StateContextTest::_testTypeMismatch()
{
    StateContext context;

    context.set("intValue", 42);

    // Try to get as wrong type
    auto stringResult = context.get<QString>("intValue");
    QVERIFY(!stringResult.has_value());

    // Original value is unchanged
    auto intResult = context.get<int>("intValue");
    QVERIFY(intResult.has_value());
    QCOMPARE(*intResult, 42);
}

void StateContextTest::_testVariantApi()
{
    StateContext context;

    context.setVariant("qmlKey", QVariant(123));
    QVERIFY(context.containsVariant("qmlKey"));

    QVariant value = context.variant("qmlKey");
    QCOMPARE(value.toInt(), 123);
}

void StateContextTest::_testContextFromState()
{
    QGCStateMachine machine(QStringLiteral("ContextTest"), nullptr);

    auto* state = new FunctionState(QStringLiteral("TestState"), &machine, [&machine]() {
        // Access context from within state via machine
        machine.context().set("fromState", true);
    });
    auto* finalState = machine.addFinalState();

    state->addTransition(state, &QGCState::advance, finalState);
    machine.setInitialState(state);

    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);
    machine.start();

    QVERIFY(finishedSpy.wait(500));

    // Verify state set the context
    auto result = machine.context().get<bool>("fromState");
    QVERIFY(result.has_value());
    QVERIFY(*result);
}

void StateContextTest::_testDataPassingBetweenStates()
{
    QGCStateMachine machine(QStringLiteral("DataPassingTest"), nullptr);
    int receivedValue = 0;

    // Producer state: sets data in context
    auto* producer = new FunctionState(QStringLiteral("Producer"), &machine, [&machine]() {
        machine.context().set("computedValue", 42);
        machine.context().set("userName", QString("Alice"));
    });

    // Consumer state: reads data from context
    auto* consumer = new FunctionState(QStringLiteral("Consumer"), &machine, [&machine, &receivedValue]() {
        auto value = machine.context().get<int>("computedValue");
        if (value) {
            receivedValue = *value;
        }
    });

    auto* finalState = machine.addFinalState();

    producer->addTransition(producer, &QGCState::advance, consumer);
    consumer->addTransition(consumer, &QGCState::advance, finalState);
    machine.setInitialState(producer);

    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);
    machine.start();

    QVERIFY(finishedSpy.wait(500));

    // Consumer should have received the value from producer
    QCOMPARE(receivedValue, 42);

    // Verify both values are in context
    auto name = machine.context().get<QString>("userName");
    QVERIFY(name.has_value());
    QCOMPARE(*name, QString("Alice"));
}

void StateContextTest::_testStateContextAccessor()
{
    // Test that states can access context via their context() method
    QGCStateMachine machine(QStringLiteral("StateAccessorTest"), nullptr);
    bool contextAccessedViaState = false;

    // Create a custom state that uses the context() accessor
    auto* state = new QGCState(QStringLiteral("TestState"), &machine);
    state->setOnEntry([state, &contextAccessedViaState]() {
        // Access context via state's context() method
        StateContext* ctx = state->context();
        if (ctx) {
            ctx->set("accessedViaState", true);
            contextAccessedViaState = true;
        }
        emit state->advance();
    });

    auto* finalState = machine.addFinalState();
    state->addTransition(state, &QGCState::advance, finalState);
    machine.setInitialState(state);

    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);
    machine.start();

    QVERIFY(finishedSpy.wait(500));
    QVERIFY(contextAccessedViaState);

    auto result = machine.context().get<bool>("accessedViaState");
    QVERIFY(result.has_value());
    QVERIFY(*result);
}
