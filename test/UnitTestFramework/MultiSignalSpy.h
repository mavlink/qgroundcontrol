#pragma once

#include <QtCore/QHash>
#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QStringList>
#include <QtPositioning/QGeoCoordinate>
#include <QtTest/QSignalSpy>

#include <memory>
#include <span>
#include <vector>

Q_DECLARE_LOGGING_CATEGORY(MultiSignalSpyLog)

/// @file
/// @brief Multi-signal spy for unit testing. Monitors multiple signals on a QObject
///        and provides methods to check signal counts, wait for signals, and extract arguments.
///
/// Usage (automatic discovery - recommended):
///     MultiSignalSpy spy;
///     spy.init(myObject);
///     // ... trigger some action ...
///     QVERIFY(spy.checkOnlySignal("valueChanged"));
///
/// Usage (specific signals only):
///     MultiSignalSpy spy;
///     spy.init(myObject, {"valueChanged", "errorOccurred"});
///
/// Usage with QtTestExtensions macros:
///     QVERIFY_SIGNAL(spy, "valueChanged");
///     QVERIFY_ONLY_SIGNAL(spy, "valueChanged");
///     QVERIFY_NO_SIGNALS(spy);

class MultiSignalSpy : public QObject
{
    Q_OBJECT

public:
    /// Maximum number of signals that can be monitored (limited by quint64 bitmask)
    static constexpr int kMaxSignals = 64;

    explicit MultiSignalSpy(QObject* parent = nullptr);
    ~MultiSignalSpy() override;

    /// Returns true if the spy is valid and the emitter is still alive
    bool isValid() const { return _signalEmitter != nullptr && !_spies.empty(); }

    /// Initialize with automatic signal discovery (recommended)
    /// Discovers all signals declared on the object (excludes inherited QObject signals)
    /// @param signalEmitter Object to monitor
    /// @return true on success
    bool init(QObject* signalEmitter);

    /// Initialize with specific signals only
    /// @param signalEmitter Object to monitor
    /// @param signalNames List of signal names (without SIGNAL() macro, just "valueChanged")
    /// @return true on success
    bool init(QObject* signalEmitter, const QStringList& signalNames);

    // ========================================================================
    // Signal name to mask conversion
    // ========================================================================

    /// Get bitmask for a signal by name
    /// @return Bitmask with bit set for this signal, or 0 if not found
    quint64 mask(const char* signalName) const;

    /// Combine masks for multiple signals
    /// @return Combined bitmask
    template<typename... Args>
    quint64 mask(const char* first, Args... rest) const {
        return mask(first) | mask(rest...);
    }

    // ========================================================================
    // Check methods - verify signal emission counts
    // ========================================================================

    /// Check signal was emitted exactly once
    bool checkSignal(const char* signalName) const;

    /// Check signal was emitted at least once
    bool checkSignals(const char* signalName) const;

    /// Check signal was emitted exactly once AND no other signals fired
    bool checkOnlySignal(const char* signalName) const;

    /// Check signal was emitted at least once AND no other signals fired
    bool checkOnlySignals(const char* signalName) const;

    /// Check signal was not emitted
    bool checkNoSignal(const char* signalName) const;

    /// Check no signals were emitted
    bool checkNoSignals() const;

    // Mask-based versions (for checking multiple signals at once)
    bool checkSignalByMask(quint64 signalMask) const;
    bool checkSignalsByMask(quint64 signalMask) const;
    bool checkOnlySignalByMask(quint64 signalMask) const;
    bool checkOnlySignalsByMask(quint64 signalMask) const;
    bool checkNoSignalByMask(quint64 signalMask) const;

    // ========================================================================
    // Clear methods - reset signal counts
    // ========================================================================

    void clearSignal(const char* signalName);
    void clearSignalsByMask(quint64 signalMask);
    void clearAllSignals();

    // ========================================================================
    // Wait methods - block until signal is emitted
    // ========================================================================

    /// Wait for signal to be emitted
    /// @param signalName Signal to wait for
    /// @param msec Timeout in milliseconds
    /// @return true if signal was emitted before timeout
    bool waitForSignal(const char* signalName, int msec = 5000);

    // ========================================================================
    // Access methods - get spy and signal data
    // ========================================================================

    /// Get the QSignalSpy for a signal
    QSignalSpy* spy(const char* signalName) const;

    /// Get emission count for a signal
    int count(const char* signalName) const;

    /// Get list of all monitored signal names
    QStringList signalNames() const { return _signalNames; }

    // ========================================================================
    // Argument extraction - get signal parameters
    // ========================================================================

    /// Get argument from signal emission
    /// @param signalName Signal name
    /// @param emission Which emission (0 = first)
    /// @param argIndex Which argument (0 = first)
    template<typename T>
    T argument(const char* signalName, int emission = 0, int argIndex = 0) const {
        QSignalSpy* s = spy(signalName);
        if (!s || emission >= s->count()) {
            return T{};
        }
        return s->at(emission).at(argIndex).value<T>();
    }

    // Convenience methods for common types
    bool pullBoolFromSignal(const char* signalName);
    int pullIntFromSignal(const char* signalName);
    QGeoCoordinate pullQGeoCoordinateFromSignal(const char* signalName);

    // ========================================================================
    // Debug
    // ========================================================================

    /// Print current state of all signals to debug output
    void printState(quint64 expectedMask = 0) const;

    // ========================================================================
    // Summary and reporting
    // ========================================================================

