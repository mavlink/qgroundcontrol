#pragma once

#include <QtTest/QSignalSpy>

#include <memory>
#include <vector>

#include "MultiSignalSpy.h"
#include "UnitTest.h"

/// Test fixture with helper methods for signal spy operations.
///
/// Provides convenience methods for creating and managing signal spies,
/// reducing boilerplate in tests that verify signal emissions.
///
/// Example:
/// @code
/// class MySignalTest : public SignalSpyTest
/// {
///     Q_OBJECT
/// private slots:
///     void _testSignals() {
///         MyObject obj;
///         auto spy = createMultiSpy(&obj);
///
///         obj.doSomething();
///
///         QVERIFY(spy->emittedOnce("valueChanged"));
///         QVERIFY(spy->notEmitted("errorOccurred"));
///     }
/// };
/// @endcode
class SignalSpyTest : public UnitTest
{
    Q_OBJECT

public:
    explicit SignalSpyTest(QObject* parent = nullptr) : UnitTest(parent) {}

protected:
    /// Creates a MultiSignalSpy for the given object with automatic signal discovery
    /// The spy is owned by the test and cleaned up automatically
    MultiSignalSpy* createMultiSpy(QObject* target)
    {
        auto spy = new MultiSignalSpy(this);
        if (!spy->init(target)) {
            delete spy;
            return nullptr;
        }
        _spies.push_back(spy);
        return spy;
    }

    /// Creates a MultiSignalSpy for specific signals
    MultiSignalSpy* createMultiSpy(QObject* target, const QStringList& signalNames)
    {
        auto spy = new MultiSignalSpy(this);
        if (!spy->init(target, signalNames)) {
            delete spy;
            return nullptr;
        }
        _spies.push_back(spy);
        return spy;
    }

    /// Creates a QSignalSpy for a specific signal
    /// @param sender The object emitting the signal
    /// @param signal The signal (use SIGNAL() macro)
    template <typename Func>
    QSignalSpy* createSpy(const typename QtPrivate::FunctionPointer<Func>::Object* sender, Func signal)
    {
        auto spy = new QSignalSpy(sender, signal);
        _qspies.emplace_back(spy);
        return spy;
    }

    /// Clears all managed spies
    void clearAllSpies()
    {
        for (auto* spy : _spies) {
            spy->clearAllSignals();
        }
        for (auto& spy : _qspies) {
            spy->clear();
        }
    }

    /// Verifies that a signal was emitted exactly once on a MultiSignalSpy
    bool verifySignalEmitted(MultiSignalSpy* spy, const char* signalName)
    {
        if (!spy) {
            return false;
        }
        return spy->emittedOnce(signalName);
    }

    /// Verifies that only the specified signal was emitted (no others)
    bool verifyOnlySignalEmitted(MultiSignalSpy* spy, const char* signalName)
    {
        if (!spy) {
            return false;
        }
        return spy->onlyEmittedOnce(signalName);
    }

    /// Waits for a signal with timeout
    bool waitForSignal(MultiSignalSpy* spy, const char* signalName, int timeoutMs = 5000)
    {
        if (!spy) {
            return false;
        }
        return spy->waitForSignal(signalName, timeoutMs);
    }

protected slots:
    void cleanup() override
    {
        _spies.clear();
        _qspies.clear();
        UnitTest::cleanup();
    }

private:
    std::vector<MultiSignalSpy*> _spies;  // Owned by this (as QObject children)
    std::vector<std::unique_ptr<QSignalSpy>> _qspies;
};
