#include "MultiSignalSpyTest.h"
#include "MultiSignalSpy.h"

#include <QtCore/QObject>
#include <QtTest/QTest>

// Test object with signals for MultiSignalSpy testing
class TestEmitter : public QObject
{
    Q_OBJECT

public:
    explicit TestEmitter(QObject *parent = nullptr) : QObject(parent) {}

    void emitSignal1() { emit signal1(); }
    void emitSignal2() { emit signal2(); }
    void emitSignal3() { emit signal3(); }
    void emitValueChanged(int value) { emit valueChanged(value); }
    void emitBoolChanged(bool value) { emit boolChanged(value); }
    void emitStringChanged(const QString &value) { emit stringChanged(value); }

signals:
    void signal1();
    void signal2();
    void signal3();
    void valueChanged(int value);
    void boolChanged(bool value);
    void stringChanged(const QString &value);
};

// ============================================================================
// Initialization Tests
// ============================================================================

void MultiSignalSpyTest::_testInitWithNullEmitter()
{
    MultiSignalSpy spy;
    QVERIFY(!spy.init(nullptr));
    QVERIFY(!spy.isValid());
}

void MultiSignalSpyTest::_testInitWithValidEmitter()
{
    TestEmitter emitter;
    MultiSignalSpy spy;

    QVERIFY(spy.init(&emitter));
    QVERIFY(spy.isValid());
    QVERIFY(spy.signalCount() > 0);
}

void MultiSignalSpyTest::_testInitWithSpecificSignals()
{
    TestEmitter emitter;
    MultiSignalSpy spy;

    QVERIFY(spy.init(&emitter, {"signal1", "signal2"}));
    QVERIFY(spy.isValid());
    QCOMPARE_EQ(spy.signalCount(), 2);
    QVERIFY(spy.hasSignal("signal1"));
    QVERIFY(spy.hasSignal("signal2"));
    QVERIFY(!spy.hasSignal("signal3"));
}

void MultiSignalSpyTest::_testInitWithEmptySignalList()
{
    TestEmitter emitter;
    MultiSignalSpy spy;

    QVERIFY(!spy.init(&emitter, {}));
    QVERIFY(!spy.isValid());
}

void MultiSignalSpyTest::_testInitWithInvalidSignalName()
{
    TestEmitter emitter;
    MultiSignalSpy spy;

    QVERIFY(!spy.init(&emitter, {"nonExistentSignal"}));
    QVERIFY(!spy.isValid());
}

void MultiSignalSpyTest::_testInitMaxSignalLimit()
{
    // This test verifies the constant is defined correctly
    QCOMPARE_EQ(MultiSignalSpy::kMaxSignals, 64);
}

void MultiSignalSpyTest::_testReInitCleansUp()
{
    TestEmitter emitter1;
    TestEmitter emitter2;
    MultiSignalSpy spy;

    // First init
    QVERIFY(spy.init(&emitter1, {"signal1"}));
    emitter1.emitSignal1();
    QCOMPARE_EQ(spy.count("signal1"), 1);

    // Re-init should clean up previous state
    QVERIFY(spy.init(&emitter2, {"signal2"}));
    QVERIFY(!spy.hasSignal("signal1"));
    QVERIFY(spy.hasSignal("signal2"));
    QCOMPARE_EQ(spy.count("signal2"), 0);
}

// ============================================================================
// Constants Tests
// ============================================================================

void MultiSignalSpyTest::_testMaxSignalsConstant()
{
    // kMaxSignals should be 64 (quint64 bitmask limitation)
    QCOMPARE_EQ(MultiSignalSpy::kMaxSignals, 64);
}

// ============================================================================
// isValid Tests
// ============================================================================

void MultiSignalSpyTest::_testIsValidAfterInit()
{
    TestEmitter emitter;
    MultiSignalSpy spy;

    QVERIFY(spy.init(&emitter));
    QVERIFY(spy.isValid());
}

void MultiSignalSpyTest::_testIsValidBeforeInit()
{
    MultiSignalSpy spy;
    QVERIFY(!spy.isValid());
}

