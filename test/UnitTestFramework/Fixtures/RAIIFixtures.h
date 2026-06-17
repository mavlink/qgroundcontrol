#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonParseError>
#include <QtCore/QTemporaryDir>
#include <QtCore/QTemporaryFile>
#include <QtCore/QUrl>
#include <QtCore/QVariant>
#include <QtNetwork/QNetworkReply>

#include <memory>

#include "MAVLinkMessageType.h"

class RunGuard;
class Fact;

/// @file
/// @brief RAII wrappers for test resources

namespace TestFixtures {

// ============================================================================
// SettingsFixture - RAII wrapper for settings save/restore
// ============================================================================

/// RAII fixture that saves settings on construction and restores them on destruction
/// Prevents test settings from polluting subsequent tests
class SettingsFixture
{
public:
    /// Save current settings state
    SettingsFixture();

    /// Restore original settings
    ~SettingsFixture();

    // Non-copyable
    SettingsFixture(const SettingsFixture&) = delete;
    SettingsFixture& operator=(const SettingsFixture&) = delete;

    /// Set offline editing firmware type
    void setOfflineFirmware(MAV_AUTOPILOT autopilot);

    /// Set offline editing vehicle type
    void setOfflineVehicleType(MAV_TYPE vehicleType);

    /// Set a Fact value (will be restored on destruction)
    void setFactValue(Fact* fact, const QVariant& value);

private:
    struct SavedFact
    {
        Fact* fact;
        QVariant originalValue;
    };

    QList<SavedFact> _savedFacts;

    MAV_AUTOPILOT _originalFirmware;
    MAV_TYPE _originalVehicleType;
};

// ============================================================================
// NetworkReplyFixture - Lightweight fake QNetworkReply for helper tests
// ============================================================================

/// Fake QNetworkReply with configurable status/redirect/error/body data
class NetworkReplyFixture final : public QNetworkReply
{
public:
    explicit NetworkReplyFixture(const QUrl& url, QObject* parent = nullptr);
    ~NetworkReplyFixture() override;

    void setHttpStatus(int statusCode);
    void setRedirectTarget(const QUrl& target);
    void setNetworkError(QNetworkReply::NetworkError errorCode, const QString& message);
    void setContentType(const QString& contentType);
    void setContentLength(qint64 length);
    void setBody(const QByteArray& body, const QString& contentType = QString());

    void abort() override;

protected:
    qint64 readData(char* data, qint64 maxSize) override;

private:
    QByteArray _body;
    qint64 _readOffset = 0;
};

// ============================================================================
// SingleInstanceLockFixture - RAII wrapper for RunGuard lock ownership
// ============================================================================

class SingleInstanceLockFixture
{
public:
    explicit SingleInstanceLockFixture(const QString& lockKey, bool acquireOnCreate = true);
    ~SingleInstanceLockFixture();

    SingleInstanceLockFixture(const SingleInstanceLockFixture&) = delete;
    SingleInstanceLockFixture& operator=(const SingleInstanceLockFixture&) = delete;

    bool tryAcquire();
    void release();

    bool isLocked() const;
    QString key() const;

private:
    QString _lockKey;
    std::unique_ptr<RunGuard> _guard;
};

// ============================================================================
// TempFileFixture - RAII wrapper for temporary files
// ============================================================================

/// RAII fixture for temporary files that auto-delete on destruction
class TempFileFixture
{
public:
    /// Create a temporary file
    /// @param templateName Optional template (e.g., "test_XXXXXX.txt")
    explicit TempFileFixture(const QString& templateName = QString());

    ~TempFileFixture();

    // Non-copyable
    TempFileFixture(const TempFileFixture&) = delete;
    TempFileFixture& operator=(const TempFileFixture&) = delete;

    /// Check if file was created successfully
    bool isValid() const
    {
        return _file && _file->isOpen();
    }

    /// Get the file path
    QString path() const;

    /// Write content to the file
    bool write(const QByteArray& content);
    bool write(const QString& content);

    /// Read all content from the file
    QByteArray readAll();

    /// Get underlying QTemporaryFile
    QTemporaryFile* file() const
    {
        return _file.get();
    }

private:
    std::unique_ptr<QTemporaryFile> _file;
};

// ============================================================================
// TempJsonFileFixture - JSON-focused temp file helper
// ============================================================================

class TempJsonFileFixture
{
public:
    explicit TempJsonFileFixture(const QString& templateName = QStringLiteral("test_json_XXXXXX.json"));
    ~TempJsonFileFixture();

    TempJsonFileFixture(const TempJsonFileFixture&) = delete;
    TempJsonFileFixture& operator=(const TempJsonFileFixture&) = delete;

    bool isValid() const;
    QString path() const;

    bool writeJson(const QJsonDocument& json, bool compact = true);
    bool writeJson(const QJsonObject& object, bool compact = true);
    QJsonDocument readJson(QJsonParseError* error = nullptr);

private:
    TempFileFixture _file;
};

// ============================================================================
// EnvVarFixture - RAII guard for environment variable save/restore
// ============================================================================

/// RAII guard that saves an environment variable on construction and restores it on destruction.
class EnvVarFixture
{
public:
    explicit EnvVarFixture(const char* name)
        : _name(name)
        , _wasSet(qEnvironmentVariableIsSet(name))
    {
        if (_wasSet) {
            _value = qgetenv(name);
        }
    }

    ~EnvVarFixture()
    {
        if (!_name) return; // moved-from
        if (_wasSet) {
            qputenv(_name, _value);
        } else {
            qunsetenv(_name);
        }
    }

    EnvVarFixture(EnvVarFixture&& other) noexcept
        : _name(other._name)
        , _value(std::move(other._value))
        , _wasSet(other._wasSet)
    {
        other._name = nullptr;
    }

    EnvVarFixture(const EnvVarFixture&) = delete;
    EnvVarFixture& operator=(const EnvVarFixture&) = delete;
    EnvVarFixture& operator=(EnvVarFixture&&) = delete;

private:
    const char* _name;
    QByteArray _value;
    bool _wasSet;
};

// ============================================================================
// TempDirFixture - RAII wrapper for temporary directories
// ============================================================================

/// RAII fixture for temporary directories that auto-delete on destruction
class TempDirFixture
{
public:
    TempDirFixture();
    ~TempDirFixture();

    // Non-copyable
    TempDirFixture(const TempDirFixture&) = delete;
    TempDirFixture& operator=(const TempDirFixture&) = delete;

    /// Check if directory was created successfully
    bool isValid() const
    {
        return _dir && _dir->isValid();
    }

    /// Get the directory path
    QString path() const;

    /// Create a file in the temp directory
    /// @param relativePath Relative path within temp directory
    /// @param content Optional content to write
    /// @return Full path to created file, or empty string on failure
    QString createFile(const QString& relativePath, const QByteArray& content = QByteArray());

private:
    std::unique_ptr<QTemporaryDir> _dir;
};

}  // namespace TestFixtures
