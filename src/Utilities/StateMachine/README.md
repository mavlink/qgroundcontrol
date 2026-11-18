# State machines

A thin layer over Qt's `QStateMachine` for sequences that talk to a vehicle: a machine that knows its
vehicle and its progress, and states for the steps such sequences are made of. Everything Qt's state
machine framework offers still applies; this module only adds to it.

Four parts of the application build their sequences with it:

- `InitialConnectStateMachine` (`src/Vehicle`): what QGC requests from a vehicle that connects.
- `ComponentInformationManager` and `RequestMetaDataTypeStateMachine`
  (`src/Vehicle/ComponentInformation`): the component metadata, one type after the other.
- `ParameterManager` (`src/FactSystem`): one machine for each parameter write and each single
  parameter read, deleted when it finishes.

The module is for a sequence of asynchronous steps on a thread with an event loop: ask for
something, wait for the answer or for a timeout, go on. It is not the tool for:

- keeping a connection up with growing delays between the attempts. That is a plain value that its
  owner drives, with `ExponentialBackoff` (`src/Utilities/Timing`);
- code that blocks on a worker thread;
- something that exists once for each of many items, such as the request for a map tile;
- state that its owner reads right after the call that changes it: `start()` and `postEvent()` take
  effect in a later pass of the event loop.

## The machine

`QGCStateMachine` is a `QStateMachine` with:

- a name (its `objectName()`, used in the log) and a `vehicle()`;
- `inProgress()`, which is true from `start()` until `stopped` or `finished`;
- a progress from 0 to 1, from weights given to its states (`setProgressWeights()`,
  `setSubProgress()`), which starts over with every `start()`;
- a `scheduler()` that times the waits and delays of its states: the `RuntimeScheduler`
  (`src/Utilities/Timing`) given to its constructor, one of its own otherwise;
- factories for transitions;
- `restart()`.

## The states

A state reports each outcome with one signal, and the machine is wired with transitions on those
signals: `advance()` when the state is done or its step was skipped, `error()` when it failed, and
`timedOut()` when a wait ran out.

| State | What it does | Signals |
| --- | --- | --- |
| `FunctionState` | Runs a function | `advance()` |
| `AsyncFunctionState` | Starts something and waits for a signal, or for `complete()` or `fail()`, unless its skip predicate says that the step is to be skipped | `advance()`, `error()`, `timedOut()` |
| `DelayState` | Waits a fixed time | `advance()` |
| `RetryableRequestMessageState` | Requests a MAVLink message from the vehicle and retries | `advance()`, `error()`, `timedOut()` |
| `SendMavlinkCommandState` | Sends a command to the autopilot and waits for its result | `advance()`, `error()` |
| `WaitForMavlinkMessageState` | Waits for a message with a given id, which it can request | `advance()`, `error()`, `timedOut()` |
| `WaitForParamResponseState` | Waits for `PARAM_VALUE` or `PARAM_ERROR`, which it can request | `advance()`, `error()`, `timedOut()` |
| `QGCFinalState` | Finishes the machine | |

The two states that wait for a message of the vehicle take the request that asks for it
(`setRequest()`): a message they send when they are entered and again after each timeout, as often
as its retry count allows. `timedOut()` then comes when the last attempt has timed out.

`WaitForMavlinkMessageState`, `SendMavlinkCommandState` and `DelayState` have no caller in the
application yet. They are kept for code that does by hand what they do.

The base classes, for a state of your own: `QGCState` (name, exit callback, `advance()` and
`error()`, `addThisTransition()`, `vehicleOrWarn()`), `WaitStateBase` (a wait with a timeout),
`CompletionWaitState` (a wait that a signal or a call ends) and `VehicleMessageWaitState` (a wait
for a message of the vehicle).

## The transitions

Most transitions are Qt's own signal transitions:

```cpp
state->addTransition(state, &QGCState::advance, nextState);
```

Three more kinds, each with a factory on the machine:

- `GuardedTransition` is taken only if a predicate returns true (`addGuardedTransition()`).
- `RetryTransition` lets its source state wait again a number of times, and runs an action each
  time, before it goes on to its target (`addRetryTransition()`).
- `MachineEventTransition` is taken for a named event that was posted to the machine
  (`addEventTransition()`, `postEvent()`, `postDelayedEvent()`).

The application uses none of the three yet.

## An example

```cpp
auto* machine = new QGCStateMachine(QStringLiteral("Example"), vehicle, this);

auto* request = new RetryableRequestMessageState(
    QStringLiteral("RequestVersion"), machine, MAVLINK_MSG_ID_AUTOPILOT_VERSION,
    [](Vehicle*, const mavlink_message_t& message) { /* use the message */ });
auto* load = new AsyncFunctionState(
    QStringLiteral("Load"), machine,
    [this](AsyncFunctionState* state) {
        state->connectToCompletion(_loader, &Loader::finished);
        _loader->start();
    },
    30000);
auto* done = new FunctionState(QStringLiteral("Done"), machine, [this]() { emit ready(); });
auto* finalState = new QGCFinalState(QStringLiteral("Final"), machine);

request->addTransition(request, &QGCState::advance, load);
request->addTransition(request, &QGCState::error, done);  // the vehicle is gone
load->addTransition(load, &QGCState::advance, done);
load->addTransition(load, &WaitStateBase::timedOut, done);
done->addTransition(done, &QGCState::advance, finalState);

machine->setInitialState(request);
machine->start();
```

## What to know before you use it

