#include "GPSConnectionSupervisorTest.h"

#include <chrono>
#include <functional>
#include <optional>

#include <QtCore/QList>
#include <QtCore/QString>
#include <QtTest/QTest>

#include "GPSConnectionSupervisor.h"

using namespace std::chrono_literals;

namespace {
using Supervisor = GPSConnectionSupervisor;
using Phase = Supervisor::Phase;
using Delay = Supervisor::Delay;

/// Delays of 1, 2 and 4 seconds, and of 10 at most where the peer asks for one.
constexpr ExponentialBackoff BACKOFF{1s, 2, 4s, 10s};
constexpr std::optional<Delay> NO_DELAY;
constexpr bool GATE_CLOSED = true;

/// What the owner tells the supervisor.
struct Told
{
    const char* name;
    std::function<void(Supervisor&)> tell;
};

Told start(bool gateClosed = false)
{
    return {gateClosed ? "start behind a gate" : "start",
            [gateClosed](Supervisor& supervisor) { supervisor.start(gateClosed); }};
}

Told stop()
{
    return {"stop", [](Supervisor& supervisor) { supervisor.stop(); }};
}

Told reopen()
{
    return {"reopen", [](Supervisor& supervisor) { supervisor.reopen(); }};
}

Told connected()
{
    return {"connected", [](Supervisor& supervisor) { supervisor.connected(); }};
}

Told served()
{
    return {"served", [](Supervisor& supervisor) { supervisor.served(); }};
}

Told failed(Delay retryAfter = Delay::zero(), bool gateClosed = false)
{
    return {gateClosed ? "failed behind a gate" : "failed",
            [retryAfter, gateClosed](Supervisor& supervisor) { supervisor.failed(retryAfter, gateClosed); }};
}

Told halt()
{
    return {"halt", [](Supervisor& supervisor) { supervisor.halt(); }};
}

Told delayEnded(bool gateClosed = false)
{
    return {gateClosed ? "delay ended behind a gate" : "delay ended",
            [gateClosed](Supervisor& supervisor) { supervisor.delayEnded(gateClosed); }};
}

Told gateOpened()
{
    return {"gate opened", [](Supervisor& supervisor) { supervisor.gateOpened(); }};
}

/// One thing the supervisor is told, and what it answers: its phase, the delay before the next attempt and the
/// attempts that used the budget.
struct Step
{
    Told told;
    Phase phase;
    std::optional<Delay> retryDelay;
    int attempts;
};

using Steps = QList<Step>;

/// A row of the table: a sequence of steps for a supervisor with @a maxAttempts, which is added when it is complete.
class Sequence
{
public:
    explicit Sequence(const char* name, std::optional<int> maxAttempts = std::nullopt)
        : _name(name)
        , _maxAttempts(maxAttempts)
    {}

    ~Sequence() { QTest::newRow(_name) << _maxAttempts << _steps; }

    Sequence& told(Told told, Phase phase, std::optional<Delay> retryDelay, int attempts)
    {
        _steps.append({std::move(told), phase, retryDelay, attempts});
        return *this;
    }

private:
    const char* const _name;
    const std::optional<int> _maxAttempts;
    Steps _steps;
};

QString answer(Phase phase, std::optional<Delay> retryDelay, int attempts)
{
    static constexpr const char* PHASES[] = {"Off", "Waiting", "Opening", "Up", "Stopped"};
    return QStringLiteral("%1, %2, %3 attempts")
        .arg(QLatin1String(PHASES[static_cast<int>(phase)]),
             retryDelay ? QStringLiteral("%1 ms").arg(retryDelay->count()) : QStringLiteral("no delay"))
        .arg(attempts);
}
}  // namespace