// ============================================================================
// Signal Checking Tests
// ============================================================================

void MultiSignalSpyTest::_testCheckSignalNotEmitted()
{
    TestEmitter emitter;
    MultiSignalSpy spy;
    spy.init(&emitter, {"signal1"});

    // Signal not emitted, checkSignal should return false
    QVERIFY(!spy.checkSignal("signal1"));
}

void MultiSignalSpyTest::_testCheckSignalEmittedOnce()
{
    TestEmitter emitter;
    MultiSignalSpy spy;
    spy.init(&emitter, {"signal1"});

    emitter.emitSignal1();

    QVERIFY(spy.checkSignal("signal1"));
}

void MultiSignalSpyTest::_testCheckSignalEmittedMultiple()
{
    TestEmitter emitter;
    MultiSignalSpy spy;
    spy.init(&emitter, {"signal1"});

    emitter.emitSignal1();
    emitter.emitSignal1();

    // checkSignal expects exactly 1, checkSignals expects at least 1
    QVERIFY(!spy.checkSignal("signal1"));
    QVERIFY(spy.checkSignals("signal1"));
}

void MultiSignalSpyTest::_testCheckOnlySignal()
{
    TestEmitter emitter;
    MultiSignalSpy spy;
    spy.init(&emitter, {"signal1", "signal2"});

    emitter.emitSignal1();

    QVERIFY(spy.checkOnlySignal("signal1"));

    // Emit signal2, now checkOnlySignal should fail
    emitter.emitSignal2();
    spy.clearAllSignals();
    emitter.emitSignal1();
    emitter.emitSignal2();
    QVERIFY(!spy.checkOnlySignal("signal1"));
}

void MultiSignalSpyTest::_testCheckNoSignal()
{
    TestEmitter emitter;
    MultiSignalSpy spy;
    spy.init(&emitter, {"signal1"});

    QVERIFY(spy.checkNoSignal("signal1"));

    emitter.emitSignal1();
    QVERIFY(!spy.checkNoSignal("signal1"));
}

void MultiSignalSpyTest::_testCheckNoSignals()
{
    TestEmitter emitter;
    MultiSignalSpy spy;
    spy.init(&emitter, {"signal1", "signal2"});

    QVERIFY(spy.checkNoSignals());

    emitter.emitSignal1();
    QVERIFY(!spy.checkNoSignals());
}

// ============================================================================
// Mask Tests
// ============================================================================

void MultiSignalSpyTest::_testMaskForKnownSignal()
{
    TestEmitter emitter;
    MultiSignalSpy spy;
    spy.init(&emitter, {"signal1", "signal2"});

    quint64 mask1 = spy.mask("signal1");
    quint64 mask2 = spy.mask("signal2");

    QVERIFY(mask1 != 0);
    QVERIFY(mask2 != 0);
    QVERIFY(mask1 != mask2);
}

void MultiSignalSpyTest::_testMaskForUnknownSignal()
{
    TestEmitter emitter;
    MultiSignalSpy spy;
    spy.init(&emitter, {"signal1"});

    quint64 mask = spy.mask("nonExistent");
    QCOMPARE_EQ(mask, 0ULL);
}

void MultiSignalSpyTest::_testMaskCombination()
{
    TestEmitter emitter;
    MultiSignalSpy spy;
    spy.init(&emitter, {"signal1", "signal2"});

    quint64 combined = spy.mask("signal1", "signal2");
    quint64 individual = spy.mask("signal1") | spy.mask("signal2");

    QCOMPARE_EQ(combined, individual);
}

// ============================================================================
// Clear Tests
// ============================================================================

void MultiSignalSpyTest::_testClearSignal()
{
    TestEmitter emitter;
    MultiSignalSpy spy;
    spy.init(&emitter, {"signal1", "signal2"});

    emitter.emitSignal1();
    emitter.emitSignal2();

    spy.clearSignal("signal1");

    QCOMPARE_EQ(spy.count("signal1"), 0);
    QCOMPARE_EQ(spy.count("signal2"), 1);
}

