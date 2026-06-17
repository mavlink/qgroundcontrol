#pragma once

#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QStringList>
#include <QtTest/QSignalSpy>

#include <chrono>
#include <memory>
#include <vector>

#include "UnitTest.h"

Q_DECLARE_LOGGING_CATEGORY(MultiSignalSpyLog)

/// Multi-signal spy for unit testing.
///
/// Monitors multiple signals on a QObject and provides methods to check
/// signal counts, wait for signals, and extract arguments.
///
/// Usage (automatic discovery):
///     MultiSignalSpy spy;
///     spy.init(myObject);
///     // ... trigger action ...
///     QVERIFY(spy.onlyEmittedOnce("valueChanged"));
///
/// Usage (specific signals):
///     MultiSignalSpy spy;
///     spy.init(myObject, {"valueChanged", "errorOccurred"});

class MultiSignalSpy : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(MultiSignalSpy)

public:
    static constexpr int kMaxSignals = 64;

    explicit MultiSignalSpy(QObject* parent = nullptr);
    ~MultiSignalSpy() override;

    bool isValid() const
    {
        return _signalEmitter && !_spies.empty();
    }

    /// Initialize with automatic signal discovery
    bool init(QObject* signalEmitter);

    /// Initialize with specific signals
    bool init(QObject* signalEmitter, const QStringList& signalNames);

    // ------------------------------------------------------------------------
    // Check methods
    // ------------------------------------------------------------------------

    /// Each named signal emitted exactly once
    bool emittedOnce(const QList<const char*>& signalNames) const;

    /// Each named signal emitted at least once
    bool emitted(const QList<const char*>& signalNames) const;

    /// Each named signal emitted exactly once AND no other monitored signal fired
    bool onlyEmittedOnce(const QList<const char*>& signalNames) const;

    /// Each named signal emitted at least once AND no other monitored signal fired
    bool onlyEmitted(const QList<const char*>& signalNames) const;

    /// None of the named signals were emitted
    bool notEmitted(const QList<const char*>& signalNames) const;

    /// No signals emitted
    bool noneEmitted() const;

    template <typename... Args>
    bool emittedOnce(const char* first, Args... rest) const
    {
        return emittedOnce(QList<const char*>{first, rest...});
    }

    template <typename... Args>
    bool emitted(const char* first, Args... rest) const
    {
        return emitted(QList<const char*>{first, rest...});
    }

    template <typename... Args>
    bool onlyEmittedOnce(const char* first, Args... rest) const
    {
        return onlyEmittedOnce(QList<const char*>{first, rest...});
    }

    template <typename... Args>
    bool onlyEmitted(const char* first, Args... rest) const
    {
        return onlyEmitted(QList<const char*>{first, rest...});
    }

    template <typename... Args>
    bool notEmitted(const char* first, Args... rest) const
    {
        return notEmitted(QList<const char*>{first, rest...});
    }

    // ------------------------------------------------------------------------
    // Clear methods
    // ------------------------------------------------------------------------

    void clearSignal(const char* signalName);
    void clearAllSignals();

    // ------------------------------------------------------------------------
    // Wait methods
    // ------------------------------------------------------------------------

    bool waitForSignal(const char* signalName, std::chrono::milliseconds timeout);

    bool waitForSignal(const char* signalName, int msec = TestTimeout::mediumMs())
    {
        return waitForSignal(signalName, std::chrono::milliseconds(msec));
    }

    // ------------------------------------------------------------------------
    // Access methods
    // ------------------------------------------------------------------------

    QSignalSpy* spy(const char* signalName) const;
    int count(const char* signalName) const;

    QStringList signalNames() const
    {
        return _signalNames;
    }

    qsizetype signalCount() const
    {
        return _signalNames.size();
    }

    bool hasSignal(const char* signalName) const;

    // ------------------------------------------------------------------------
    // Argument extraction
    // ------------------------------------------------------------------------

    template <typename T>
    T argument(const char* signalName, int emission = 0, int argIndex = 0) const
    {
        QSignalSpy* s = spy(signalName);
        if (!s || emission >= s->count()) {
            return T{};
        }
        return s->at(emission).at(argIndex).value<T>();
    }

    // ------------------------------------------------------------------------
    // Reporting
    // ------------------------------------------------------------------------

    QString summary() const;
    int totalEmissions() const;
    int uniqueSignalsEmitted() const;
    void printState() const;

    // ------------------------------------------------------------------------
    // Fluent API
    // ------------------------------------------------------------------------

    class Expectation
    {
    public:
        Expectation(MultiSignalSpy& spy, const char* signalName)
            : _spy(spy), _signalName(signalName)
        {
        }

        bool once() const
        {
            return _spy.emittedOnce(_signalName);
        }

        bool atLeastOnce() const
        {
            return _spy.emitted(_signalName);
        }

        bool times(int n) const
        {
            return _spy.count(_signalName) == n;
        }

        bool never() const
        {
            return _spy.notEmitted(_signalName);
        }

        bool onlyOnce() const
        {
            return _spy.onlyEmittedOnce(_signalName);
        }

        bool wait(std::chrono::milliseconds timeout) const
        {
            return _spy.waitForSignal(_signalName, timeout);
        }

        bool wait(int msec = TestTimeout::mediumMs()) const
        {
            return _spy.waitForSignal(_signalName, msec);
        }

        void clear()
        {
            _spy.clearSignal(_signalName);
        }

    private:
        MultiSignalSpy& _spy;
        const char* _signalName;
    };

    Expectation expect(const char* signalName)
    {
        return Expectation(*this, signalName);
    }

