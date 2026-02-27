#pragma once

#include <QtCore/QString>
#include <QtCore/QLockFile>

class RunGuard
{
public:
    explicit RunGuard(const QString &key);
    ~RunGuard();

    // Returns true if another instance holds the lock.
    bool isAnotherRunning();

    // Attempts to acquire the single-instance lock.
    bool tryToRun();

    // Releases the lock if held.
    void release();

    // Optional: returns true if this instance currently holds the lock.
    bool isLocked() const { return _lockFile.isLocked(); }

private:
    static QString generateKeyHash(const QString &key, const QString &salt);
    static QString lockDir();

    const QString _key;
    const QString _lockFilePath;
    QLockFile _lockFile;

    Q_DISABLE_COPY(RunGuard)
};
