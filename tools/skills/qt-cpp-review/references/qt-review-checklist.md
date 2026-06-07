# Qt6 Code Review Checklist

Distilled from the Qt Wiki "Things To Look Out For In Reviews".
Each rule has a short ID for cross-referencing in review reports.

Rules specific to Qt framework/module development (binary
compatibility, export macros, d-pointers, qdoc, QML versioning)
are in `qt-framework-checklist.md` — loaded only when reviewing
Qt module code.

## API & Naming

- **API-3**: Check naming consistency with similar Qt classes
  (e.g. `timeout` not `timeOut`, `size()` not `count()`).
- **API-5**: `get`-prefix means user interaction or decomposition
  (out-params), NOT mere getters.

## Public Headers

- **HDR-3**: Protect min/max calls: `(std::min)(a,b)`,
  `(std::numeric_limits<T>::min)()`. Also in .cpp files
  (unity builds).

## Includes

- **INC-4**: Include everything needed in-size (Lakos). Don't
  rely on transitive includes.

## Variables

- **VAR-3**: Braced initializers: opening `{` on same line.
  Prefer `var = {` over `var{`.
- **VAR-4**: Never use dynamically-sized containers for
  statically-sized data. Use `std::array` or C arrays.

## Methods

- **MTH-1**: Inline methods: prefer defining in class body
  (skip `inline` keyword).

## Macros

- **MAC-1**: Function-scope macros → `do {} while(false)`.

## Properties

- **PRP-1**: New classes: Q_PROPERTY FINAL unless intended for
  override.
- **PRP-3**: Avoid properties with same name as meta-methods
  (shadowing in QML).

## Timeouts

- **TMO-1**: No ints/qint64 for timeouts or intervals. Use
  QDeadlineTimer or std::chrono types.

## Polymorphic Classes

- **PLY-2**: Q_DISABLE_COPY_MOVE on polymorphic classes.
- **PLY-3**: Overridden virtuals: same default args and access
  specifier as base. Comment if intentional deviation.
- **PLY-4**: Virtual functions marked by exactly ONE of
  `virtual`, `override`, `final`.
- **PLY-5**: If class is `final`, use `override` on methods
  (not `final`).
- **PLY-6**: Virtual access: public if callable, private if
  reimpl shouldn't call base, protected if reimpl should call
  base.

## QObject Subclasses

- **QOB-1**: Always include Q_OBJECT macro.
- **QOB-4**: Idiomatic element order: Q_OBJECT, Q_PROPERTY,
  Q_CLASSINFO, public (enums, ctors, all non-mutating methods),
  public slots, signals, event handlers, protected, private.

## RAII Classes

- **RAI-2**: Q_DISABLE_COPY. Make movable (or comment why not).
- **RAI-3**: Move-assignment: use move-and-swap.

## Tests

- **TST-1**: QCOMPARE_EQ for QStringList comparisons.
- **TST-2**: QCOMPARE: tested value first, expected second.
- **TST-3**: QSKIP over `#if` for non-pertinent tests.

## Special Member Functions

- **SMF-1**: Order: default ctor, non-SMF ctors, copy ctor,
  copy-assign, move ctor, move-assign, dtor, swap.
- **SMF-6**: Never implement copy/move ctor via assignment.

## Enums

- **ENM-1**: Trailing comma on last enumerator — reduces diff
  noise when adding new values.
- **ENM-2**: Scoped or explicit underlying type — prevents the
  underlying type from changing (binary compatibility break).
- **ENM-4**: Purpose clarity: enumeration (no =), QFlags
  (= 0x), strong typedef (arithmetic ops).
- **ENM-5**: `{}` (value 0) should mean "default".
- **ENM-7**: Switch over enum: no `default:` label, list all
  enumerators explicitly.

## Exceptions / noexcept

- **NXC-1**: If a function is marked `noexcept`, verify it
  cannot fail — check for allocation, Q_ASSERT (precondition
  style), and calls to functions that may throw. Flag only if
  a clear throwing path is found (Lakos Rule).
- **NXC-2**: Smart pointer `operator*()` may be noexcept but
  then must not contain Q_ASSERT.
- **NXC-3**: Q_ASSERTs checking caller obligations
  (preconditions) are incompatible with noexcept. Q_ASSERTs
  verifying internal invariants are acceptable in noexcept
  functions. If the distinction is unclear, report as an
  investigation target for human verification.

## Functions — Returning Data

- **RET-1**: Prefer returning by value (compilers dislike
  out-params).
- **RET-2**: Write functions to enable RVO/NRVO. Don't mix
  named and unnamed returns in the same function. Flag only
  when mixed return paths are visible in the source — do not
  guess whether the compiler will apply the optimization.

## Move Semantics

- **MOV-1**: Distinguish rvalue refs (std::move) from universal
  refs (std::forward).
- **MOV-2**: Document moved-from state: default-constructed,
  valid-but-unspecified, or partially-formed.

## Operators

- **OPR-1**: Operators as hidden friends of least-general class.
- **OPR-2**: Never break equality/qHash relation.
- **OPR-3**: No fuzzy FP comparisons in regular relational
  operators.

## Lambdas

- **LAM-1**: Always name lambdas (except IILE, private slots).
- **LAM-2**: Use domain-specific names, not prose of
  implementation.
