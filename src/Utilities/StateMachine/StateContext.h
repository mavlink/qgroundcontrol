#pragma once

#include <QtCore/QHash>
#include <QtCore/QString>
#include <QtCore/QVariant>

#include <any>
#include <optional>
#include <typeinfo>

/// Type-safe context for passing data between states in a state machine.
///
/// StateContext provides a way for states to share data without tight coupling.
/// Data is stored by key and can be retrieved with type checking.
///
/// Example usage:
/// @code
/// // In producer state
/// context->set("userId", 42);
/// context->set("userName", QString("Alice"));
///
/// // In consumer state
/// auto userId = context->get<int>("userId");
/// if (userId) {
///     qDebug() << "User ID:" << *userId;
/// }
/// @endcode
///
/// The context can also be accessed via QGCStateMachine::context().
class StateContext
{
public:
    StateContext() = default;
    ~StateContext() = default;

    // -------------------------------------------------------------------------
    // Type-safe API (preferred)
    // -------------------------------------------------------------------------

    /// Store a value with the given key
    /// @param key Unique identifier for the value
    /// @param value The value to store
    template<typename T>
    void set(const QString& key, T&& value)
    {
        _data[key] = std::any(std::forward<T>(value));
    }

    /// Retrieve a value by key with type checking
    /// @param key The key to look up
    /// @return The value if found and type matches, std::nullopt otherwise
    template<typename T>
    std::optional<T> get(const QString& key) const
    {
        auto it = _data.find(key);
        if (it == _data.end()) {
            return std::nullopt;
        }

        try {
            return std::any_cast<T>(it.value());
        } catch (const std::bad_any_cast&) {
            return std::nullopt;
        }
    }

    /// Retrieve a value by key, returning a default if not found
    /// @param key The key to look up
    /// @param defaultValue Value to return if key not found or type mismatch
    /// @return The stored value or defaultValue
    template<typename T>
    T getOr(const QString& key, const T& defaultValue) const
    {
        auto result = get<T>(key);
        return result.value_or(defaultValue);
    }

    /// Check if a key exists in the context
    /// @param key The key to check
    /// @return true if the key exists
    bool contains(const QString& key) const
    {
        return _data.contains(key);
    }

    /// Check if a key exists and holds the specified type
    /// @param key The key to check
    /// @return true if key exists and holds type T
    template<typename T>
    bool containsType(const QString& key) const
    {
        auto it = _data.find(key);
        if (it == _data.end()) {
            return false;
        }
        return it.value().type() == typeid(T);
    }

    /// Remove a value by key
    /// @param key The key to remove
    /// @return true if the key was found and removed
    bool remove(const QString& key)
    {
        return _data.remove(key) > 0;
    }

    /// Clear all stored values
    void clear()
    {
        _data.clear();
    }

    /// Get the number of stored values
    int count() const
    {
        return _data.count();
    }

    /// Get all keys in the context
    QStringList keys() const
    {
        return _data.keys();
    }

    // -------------------------------------------------------------------------
    // QVariant API (for QML compatibility)
    // -------------------------------------------------------------------------

    /// Store a QVariant value
    void setVariant(const QString& key, const QVariant& value)
    {
        _variantData[key] = value;
    }

    /// Retrieve a QVariant value
    QVariant variant(const QString& key) const
    {
        return _variantData.value(key);
    }

    /// Check if a variant key exists
    bool containsVariant(const QString& key) const
    {
        return _variantData.contains(key);
    }

private:
    QHash<QString, std::any> _data;
    QHash<QString, QVariant> _variantData;
};
