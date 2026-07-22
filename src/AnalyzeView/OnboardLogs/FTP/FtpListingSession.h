#pragma once

#include <QtCore/QString>
#include <QtCore/QStringList>
#include <cstdint>

#include "FtpListingParser.h"
#include "OnboardLogSession.h"

/// Directory traversal and parser state for one MAVLink FTP onboard-log scan.
class FtpListingSession final : public OnboardLogSessionBase
{
    Q_DISABLE_COPY_MOVE(FtpListingSession)

public:
    enum class State : uint8_t
    {
        Idle,
        ListingRoot,
        ListingSubdir,
    };

    FtpListingSession() = default;

    [[nodiscard]] quint64 begin(const QString& root, uint firstLogId = 0, int entryBudget = kMaxListingEntries);
    bool cancel();
    void finish();
    void clear();

    bool useFallbackRoot(const QString& root);
    bool observeDirectoryResult(qsizetype entryCount, bool truncated);
    FtpListingParser::ParseResult parse(const QStringList& entries, const QString& subdir, bool collectDirectories);
    void beginSubdirectories();
    void completeCurrentDirectory();

    void markPartial() { _partial = true; }

    bool partial() const { return _partial; }

    bool isCurrent(quint64 generation) const;
    bool canListNextDirectory() const;

    bool hasPendingDirectories() const { return !_directories.isEmpty(); }

    bool logLimitReached() const { return _parser.nextLogId() >= FtpListingParser::kMaxLogEntries; }

    State state() const { return _state; }

    const QString& root() const { return _root; }

    QString currentDirectory() const;
    QString currentDirectoryPath() const;

    qsizetype pendingDirectoryCount() const { return _directories.size(); }

    int remainingEntryBudget() const { return _remainingEntryBudget; }

    static constexpr int kMaxListingEntries =
        static_cast<int>(FtpListingParser::kMaxLogEntries + FtpListingParser::kMaxLogDirectories);

private:
    State _state = State::Idle;
    QString _root;
    QStringList _directories;
    FtpListingParser _parser;
    uint _firstLogId = 0;
    int _remainingEntryBudget = 0;
    bool _triedFallbackRoot = false;
    bool _partial = false;
};
