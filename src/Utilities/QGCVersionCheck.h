#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QString>

Q_DECLARE_LOGGING_CATEGORY(QGCVersionCheckLog)

/// Checks for a newer stable release and notifies the user once per new version.
/// Versions are of the form "v<major>.<minor>.<patch>".
class QGCVersionCheck : public QObject
{
    Q_OBJECT

public:
    explicit QGCVersionCheck(QObject* parent = nullptr);

    static QGCVersionCheck* instance();

    /// Starts the asynchronous download of the stable version file
    void start();

    /// Newer stable version available for download, empty if none
    QString newStableVersion() const { return _newStableVersion; }

    /// @return true if latestVersion parses and is newer than currentVersion
    [[nodiscard]] static bool isNewerVersion(const QString& currentVersion, const QString& latestVersion);

    /// @return true if the user has not yet been told about latestVersion (or anything newer)
    [[nodiscard]] static bool shouldNotify(const QString& latestVersion, const QString& lastNotifiedVersion);

signals:
    void newStableVersionChanged();

private:
    void _downloadComplete(bool success, const QString& localFile, const QString& errorMsg);

    QString _newStableVersion;
};
