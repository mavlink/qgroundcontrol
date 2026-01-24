#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QRandomGenerator>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QTemporaryDir>
#include <QtCore/QTimer>
#include <QtCore/QUuid>
#include <QtPositioning/QGeoCoordinate>

#include <concepts>
#include <functional>
#include <optional>
#include <span>

/// @file
/// @brief Mock helpers for deterministic unit testing
///
/// Provides utilities for:
/// - TestDataGenerator: Random but reproducible test data generation
/// - DeferredAction: Schedule actions with delays for async testing
/// - TemporaryTestDirectory: RAII temporary directory management
/// - CallCounter: Track function/slot invocations
/// - ValueRecorder: Record values passed to callbacks
/// - MockClock: Control time for testing
/// - Expectation: Fluent expectation API

namespace MockHelpers {

// ============================================================================
// TestDataGenerator - Generate random but reproducible test data
// ============================================================================

/// Generates random test data with optional seeding for reproducibility
class TestDataGenerator
{
public:
    /// Constructor with optional seed for reproducibility
    explicit TestDataGenerator(quint32 seed = 0)
        : _rng(seed == 0 ? QRandomGenerator::global()->generate() : seed)
    {}

    /// Resets the generator with a new seed
    void seed(quint32 newSeed) { _rng.seed(newSeed); }

    // --- Numeric generators ---

    /// Generates a random integer in range [min, max]
    int randomInt(int min, int max)
    {
        return _rng.bounded(min, max + 1);
    }

    /// Generates a random double in range [min, max]
    double randomDouble(double min = 0.0, double max = 1.0)
    {
        return min + _rng.generateDouble() * (max - min);
    }

    /// Generates a random boolean with given probability of true
    bool randomBool(double probabilityTrue = 0.5)
    {
        return _rng.generateDouble() < probabilityTrue;
    }

    // --- String generators ---

    /// Generates a random string of given length
    QString randomString(int length, const QString& charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789")
    {
        QString result;
        result.reserve(length);
        for (int i = 0; i < length; ++i) {
            result.append(charset.at(_rng.bounded(charset.length())));
        }
        return result;
    }

    /// Generates a random file name with extension
    QString randomFileName(const QString& extension = "txt")
    {
        return QString("%1.%2").arg(randomString(8)).arg(extension);
    }

    /// Generates a UUID string
    QString randomUuid()
    {
        return QUuid::createUuid().toString(QUuid::WithoutBraces);
    }

    // --- Coordinate generators ---

    /// Generates a random valid latitude
    double randomLatitude()
    {
        return randomDouble(-90.0, 90.0);
    }

    /// Generates a random valid longitude
    double randomLongitude()
    {
        return randomDouble(-180.0, 180.0);
    }

    /// Generates a random altitude in meters
    double randomAltitude(double min = 0.0, double max = 1000.0)
    {
        return randomDouble(min, max);
    }

    /// Generates a random coordinate
    QGeoCoordinate randomCoordinate()
    {
        return QGeoCoordinate(randomLatitude(), randomLongitude());
    }

    /// Generates a random coordinate with altitude
    QGeoCoordinate randomCoordinate3D(double minAlt = 0.0, double maxAlt = 1000.0)
    {
        return QGeoCoordinate(randomLatitude(), randomLongitude(), randomAltitude(minAlt, maxAlt));
    }

    /// Generates a coordinate within distance of a center point
    QGeoCoordinate randomCoordinateNear(const QGeoCoordinate& center, double maxDistanceMeters)
    {
        const double distance = randomDouble(0, maxDistanceMeters);
        const double bearing = randomDouble(0, 360);
        return center.atDistanceAndAzimuth(distance, bearing);
    }

    /// Generates a list of random coordinates forming a polygon
    QList<QGeoCoordinate> randomPolygon(int numVertices, const QGeoCoordinate& center, double radiusMeters)
    {
        QList<QGeoCoordinate> coords;
        const double angleStep = 360.0 / numVertices;
        for (int i = 0; i < numVertices; ++i) {
            const double angle = i * angleStep + randomDouble(-10, 10);
            const double distance = radiusMeters * randomDouble(0.8, 1.2);
            coords.append(center.atDistanceAndAzimuth(distance, angle));
        }
        return coords;
    }

    // --- Byte array generators ---

    /// Generates random bytes
    QByteArray randomBytes(int length)
    {
        QByteArray data;
        data.resize(length);
        for (int i = 0; i < length; ++i) {
            data[i] = static_cast<char>(_rng.bounded(256));
        }
        return data;
    }

    // --- JSON generators ---

    /// Generates a simple random JSON object
    QJsonObject randomJsonObject(int numKeys = 5)
    {
        QJsonObject obj;
        for (int i = 0; i < numKeys; ++i) {
            const QString key = QString("key%1").arg(i);
            switch (_rng.bounded(4)) {
            case 0: obj[key] = randomInt(0, 100); break;
            case 1: obj[key] = randomDouble(); break;
            case 2: obj[key] = randomString(10); break;
            case 3: obj[key] = randomBool(); break;
            }
        }
        return obj;
    }

private:
    QRandomGenerator _rng;
};

// ============================================================================
// DeferredAction - Schedule actions with delays
// ============================================================================

/// Schedules an action to run after a delay
/// Useful for testing asynchronous code
class DeferredAction : public QObject
{
    Q_OBJECT

public:
    explicit DeferredAction(QObject* parent = nullptr) : QObject(parent) {}