void GPSConnectionSupervisorTest::_answers_data()
{
    QTest::addColumn<std::optional<int>>("maxAttempts");
    QTest::addColumn<Steps>("steps");

    Sequence("connects-and-is-turned-off")
        .told(start(), Phase::Opening, NO_DELAY, 0)
        .told(connected(), Phase::Up, NO_DELAY, 0)
        .told(stop(), Phase::Off, NO_DELAY, 0)
        // Turned off, it waits for nothing.
        .told(start(), Phase::Opening, NO_DELAY, 0)
        .told(failed(), Phase::Waiting, 1s, 0)
        .told(stop(), Phase::Off, NO_DELAY, 0);
    // An attempt uses the budget when it opens after its delay. Without a ceiling the connection never gives up.
    Sequence("delays-double-up-to-the-cap")
        .told(start(), Phase::Opening, NO_DELAY, 0)
        .told(failed(), Phase::Waiting, 1s, 0)
        .told(delayEnded(), Phase::Opening, NO_DELAY, 1)
        .told(failed(), Phase::Waiting, 2s, 1)
        .told(delayEnded(), Phase::Opening, NO_DELAY, 2)
        .told(failed(), Phase::Waiting, 4s, 2)
        .told(delayEnded(), Phase::Opening, NO_DELAY, 3)
        .told(failed(), Phase::Waiting, 4s, 3)
        .told(delayEnded(), Phase::Opening, NO_DELAY, 4);
    Sequence("budget-starts-over-only-when-served")
        .told(start(), Phase::Opening, NO_DELAY, 0)
        .told(failed(), Phase::Waiting, 1s, 0)
        .told(delayEnded(), Phase::Opening, NO_DELAY, 1)
        // A connection that is accepted and then dropped goes on backing off.
        .told(connected(), Phase::Up, NO_DELAY, 1)
        .told(failed(), Phase::Waiting, 2s, 1)
        .told(delayEnded(), Phase::Opening, NO_DELAY, 2)
        .told(connected(), Phase::Up, NO_DELAY, 2)
        .told(served(), Phase::Up, NO_DELAY, 0)
        .told(failed(), Phase::Waiting, 1s, 0);
    Sequence("start-gives-a-new-budget")
        .told(start(), Phase::Opening, NO_DELAY, 0)
        .told(failed(), Phase::Waiting, 1s, 0)
        .told(delayEnded(), Phase::Opening, NO_DELAY, 1)
        .told(failed(), Phase::Waiting, 2s, 1)
        // The delay that was waited for is not any more.
        .told(start(), Phase::Opening, NO_DELAY, 0)
        .told(delayEnded(), Phase::Opening, NO_DELAY, 0)
        .told(failed(), Phase::Waiting, 1s, 0);
    Sequence("peer-asks-for-a-delay")
        .told(start(), Phase::Opening, NO_DELAY, 0)
        // Longer than the backoff's delay, it replaces it.
        .told(failed(3s), Phase::Waiting, 3s, 0)
        .told(delayEnded(), Phase::Opening, NO_DELAY, 1)
        .told(failed(500ms), Phase::Waiting, 2s, 1)
        .told(delayEnded(), Phase::Opening, NO_DELAY, 2)
        // No longer than the backoff allows a peer.
        .told(failed(1h), Phase::Waiting, 10s, 2);
    Sequence("gives-up-after-the-last-attempt", 2)
        .told(start(), Phase::Opening, NO_DELAY, 0)
        .told(failed(), Phase::Waiting, 1s, 0)
        .told(delayEnded(), Phase::Opening, NO_DELAY, 1)
        .told(failed(), Phase::Waiting, 2s, 1)
        .told(delayEnded(), Phase::Opening, NO_DELAY, 2)
        .told(connected(), Phase::Up, NO_DELAY, 2)
        .told(failed(), Phase::Stopped, NO_DELAY, 2)
        // It stays ended until it is started again.
        .told(delayEnded(), Phase::Stopped, NO_DELAY, 2)
        .told(gateOpened(), Phase::Stopped, NO_DELAY, 2)
        .told(failed(), Phase::Stopped, NO_DELAY, 2)
        .told(connected(), Phase::Stopped, NO_DELAY, 2)
        .told(start(), Phase::Opening, NO_DELAY, 0);
    Sequence("failure-behind-a-closed-gate-uses-no-attempt", 1)
        .told(start(), Phase::Opening, NO_DELAY, 0)
        // It waits for the gate and not for a delay, as often as it fails so.
        .told(failed({}, GATE_CLOSED), Phase::Waiting, NO_DELAY, 0)
        .told(delayEnded(), Phase::Waiting, NO_DELAY, 0)
        .told(gateOpened(), Phase::Opening, NO_DELAY, 0)
        .told(failed({}, GATE_CLOSED), Phase::Waiting, NO_DELAY, 0)
        .told(gateOpened(), Phase::Opening, NO_DELAY, 0)
        .told(failed(), Phase::Waiting, 1s, 0)
        .told(delayEnded(), Phase::Opening, NO_DELAY, 1)
        // With the budget used up it still waits, and gives up at the failure with the gate open.
        .told(failed({}, GATE_CLOSED), Phase::Waiting, NO_DELAY, 1)
        .told(gateOpened(), Phase::Opening, NO_DELAY, 1)
        .told(failed(), Phase::Stopped, NO_DELAY, 1);
    Sequence("gate-closed-when-the-delay-ends-gives-the-attempt-back")
        .told(start(), Phase::Opening, NO_DELAY, 0)
        .told(failed(), Phase::Waiting, 1s, 0)
        .told(delayEnded(), Phase::Opening, NO_DELAY, 1)
        .told(failed(), Phase::Waiting, 2s, 1)
        .told(delayEnded(GATE_CLOSED), Phase::Waiting, NO_DELAY, 1)
        .told(gateOpened(), Phase::Opening, NO_DELAY, 1)
        // The attempt that never ran left its delay to this one.
        .told(failed(), Phase::Waiting, 2s, 1)
        .told(delayEnded(), Phase::Opening, NO_DELAY, 2);
    Sequence("gate-that-opens-makes-an-attempt-due-at-once")
        .told(start(GATE_CLOSED), Phase::Waiting, NO_DELAY, 0)
        .told(gateOpened(), Phase::Opening, NO_DELAY, 0)
        .told(connected(), Phase::Up, NO_DELAY, 0)
        // A connection that is up can be sent behind the gate, with a new budget.
        .told(start(GATE_CLOSED), Phase::Waiting, NO_DELAY, 0)
        .told(gateOpened(), Phase::Opening, NO_DELAY, 0)
        // A delay is waited out, whatever opens meanwhile.
        .told(failed(), Phase::Waiting, 1s, 0)
        .told(gateOpened(), Phase::Waiting, 1s, 0)
        .told(delayEnded(), Phase::Opening, NO_DELAY, 1);
    Sequence("halt-ends-a-wanted-connection")
        .told(halt(), Phase::Off, NO_DELAY, 0)
        .told(start(), Phase::Opening, NO_DELAY, 0)
        .told(halt(), Phase::Stopped, NO_DELAY, 0)
        .told(start(), Phase::Opening, NO_DELAY, 0)
        .told(failed(), Phase::Waiting, 1s, 0)
        .told(halt(), Phase::Stopped, NO_DELAY, 0)
        .told(delayEnded(), Phase::Stopped, NO_DELAY, 0)
        .told(stop(), Phase::Off, NO_DELAY, 0);
    Sequence("reopen-keeps-the-budget")
        .told(start(), Phase::Opening, NO_DELAY, 0)
        .told(failed(), Phase::Waiting, 1s, 0)
        .told(delayEnded(), Phase::Opening, NO_DELAY, 1)
        .told(reopen(), Phase::Opening, NO_DELAY, 1)
        .told(connected(), Phase::Up, NO_DELAY, 1)
        .told(reopen(), Phase::Opening, NO_DELAY, 1)
        .told(failed(), Phase::Waiting, 2s, 1)
        // No attempt is open to replace.
        .told(reopen(), Phase::Waiting, 2s, 1);
    Sequence("ignores-what-does-not-apply-to-the-phase")
        .told(connected(), Phase::Off, NO_DELAY, 0)
        .told(failed(), Phase::Off, NO_DELAY, 0)
        .told(delayEnded(), Phase::Off, NO_DELAY, 0)
        .told(gateOpened(), Phase::Off, NO_DELAY, 0)
        .told(reopen(), Phase::Off, NO_DELAY, 0)
        .told(start(), Phase::Opening, NO_DELAY, 0)
        .told(delayEnded(), Phase::Opening, NO_DELAY, 0)
        .told(gateOpened(), Phase::Opening, NO_DELAY, 0)
        .told(connected(), Phase::Up, NO_DELAY, 0)
        .told(connected(), Phase::Up, NO_DELAY, 0)
        .told(delayEnded(), Phase::Up, NO_DELAY, 0)
        .told(gateOpened(), Phase::Up, NO_DELAY, 0)
        .told(failed(), Phase::Waiting, 1s, 0)
        .told(connected(), Phase::Waiting, 1s, 0)
        .told(failed(), Phase::Waiting, 1s, 0);
}

void GPSConnectionSupervisorTest::_answers()
{
    QFETCH(std::optional<int>, maxAttempts);
    QFETCH(Steps, steps);
    Supervisor supervisor(BACKOFF, maxAttempts);
    QCOMPARE(supervisor.phase(), Phase::Off);
    for (qsizetype i = 0; i < steps.size(); ++i) {
        const Step& step = steps.at(i);
        step.told.tell(supervisor);
        const QString at = QStringLiteral("step %1, %2: ").arg(i + 1).arg(QLatin1String(step.told.name));
        QCOMPARE(at + answer(supervisor.phase(), supervisor.retryDelay(), supervisor.attempts()),
                 at + answer(step.phase, step.retryDelay, step.attempts));
    }
}

UT_REGISTER_TEST_LIGHTWEIGHT(GPSConnectionSupervisorTest, TestLabel::Unit)