- **LAM-3**: Stateful lambdas: lambda-returning-lambda pattern.
- **LAM-4**: Omit `()` when empty (unless needed for ->, mutable,
  noexcept).

## Templates

- **TPL-1**: Know mandates (static_assert) vs constraints
  (SFINAE). Use constraints when overloaded.
- **TPL-5**: Don't explicitly specify deducible template args.

## Ternary Operator

- **TRN-1**: Nested ternaries: one condition per line.
- **TRN-2**: Long condition: break with `?`/`:` at start of
  continuation line.
- **TRN-3**: Never use ternary to invert/convert to bool.

## Relational Operators

- **REL-2**: Never convert unsigned to signed for comparison.

## Conditional Compilation

- **CND-1**: Don't extra-indent inside temporary #ifdefs.

## Model Contracts (QAbstractItemModel)

- **MDL-1**: Structural changes (add/remove/move rows) must use
  proper begin/end signals, not `layoutChanged`.
- **MDL-2**: `dataChanged` should pass specific changed roles,
  not an empty vector (empty = "all roles changed").
- **MDL-3**: `setData()` must emit `dataChanged` before
  returning true (or not return true without emitting).
- **MDL-4**: `beginRemoveRows(parent, 0, count-1)` where
  count==0 violates QAIM contract (first > last).
- **MDL-5**: `flags()` should return appropriate flags per
  item type (e.g. no `ItemIsEditable` on category nodes).
- **MDL-6**: begin/end signal pairs must be balanced within
  each code path.
- **MDL-7**: `data()` switch should list all role cases
  explicitly; avoid `default:` (suppresses -Wswitch).
- **MDL-8**: `roleNames()` return must match `data()` switch
  cases. Missing cases return `QVariant()` silently.
- **MDL-9**: Proxy/filter models must use source model's
  `data()`/`index()` API, not raw struct pointers.
- **MDL-10**: `roleNames()` should cache the QHash (static
  local or member), not rebuild on every call.

## Error Handling & Validation

- **ERR-1**: Check `QFile::open()` return value before
  reading/writing.
- **ERR-2**: Check `QJsonDocument::fromJson()` result with
  `isNull()`/`isObject()` before accessing data.
- **ERR-3**: Check `QNetworkReply::error()` before
  `readAll()`.
- **ERR-4**: Use `https://` not `http://` for network URLs.
- **ERR-5**: Set `QNetworkRequest::setTransferTimeout()` on
  all network requests.
- **ERR-6**: Match `QString::arg()` placeholder count to
  `.arg()` call count.
- **ERR-7**: Check `QXmlStreamWriter::hasError()` after
  writing.
- **ERR-8**: Validate negative values in integer setters
  (not just zero).
- **ERR-9**: Handle `QNetworkAccessManager::sslErrors` signal.
- **ERR-10**: Validate schema version on imported data.
- **ERR-11**: Validate input lengths from untrusted sources
  (imported JSON, network downloads).
- **ERR-12**: Consistent error reporting pattern across
  methods (don't mix return-bool, set-error, emit-signal).

## Resource Lifecycle

- **LCY-1**: Call `deleteLater()` on QNetworkReply in every
  finished handler.
- **LCY-2**: QObject-derived objects created with `new` should
  have a parent (or explicit lifecycle management).
- **LCY-3**: Never put side-effectful expressions inside
  `Q_ASSERT` (compiled out in release).
- **LCY-4**: Don't use `Q_ASSERT(ptr)` as the sole null guard
  before dereference (crashes in release).
- **LCY-5**: Cap unbounded container growth (append-only lists
  with no trim/clear).
- **LCY-6**: Destructor must clean up owned children
  recursively (qDeleteAll on direct children leaks
  grandchildren).

## Thread Safety

- **THR-1**: Never write QObject member variables from
  `QtConcurrent::run()` without synchronization (mutex,
  atomic, or queued invocation).
- **THR-2**: Never emit signals from worker threads with
  `Qt::DirectConnection` to main-thread receivers.
- **THR-3**: Never mutate QAbstractItemModel from background
  threads.
- **THR-4**: Protect shared containers consistently with mutex
  across all code paths (not just some).
- **THR-5**: Use `std::atomic` or mutex for shared counters
  accessed from multiple threads.

## Performance & Code Quality

- **PRF-1**: Don't construct `QRegularExpression` inside loops
  (expensive compilation).
- **PRF-2**: Cache `roleNames()` QHash (static local or member).
- **PRF-3**: Use `const auto&` in range-for over shared
  containers to avoid COW detach.
- **PRF-4**: Use `.value()` not `operator[]` for reads on
  shared QHash/QMap (avoids detach).
- **PRF-5**: Put cheap early-exit checks before expensive
  operations.
- **PRF-6**: Flag likely dead code (unreachable branches, unused
  methods, unused members). If callers may exist outside the
  reviewed scope (templates, plugins, reflection), report as
  investigation target instead of confirmed finding.
- **PRF-7**: Extract magic numbers to named constants.
- **PRF-8**: Don't use `QMap` for small fixed-size constant
  data (use array, switch, or if-chain).
- **PRF-9**: Invalidate member caches when underlying data
  changes.
- **PRF-10**: Add re-entrancy guards on methods that emit
  signals which could trigger recursive calls.