    /// Schedules an action to run after delayMs milliseconds
    void schedule(int delayMs, std::function<void()> action)
    {
        _action = action;
        QTimer::singleShot(delayMs, this, &DeferredAction::_execute);
    }

private slots:
    void _execute()
    {
        if (_action) {
            _action();
        }
    }

private:
    std::function<void()> _action;
};

// ============================================================================
// TemporaryTestDirectory - RAII temp directory for tests
// ============================================================================

/// Creates a temporary directory that auto-cleans on destruction
class TemporaryTestDirectory
{
public:
    TemporaryTestDirectory() : _dir(new QTemporaryDir())
    {
        _valid = _dir->isValid();
    }

    ~TemporaryTestDirectory()
    {
        delete _dir;
    }

    bool isValid() const { return _valid; }
    QString path() const { return _dir->path(); }

    /// Creates a file in the temp directory with given content
    QString createFile(const QString& name, const QByteArray& content)
    {
        const QString filePath = _dir->filePath(name);
        QFile file(filePath);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(content);
            file.close();
            return filePath;
        }
        return QString();
    }

    /// Creates a text file in the temp directory
    QString createTextFile(const QString& name, const QString& content)
    {
        return createFile(name, content.toUtf8());
    }

    /// Creates a JSON file in the temp directory
    QString createJsonFile(const QString& name, const QJsonObject& obj)
    {
        return createFile(name, QJsonDocument(obj).toJson());
    }

    /// Creates a subdirectory in the temp directory
    QString createSubDir(const QString& name)
    {
        const QString dirPath = _dir->filePath(name);
        if (QDir().mkpath(dirPath)) {
            return dirPath;
        }
        return QString();
    }

    /// Returns the path to a file in the temp directory
    QString filePath(const QString& name) const
    {
        return _dir->filePath(name);
    }

private:
    QTemporaryDir* _dir;
    bool _valid = false;
};

// ============================================================================
// CallCounter - Track function/slot invocations
// ============================================================================

/// Counts how many times a function or slot is called
class CallCounter
{
public:
    CallCounter() = default;

    /// Increments the counter
    void operator()() { ++_count; }

    /// Returns current count
    int count() const { return _count; }

    /// Resets the counter
    void reset() { _count = 0; }

    /// Returns a lambda that increments this counter
    std::function<void()> lambda()
    {
        return [this]() { ++_count; };
    }

private:
    int _count = 0;
};

// ============================================================================
// ValueRecorder - Record values passed to callbacks
// ============================================================================

/// Records values passed to a callback for later verification
template<typename T>
class ValueRecorder
{
public:
    ValueRecorder() = default;

    /// Records a value
    void record(const T& value) { _values.append(value); }

    /// Returns all recorded values
    const QList<T>& values() const { return _values; }

    /// Returns the last recorded value
    T lastValue() const { return _values.isEmpty() ? T() : _values.last(); }

    /// Returns the value at index
    T valueAt(int index) const { return _values.value(index); }

    /// Returns the number of recorded values
    int count() const { return _values.size(); }

    /// Clears all recorded values
    void clear() { _values.clear(); }

    /// Returns a lambda that records values
    std::function<void(const T&)> lambda()
    {
        return [this](const T& value) { record(value); };
    }

private:
    QList<T> _values;
};

// ============================================================================
// MockClock - Control time for testing
// ============================================================================

/// A mock clock for testing time-dependent code
/// Note: This doesn't actually control Qt's timers, but can be used
/// for code that accepts a clock interface
class MockClock
{
public:
    MockClock() : _currentTime(0) {}

    /// Returns the current mock time
    qint64 currentMsecsSinceEpoch() const { return _currentTime; }

    /// Advances the clock by the given milliseconds
    void advance(qint64 msecs) { _currentTime += msecs; }

    /// Sets the current time
    void setTime(qint64 msecsSinceEpoch) { _currentTime = msecsSinceEpoch; }

    /// Resets to zero
    void reset() { _currentTime = 0; }

private:
    qint64 _currentTime;
};

// ============================================================================
// Expectation - Fluent expectation API
// ============================================================================

/// Fluent API for setting expectations in tests
template<typename T>
class Expectation
{
public:
    explicit Expectation(const T& value) : _value(value) {}

    /// Verifies value equals expected
    [[nodiscard]] bool equals(const T& expected) const { return _value == expected; }

    /// Verifies value is greater than expected
    [[nodiscard]] bool isGreaterThan(const T& other) const
        requires std::totally_ordered<T>
    { return _value > other; }

    /// Verifies value is less than expected
    [[nodiscard]] bool isLessThan(const T& other) const
        requires std::totally_ordered<T>
    { return _value < other; }

    /// Verifies value is in range [min, max]
    [[nodiscard]] bool isInRange(const T& min, const T& max) const
        requires std::totally_ordered<T>
    {
        return _value >= min && _value <= max;
    }

    /// Verifies value is approximately equal (for floating point)
    [[nodiscard]] bool isApproximately(const T& expected, const T& epsilon) const
        requires std::floating_point<T>
    {
        return std::abs(_value - expected) < epsilon;
    }

    /// Gets the underlying value
    [[nodiscard]] const T& value() const { return _value; }

private:
    T _value;
};

/// Creates an expectation for a value
template<typename T>
[[nodiscard]] Expectation<T> expect(const T& value)
{
    return Expectation<T>(value);
}

} // namespace MockHelpers