void MultiSignalSpyTest::_testClearAllSignals()
{
    TestEmitter emitter;
    MultiSignalSpy spy;
    spy.init(&emitter, {"signal1", "signal2"});

    emitter.emitSignal1();
    emitter.emitSignal2();

    spy.clearAllSignals();

    QCOMPARE_EQ(spy.count("signal1"), 0);
    QCOMPARE_EQ(spy.count("signal2"), 0);
}

void MultiSignalSpyTest::_testClearSignalsByMask()
{
    TestEmitter emitter;
    MultiSignalSpy spy;
    spy.init(&emitter, {"signal1", "signal2", "signal3"});

    emitter.emitSignal1();
    emitter.emitSignal2();
    emitter.emitSignal3();

    spy.clearSignalsByMask(spy.mask("signal1", "signal2"));

    QCOMPARE_EQ(spy.count("signal1"), 0);
    QCOMPARE_EQ(spy.count("signal2"), 0);
    QCOMPARE_EQ(spy.count("signal3"), 1);
}

// ============================================================================
// Wait Tests
// ============================================================================

void MultiSignalSpyTest::_testWaitForSignalAlreadyEmitted()
{
    TestEmitter emitter;
    MultiSignalSpy spy;
    spy.init(&emitter, {"signal1"});

    // Emit first, then wait should return immediately
    emitter.emitSignal1();
    QVERIFY(spy.waitForSignal("signal1", 100));
}

// ============================================================================
// Access Tests
// ============================================================================

void MultiSignalSpyTest::_testSpyForKnownSignal()
{
    TestEmitter emitter;
    MultiSignalSpy spy;
    spy.init(&emitter, {"signal1"});

    QSignalSpy *s = spy.spy("signal1");
    VERIFY_NOT_NULL(s);
    QVERIFY(s->isValid());
}

void MultiSignalSpyTest::_testSpyForUnknownSignal()
{
    TestEmitter emitter;
    MultiSignalSpy spy;
    spy.init(&emitter, {"signal1"});

    QSignalSpy *s = spy.spy("nonExistent");
    QVERIFY(s == nullptr);
}

void MultiSignalSpyTest::_testCountForSignal()
{
    TestEmitter emitter;
    MultiSignalSpy spy;
    spy.init(&emitter, {"signal1"});

    QCOMPARE_EQ(spy.count("signal1"), 0);

    emitter.emitSignal1();
    QCOMPARE_EQ(spy.count("signal1"), 1);

    emitter.emitSignal1();
    emitter.emitSignal1();
    QCOMPARE_EQ(spy.count("signal1"), 3);
}

void MultiSignalSpyTest::_testSignalNames()
{
    TestEmitter emitter;
    MultiSignalSpy spy;
    spy.init(&emitter, {"signal1", "signal2"});

    QStringList names = spy.signalNames();
    QCOMPARE_EQ(names.size(), 2);
    QVERIFY(names.contains("signal1"));
    QVERIFY(names.contains("signal2"));
}

void MultiSignalSpyTest::_testSignalCount()
{
    TestEmitter emitter;
    MultiSignalSpy spy;
    spy.init(&emitter, {"signal1", "signal2", "signal3"});

    QCOMPARE_EQ(spy.signalCount(), 3);
}

void MultiSignalSpyTest::_testHasSignal()
{
    TestEmitter emitter;
    MultiSignalSpy spy;
    spy.init(&emitter, {"signal1"});

    QVERIFY(spy.hasSignal("signal1"));
    QVERIFY(!spy.hasSignal("signal2"));
}

// ============================================================================
// Summary Tests
// ============================================================================

void MultiSignalSpyTest::_testSummaryNoSignals()
{
    TestEmitter emitter;
    MultiSignalSpy spy;
    spy.init(&emitter, {"signal1"});

    QString summary = spy.summary();
    QCOMPARE(summary, QStringLiteral("(no signals)"));
}

void MultiSignalSpyTest::_testSummaryWithSignals()
{
    TestEmitter emitter;
    MultiSignalSpy spy;
    spy.init(&emitter, {"signal1"});

    emitter.emitSignal1();
    emitter.emitSignal1();

    QString summary = spy.summary();
    QVERIFY(summary.contains("signal1=2"));
}

