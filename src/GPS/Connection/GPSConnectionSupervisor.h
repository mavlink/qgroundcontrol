#pragma once

#include <optional>

#include "ExponentialBackoff.h"

/// Supervises one connection that is kept up: says when an attempt opens, how long a retry waits and when the
/// connection gives up. It opens nothing and keeps no time. Its owner tells it what happened, reads phase() and
/// retryDelay() for what follows, opens the attempts and times the delays. What does not apply to the phase is
/// ignored.
///
/// The budget is the backoff's delays between attempts that fail one after the other, and the number of such
/// attempts. An attempt uses the budget when it opens after its delay; one that start(), reopen() or a gate opens
/// does not. The budget starts over with start() and with served(), which the owner calls once the connection has
/// done what it is for: a connection that is accepted and then dropped goes on using it.
///
/// A gate is whatever the owner opens no attempt without, such as a network. The supervisor does not know which
/// gates there are: the owner says whether one is closed where an attempt would follow.
/// - A failure behind a closed gate uses nothing of the budget: the connection waits for gateOpened(), not for a
///   delay, and does not give up.
/// - A gate found closed when the delay ends gives the attempt back: the failure after the next attempt has that
///   delay again.
/// - A gate that opens makes an attempt due at once.
class GPSConnectionSupervisor
{
public:
    using Delay = ExponentialBackoff::Delay;

    enum class Phase
    {
        Off,      ///< Not wanted.
        Waiting,  ///< Wanted, with no attempt open: for retryDelay(), or else behind a gate.
        Opening,  ///< An attempt is open and has not connected.
        Up,       ///< Connected.
        Stopped,  ///< Wanted, but ended: by halt(), or by a failure when the budget was used up.
    };

    /// @a backoff gives the delays and the longest one a peer may ask for. After @a maxAttempts attempts of one
    /// budget the next failure stops the connection; without it the connection is retried for ever.
    explicit GPSConnectionSupervisor(ExponentialBackoff backoff, std::optional<int> maxAttempts = std::nullopt);

    /// The connection is wanted, with a new budget, whatever the phase: Opening, or Waiting behind a closed gate.
    void start(bool gateClosed = false);
    /// The connection is not wanted: Off.
    void stop();
    /// The owner replaced the open attempt, or the connection, with a new attempt: Opening, on the same budget.
    void reopen();
    /// The open attempt connected: Up.
    void connected();
    /// The connection did what it is for: the budget starts over.
    void served();
    /// The open attempt or the connection failed, and is retried: Waiting, for retryDelay() or behind a closed gate,
    /// or Stopped when the budget is used up. @a retryAfter is the delay the peer asked for, which lengthens the
    /// backoff's as far as the backoff allows.
    void failed(Delay retryAfter = Delay::zero(), bool gateClosed = false);
    /// The connection ended in a way that is not retried: Stopped, unless it is Off.
    void halt();
    /// The delay of retryDelay() ended: Opening, or Waiting behind a closed gate.
    void delayEnded(bool gateClosed = false);
    /// The gate that a waiting connection is behind opened: Opening.
    void gateOpened();

    Phase phase() const { return _phase; }

    /// The delay before the next attempt, while the connection waits for one; none behind a gate.
    std::optional<Delay> retryDelay() const { return _retryDelay; }

    /// The attempts that used the budget since it last started over.
    int attempts() const { return _backoff.attempts(); }

private:
    ExponentialBackoff _backoff;
    std::optional<int> _maxAttempts;
    Phase _phase = Phase::Off;
    std::optional<Delay> _retryDelay;
};
