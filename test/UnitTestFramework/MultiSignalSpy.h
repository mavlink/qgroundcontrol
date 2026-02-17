#pragma once

#include <QtCore/QHash>
#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QStringList>
#include <QtTest/QSignalSpy>

#include <memory>
#include <vector>

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
    // Mask operations
    // ------------------------------------------------------------------------

    quint64 mask(const char* signalName) const;

    template <typename... Args>
    quint64 mask(const char* first, Args... rest) const
    {
        return mask(first) | mask(rest...);
    }

    // ------------------------------------------------------------------------
    // Check methods (string-based API)
    // ------------------------------------------------------------------------

    /// Signal emitted exactly once
    bool emittedOnce(const char* signalName) const;

    /// Signal emitted at least once
    bool emitted(const char* signalName) const;

    /// Signal emitted exactly once AND no other signals fired
    bool onlyEmittedOnce(const char* signalName) const;

    /// Signal emitted at least once AND no other signals fired
    bool onlyEmitted(const char* signalName) const;

    /// Signal was not emitted
    bool notEmitted(const char* signalName) const;

    /// No signals emitted
    bool noneEmitted() const;

    // Mask-based versions
    bool emittedOnceByMask(quint64 signalMask) const;
    bool emittedByMask(quint64 signalMask) const;
    bool onlyEmittedOnceByMask(quint64 signalMask) const;
    bool onlyEmittedByMask(quint64 signalMask) const;
    bool notEmittedByMask(quint64 signalMask) const;

    // ------------------------------------------------------------------------
    // Clear methods
    // ------------------------------------------------------------------------

    void clearSignal(const char* signalName);
    void clearSignalsByMask(quint64 signalMask);
    void clearAllSignals();

    // ------------------------------------------------------------------------
    // Wait methods
    // ------------------------------------------------------------------------

    bool waitForSignal(const char* signalName, int msec = 5000);

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
    void printState(quint64 expectedMask = 0) const;

    // ------------------------------------------------------------------------
    // Fluent API
    // ------------------------------------------------------------------------

    class Expectation
    {
    public:
        Expectation(MultiSignalSpy& spy, const char* signalName)
            : _spy(spy), _signalName(signalName), _mask(spy.mask(signalName))
        {
        }

        bool once() const
        {
            return _spy.emittedOnceByMask(_mask);
        }

        bool atLeastOnce() const
        {
            return _spy.emittedByMask(_mask);
        }

        bool times(int n) const
        {
            return _spy.count(_signalName) == n;
        }

        bool never() const
        {
            return _spy.notEmittedByMask(_mask);
        }

        bool onlyOnce() const
        {
            return _spy.onlyEmittedOnceByMask(_mask);
        }

        bool wait(int msec = 5000) const
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
        quint64 _mask;
    };

    Expectation expect(const char* signalName)
    {
        return Expectation(*this, signalName);
    }

private slots:
    void _onEmitterDestroyed();

private:
    int _indexForSignal(const char* signalName) const;
    bool _emittedOnceByMaskWorker(quint64 signalMask, bool multipleAllowed) const;
    bool _onlyEmittedOnceByMaskWorker(quint64 signalMask, bool multipleAllowed) const;
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
