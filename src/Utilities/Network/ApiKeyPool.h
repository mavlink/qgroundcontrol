/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QDateTime>
#include <QtCore/QLoggingCategory>
#include <QtCore/QMutex>
#include <QtCore/QObject>
#include <QtCore/QStringList>

Q_DECLARE_LOGGING_CATEGORY(ApiKeyPoolLog)

struct ApiKeyInfo {
    QString key;
    quint64 usageCount = 0;
    quint64 quotaRemaining = 0;
    QDateTime quotaResetTime;
    bool isBlocked = false;
};

class ApiKeyPool : public QObject
{
    Q_OBJECT

public:
    explicit ApiKeyPool(QObject *parent = nullptr);

    // Key management
    void addKey(const QString &key, quint64 dailyQuota = 0);
    void removeKey(const QString &key);
    void clearKeys();

    // Key selection (round-robin with quota awareness)
    QString getNextKey();
    QString currentKey() const;

    // Quota tracking
    void recordKeyUsage(const QString &key, quint64 count = 1);
    void blockKey(const QString &key, const QString &reason = QString());
    void unblockKey(const QString &key);
    void setQuotaRemaining(const QString &key, quint64 remaining);
    void setQuotaResetTime(const QString &key, const QDateTime &resetTime);

    // Statistics
    int keyCount() const;
    int availableKeyCount() const;
    QList<ApiKeyInfo> allKeys() const;

    // Persistence
    void loadKeys(const QStringList &keys);
    QStringList saveKeys() const;

signals:
    void keyBlocked(const QString &key, const QString &reason);
    void allKeysExhausted();

private:
    QString selectNextAvailableKey();
    bool isKeyAvailable(const ApiKeyInfo &keyInfo) const;

    QList<ApiKeyInfo> _keys;
    int _currentKeyIndex = 0;
    mutable QMutex _mutex;
};