private slots:
    void _onEmitterDestroyed();

private:
    int _indexForSignal(const char* signalName) const;
    QList<int> _indicesForSignals(const QList<const char*>& signalNames) const;
    bool _emittedWorker(const QList<int>& indices, bool multipleAllowed) const;
    bool _onlyEmittedWorker(const QList<int>& indices, bool multipleAllowed) const;
    void _cleanup();

    QObject* _signalEmitter = nullptr;
    QStringList _signalNames;
    std::vector<std::unique_ptr<QSignalSpy>> _spies;
    QHash<QString, int> _nameToIndex;
};

// ============================================================================
// Verification Macros
// ============================================================================

#define QVERIFY_SIGNAL(spy, signalName)     \
    QVERIFY2((spy).emittedOnce(signalName), \
             qPrintable(QStringLiteral("Signal '%1' not emitted exactly once. %2").arg(signalName, (spy).summary())))

#define QVERIFY_SIGNAL_EMITTED(spy, signalName) \
    QVERIFY2((spy).emitted(signalName),         \
             qPrintable(QStringLiteral("Signal '%1' not emitted. %2").arg(signalName, (spy).summary())))

#define QVERIFY_ONLY_SIGNAL(spy, signalName)    \
    QVERIFY2((spy).onlyEmittedOnce(signalName), \
             qPrintable(QStringLiteral("Expected only '%1'. %2").arg(signalName, (spy).summary())))

#define QVERIFY_NO_SIGNAL(spy, signalName) \
    QVERIFY2((spy).notEmitted(signalName), \
             qPrintable(QStringLiteral("Signal '%1' unexpectedly emitted. %2").arg(signalName, (spy).summary())))

#define QVERIFY_NO_SIGNALS(spy) \
    QVERIFY2((spy).noneEmitted(), qPrintable(QStringLiteral("Expected no signals: %1").arg((spy).summary())))

#define QVERIFY_SIGNAL_COUNT(spy, signalName, expectedCount) QCOMPARE((spy).count(signalName), expectedCount)

#define QVERIFY_WAIT_SIGNAL(spy, signalName, timeout)  \
    QVERIFY2((spy).waitForSignal(signalName, timeout), \
             qPrintable(QStringLiteral("Timeout waiting for '%1'").arg(signalName)))