    /// Returns a summary string of all signal emissions
    QString summary() const
    {
        QStringList parts;
        for (int i = 0; i < static_cast<int>(_spies.size()); ++i) {
            const int c = _spies[i]->count();
            if (c > 0) {
                parts.append(QStringLiteral("%1=%2").arg(_signalNames[i]).arg(c));
            }
        }
        return parts.isEmpty() ? QStringLiteral("(no signals)") : parts.join(QStringLiteral(", "));
    }

    /// Returns total number of signal emissions across all signals
    int totalEmissions() const
    {
        int total = 0;
        for (const auto& spy : _spies) {
            total += spy->count();
        }
        return total;
    }

    /// Returns the number of different signals that were emitted
    int uniqueSignalsEmitted() const
    {
        int count = 0;
        for (const auto& spy : _spies) {
            if (spy->count() > 0) {
                ++count;
            }
        }
        return count;
    }

    // ========================================================================
    // Fluent expectation API (optional alternative syntax)
    // ========================================================================

    /// Fluent API: Expect exactly one emission of a signal
    /// Usage: QVERIFY(spy.expect("valueChanged").once());
    class Expectation
    {
    public:
        Expectation(const MultiSignalSpy& spy, const char* signalName)
            : _spy(spy), _signalName(signalName), _mask(spy.mask(signalName)) {}

        /// Expect exactly one emission
        bool once() const { return _spy.checkSignalByMask(_mask); }

        /// Expect at least one emission
        bool atLeastOnce() const { return _spy.checkSignalsByMask(_mask); }

        /// Expect exactly N emissions
        bool times(int n) const { return _spy.count(_signalName) == n; }

        /// Expect no emissions
        bool never() const { return _spy.checkNoSignalByMask(_mask); }

        /// Expect this was the only signal emitted (exactly once)
        bool onlyOnce() const { return _spy.checkOnlySignalByMask(_mask); }

        /// Expect this was the only signal emitted (at least once)
        bool onlyAtLeastOnce() const { return _spy.checkOnlySignalsByMask(_mask); }

    private:
        const MultiSignalSpy& _spy;
        const char* _signalName;
        quint64 _mask;
    };

    /// Start a fluent expectation for a signal
    Expectation expect(const char* signalName) const { return Expectation(*this, signalName); }

    // ========================================================================
    // Modern C++20 accessors
    // ========================================================================

    /// Returns a span view of all signal names (read-only)
    std::span<const QString> signalNamesView() const
    {
        return std::span<const QString>(_signalNames.constBegin(), _signalNames.size());
    }

    /// Returns the number of monitored signals
    qsizetype signalCount() const { return _signalNames.size(); }

    /// Check if a signal is being monitored
    bool hasSignal(const char* signalName) const
    {
        return _nameToIndex.contains(QString::fromLatin1(signalName));
    }

private slots:
    void _onEmitterDestroyed();

private:
    int _indexForSignal(const char* signalName) const;
    bool _checkSignalByMaskWorker(quint64 signalMask, bool multipleAllowed) const;
    bool _checkOnlySignalByMaskWorker(quint64 signalMask, bool multipleAllowed) const;
    void _cleanup();

    QObject* _signalEmitter = nullptr;
    QStringList _signalNames;
    std::vector<std::unique_ptr<QSignalSpy>> _spies;
    QHash<QString, int> _nameToIndex;
};

// ============================================================================
// MultiSignalSpy Integration Macros
// ============================================================================

/// Verify a signal was emitted exactly once
#define QVERIFY_SIGNAL(spy, signalName) \
    QVERIFY2((spy).checkSignal(signalName), \
             qPrintable(QStringLiteral("Signal '%1' was not emitted exactly once. %2") \
                       .arg(signalName, (spy).summary())))

/// Verify a signal was emitted at least once
#define QVERIFY_SIGNAL_EMITTED(spy, signalName) \
    QVERIFY2((spy).checkSignals(signalName), \
             qPrintable(QStringLiteral("Signal '%1' was not emitted. %2") \
                       .arg(signalName, (spy).summary())))

/// Verify a signal was emitted exactly once AND no other signals fired
#define QVERIFY_ONLY_SIGNAL(spy, signalName) \
    QVERIFY2((spy).checkOnlySignal(signalName), \
             qPrintable(QStringLiteral("Expected only '%1' to be emitted. %2") \
                       .arg(signalName, (spy).summary())))

/// Verify a signal was NOT emitted
#define QVERIFY_NO_SIGNAL(spy, signalName) \
    QVERIFY2((spy).checkNoSignal(signalName), \
             qPrintable(QStringLiteral("Signal '%1' was unexpectedly emitted. %2") \
                       .arg(signalName, (spy).summary())))

/// Verify no signals were emitted
#define QVERIFY_NO_SIGNALS(spy) \
    QVERIFY2((spy).checkNoSignals(), \
             qPrintable(QStringLiteral("Expected no signals but got: %1").arg((spy).summary())))

/// Verify signal emission count
#define QVERIFY_SIGNAL_COUNT(spy, signalName, expectedCount) \
    QCOMPARE((spy).count(signalName), expectedCount)

/// Wait for a signal with verification
#define QVERIFY_WAIT_SIGNAL(spy, signalName, timeout) \
    QVERIFY2((spy).waitForSignal(signalName, timeout), \
             qPrintable(QStringLiteral("Timeout waiting for signal '%1'").arg(signalName)))
