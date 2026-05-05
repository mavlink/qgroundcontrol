#include "FtpListingSession.h"

#include <algorithm>
#include <limits>

quint64 FtpListingSession::begin(const QString& root, uint firstLogId, int entryBudget)
{
    ++_generation;
    _state = State::ListingRoot;
    _root = root;
    _directories.clear();
    _firstLogId = firstLogId;
    _parser.reset(root, _firstLogId);
    _remainingEntryBudget = (std::max) (0, entryBudget);
    _triedFallbackRoot = false;
    _partial = false;
    _canceling = false;
    return _generation;
}

bool FtpListingSession::cancel()
{
    ++_generation;
    _directories.clear();
    if (!active()) {
        return false;
    }

    _canceling = true;
    return true;
}

void FtpListingSession::finish()
{
    _state = State::Idle;
    _directories.clear();
    _canceling = false;
}

void FtpListingSession::clear()
{
    ++_generation;
    finish();
    _root.clear();
    _firstLogId = 0;
    _parser.reset(QString());
    _remainingEntryBudget = 0;
    _triedFallbackRoot = false;
    _partial = false;
}

bool FtpListingSession::useFallbackRoot(const QString& root)
{
    if ((_state != State::ListingRoot) || _triedFallbackRoot || root.isEmpty()) {
        return false;
    }

    _triedFallbackRoot = true;
    _root = root;
    _parser.reset(root, _firstLogId);
    return true;
}

bool FtpListingSession::observeDirectoryResult(qsizetype entryCount, bool truncated)
{
    const qsizetype boundedEntryCount =
        std::clamp(entryCount, qsizetype(0), static_cast<qsizetype>((std::numeric_limits<int>::max)()));
    _remainingEntryBudget = (std::max) (0, _remainingEntryBudget - static_cast<int>(boundedEntryCount));
    if (truncated) {
        _partial = true;
        return true;
    }

    return false;
}

FtpListingParser::ParseResult FtpListingSession::parse(const QStringList& entries, const QString& subdir,
                                                       bool collectDirectories)
{
    FtpListingParser::ParseResult result = _parser.parse(entries, subdir, collectDirectories);
    _directories.append(result.directories);
    if (result.partial()) {
        _partial = true;
    }
    return result;
}

void FtpListingSession::beginSubdirectories()
{
    _directories.sort();
    _state = State::ListingSubdir;
}

void FtpListingSession::completeCurrentDirectory()
{
    if (!_directories.isEmpty()) {
        _directories.removeFirst();
    }
}

bool FtpListingSession::isCurrent(quint64 generation) const
{
    return (generation == _generation) && active() && !_canceling;
}

bool FtpListingSession::canListNextDirectory() const
{
    return hasPendingDirectories() && !logLimitReached() && (_remainingEntryBudget > 0);
}

QString FtpListingSession::currentDirectory() const
{
    return _directories.isEmpty() ? QString() : _directories.constFirst();
}

QString FtpListingSession::currentDirectoryPath() const
{
    const QString directory = currentDirectory();
    return directory.isEmpty() ? QString() : (_root + QStringLiteral("/") + directory);
}