- **Start and stop.** Start a machine with `QGCStateMachine::start()` and ask it `inProgress()`.
  `isRunning()` is deleted, because Qt starts a machine one pass of the event loop after `start()`. A
  `start()` for a machine that is in progress is refused with a critical message; `restart()` stops
  the machine and starts it again, once, however often it is asked to. Qt exits no state when a
  machine stops, so the waiting states stop waiting by themselves when their machine stops, and
  the exit callbacks (`setOnExit()`, `exited()`) are not called.
- **Every signal needs a way out.** A state stays active until a transition leaves it. Give a waiting
  state a transition for each signal it can emit, and a timeout unless something else is certain to
  end it. An `AsyncFunctionState` that waits without a completion connection and without a timeout
  logs a critical message.
- **What a state waits for ends with it.** Connect in the setup function, with
  `connectToCompletion()`, or with `connectWhileWaiting()` for a signal that does not end the wait,
  such as progress. These connections end when the state completes, fails, times out or is left. A
  plain `connect()` stays after a timeout, and its slot is then called for whichever state is
  current. What the state has started and is still running when it is left, such as a download, is
  cancelled in `setOnExit()`.
- **Transitions.** Use the source state as the sender of a signal transition, as the factories do,
  or an object that outlives the machine: Qt keeps a plain pointer to the sender and uses it while
  the machine runs and when it stops or finishes. To react to another object, connect to it in a
  setup function. A transition without a target, or whose target has been deleted, is taken and
  leaves the state as it is, without a warning.
- **Deleting a machine.** Use `deleteLater()`. Transitions run inside the `emit` that triggers them,
  so a slot of `finished()` or of a state is called from within the machine.
- **Errors.** A state that fails emits `error()` and stays active, so give it a transition for that
  signal like for any other: `state->addThisTransition(&QGCState::error, target)`. A machine that Qt
  finds wrongly built stops with a critical message: one with a state that has child states and no
  initial state, or with a transition to a state outside the machine, such as a state that was
  created without a parent.
- **The vehicle may be gone.** `vehicle()` returns `nullptr` for a machine without a vehicle and once
  the vehicle has been destroyed. The states that need a vehicle log a warning and emit `error()`
  then. Check it in your own callbacks.
- **Retries.** A state that waits for a message of the vehicle sends its request again by itself
  (`setRequest()`), and the retries start over each time it is entered.
  `RetryableRequestMessageState` retries by itself too, and after its last retry it emits
  `advance()` unless `setFailOnMaxRetries()` asks for `error()`; so does a request that its skip
  predicate skips. `RetryTransition` counts for each entry of its source state. Each retry lets the
  state wait again, for which an `AsyncFunctionState` runs its setup function again and a state
  with a request sends it again.
  A retry is a transition that is taken, one without a target: while the state retries, no other
  transition for the same signal is taken, unless it was added to the state before the retry
  transition.
- **Time.** Timeouts and delays run on the scheduler of the machine that has entered the state; a
  state in a plain `QStateMachine` has a scheduler of its own. `postDelayedEvent()` is Qt's and
  uses Qt's timers.
- **Progress.** The progress starts over with every `start()`, and a machine that had progressed
  reports 0 then.
- **Events.** `postEvent()` and `postDelayedEvent()` need a running machine, which a machine is from
  the entry of its initial state. Anything else is not posted, and a warning says so.
  `QGCStateMachineEvent`, a name with optional data, is the only event class of the module. States
  report through signals, which Qt turns into events itself, and a state that is no longer active
  ignores a signal that comes late. Add an event class only for something that signals cannot do.
- **Nested machines.** A `QGCStateMachine` can be a state of another machine, which starts it when it
  enters it.

## Logging

The module logs in `Utilities.StateMachine.QGCStateMachine` (the machines and the states),
`Utilities.StateMachine.RetryableRequestMessageState` and `Utilities.StateMachine.RetryTransition`.
`--logging:Utilities.StateMachine` enables all three.

- Shown when the category is enabled: a machine that starts, stops or finishes, every state that is
  entered and exited, as `Machine:State`, the steps that are skipped and the attempts of a request
  state. For a machine that hangs, the last `Entered` line names the state it waits in.
- Always in the log: warnings (`Timeout "Machine:State" after N ms`, each retry of a
  `RetryTransition`, retries that are used up, a state without its vehicle, an event that was not
  posted) and critical messages, which are coding errors and fail a unit test (a `start()` that is
  refused, a machine that Qt stops, a state that nothing can end).

## Tests

The suites are in `test/Utilities/StateMachine` and run with
`ctest --test-dir build -L Utilities -R 'State|Transition'`. `StateMachineTest` is their base class, and
`states/StateTestCommon.h` has the helpers and test states they share. `Helpers/SignalSenderShapes.cc`
checks at compile time which senders and signals the templates that take a signal accept and reject.

A test of a timeout or a delay gives its machine a `ManualScheduler`
(`test/Utilities/Timing/Support`) and advances it, so that it does not wait for the time to pass:

```cpp
ManualScheduler scheduler;
QGCStateMachine machine(QStringLiteral("Test"), nullptr, nullptr, &scheduler);
// ... a state with a timeout of 200 ms is entered ...
QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(200)));
QCOMPARE(timedOutSpy.count(), 1);
```

Advance by one timeout at a time: a longer advance also runs what is scheduled meanwhile, such as the
timeout of a retry. An owner that creates its machines itself has to pass its scheduler on:
`ParameterManager` gives the machines of a parameter write and read the scheduler that times its
own timeouts, which a test replaces with `ParameterManager::setScheduler()`.