void MultiSignalSpyTest::_testTotalEmissions()
{
    TestEmitter emitter;
    MultiSignalSpy spy;
    spy.init(&emitter, {"signal1", "signal2"});

    QCOMPARE_EQ(spy.totalEmissions(), 0);

    emitter.emitSignal1();
    emitter.emitSignal2();
    emitter.emitSignal1();

    QCOMPARE_EQ(spy.totalEmissions(), 3);
}

void MultiSignalSpyTest::_testUniqueSignalsEmitted()
{
    TestEmitter emitter;
    MultiSignalSpy spy;
    spy.init(&emitter, {"signal1", "signal2", "signal3"});

    QCOMPARE_EQ(spy.uniqueSignalsEmitted(), 0);

    emitter.emitSignal1();
    emitter.emitSignal1();
    QCOMPARE_EQ(spy.uniqueSignalsEmitted(), 1);

    emitter.emitSignal2();
    QCOMPARE_EQ(spy.uniqueSignalsEmitted(), 2);
}

// ============================================================================
// Expectation API Tests
// ============================================================================

void MultiSignalSpyTest::_testExpectOnce()
{
    TestEmitter emitter;
    MultiSignalSpy spy;
    spy.init(&emitter, {"signal1"});

    QVERIFY(!spy.expect("signal1").once());

    emitter.emitSignal1();
    QVERIFY(spy.expect("signal1").once());

    emitter.emitSignal1();
    QVERIFY(!spy.expect("signal1").once());
}

void MultiSignalSpyTest::_testExpectAtLeastOnce()
{
    TestEmitter emitter;
    MultiSignalSpy spy;
    spy.init(&emitter, {"signal1"});

    QVERIFY(!spy.expect("signal1").atLeastOnce());

    emitter.emitSignal1();
    QVERIFY(spy.expect("signal1").atLeastOnce());

    emitter.emitSignal1();
    QVERIFY(spy.expect("signal1").atLeastOnce());
}

void MultiSignalSpyTest::_testExpectNever()
{
    TestEmitter emitter;
    MultiSignalSpy spy;
    spy.init(&emitter, {"signal1"});

    QVERIFY(spy.expect("signal1").never());

    emitter.emitSignal1();
    QVERIFY(!spy.expect("signal1").never());
}

void MultiSignalSpyTest::_testExpectTimes()
{
    TestEmitter emitter;
    MultiSignalSpy spy;
    spy.init(&emitter, {"signal1"});

    QVERIFY(spy.expect("signal1").times(0));
    QVERIFY(!spy.expect("signal1").times(1));

    emitter.emitSignal1();
    emitter.emitSignal1();
    emitter.emitSignal1();

    QVERIFY(spy.expect("signal1").times(3));
    QVERIFY(!spy.expect("signal1").times(2));
}

// ============================================================================
// Argument Extraction Tests
// ============================================================================

void MultiSignalSpyTest::_testArgumentExtraction()
{
    TestEmitter emitter;
    MultiSignalSpy spy;
    spy.init(&emitter, {"valueChanged"});

    emitter.emitValueChanged(42);

    int value = spy.argument<int>("valueChanged");
    QCOMPARE_EQ(value, 42);
}

void MultiSignalSpyTest::_testPullBoolFromSignal()
{
    TestEmitter emitter;
    MultiSignalSpy spy;
    spy.init(&emitter, {"boolChanged"});

    emitter.emitBoolChanged(true);

    bool value = spy.pullBoolFromSignal("boolChanged");
    QVERIFY(value);
}

void MultiSignalSpyTest::_testPullIntFromSignal()
{
    TestEmitter emitter;
    MultiSignalSpy spy;
    spy.init(&emitter, {"valueChanged"});

    emitter.emitValueChanged(123);

    int value = spy.pullIntFromSignal("valueChanged");
    QCOMPARE_EQ(value, 123);
}

#include "MultiSignalSpyTest.moc"
