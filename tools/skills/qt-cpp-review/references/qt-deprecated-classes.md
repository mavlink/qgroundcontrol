# Qt/std Classes That Should Not Be Used in Qt Implementation

Reference for the `lint-deprecated.sh` script and deep analysis.

## Qt Classes → Replacements

| Deprecated | Replacement | Rationale |
|------------|-------------|-----------|
| Java-style iterators | STL iterators | `QT_NO_JAVA_STYLE_ITERATORS` |
| `Q_FOREACH` | Range-based for | `QT_NO_FOREACH` |
| `QScopedPointer` | `std::unique_ptr` | Can't be moved; use `const unique_ptr` for scoped semantics |
| `QSharedPointer` / `QWeakPointer` | `std::shared_ptr` / `std::weak_ptr` | QSP needs 2× atomic ops on copy; removal planned for Qt 7 |
| `QAtomic*` | `std::atomic` | Exception: static `QBasicAtomic*` (no runtime init) |
| `QPair` | `std::pair` | QPair is a type alias since Qt 6.0 |
| `QSharedDataPointer` | `QExplicitlySharedDataPointer` | QSDP detaches prematurely (atomic check on each access) |
| `q(v)nprintf()` | `std::(v)snprintf()` | Platform-dependent fallbacks; must `#include <cstdio>` |
| `qMin`/`qMax`/`qBound` | `(std::min)()` / `(std::max)()` / `std::clamp()` | Mixed-type args in Qt 6 are harder to understand; note arg order difference |
| `QChar` (as object) | `char16_t` | Language support; QChar as namespace (e.g. `QChar::isLower()`) is OK |
| `count()` / `length()` | `size()` | Consistency with std library |

## std Classes → Replacements

| Deprecated | Replacement | Rationale |
|------------|-------------|-----------|
| `std::mutex` | `QMutex` | QMutex uses futexes (faster). Exception: std::mutex + std::condition_variable combo is more efficient than QMutex + QWaitCondition |

## Anti-Patterns (not class-specific)

| Pattern | Fix | Rule |
|---------|-----|------|
| `std::optional::value()` | Use `*opt`, `opt->foo`, `if (opt)` | Throws on empty; use pointer-compatible subset |
| `std::optional{}` default ctor | Use `std::nullopt` explicitly | GCC maybe-unused warning bug |
| `std::has_alternative<T>` + `get<T>` | Use `get_if<T>` or `std::visit` | DRY; Coverity false positives |
| `p = realloc(p, ...)` | `tmp = realloc(p, ...); check; p = tmp;` | Leaks on failure |
| `std::make_unique<T[]>(n)` for scalar T | `q20::make_unique_for_overwrite<T[]>(n)` | Value-init zeros memory unnecessarily |
| `value_or()` with non-trivial arg | Ternary or if/else | Arg always evaluated |
| `QDateTime::currentDateTime()` | `currentDateTimeUtc()` | 100× faster, stable across DST |
| `QThreadPool::globalInstance()` + blocking | Dedicated pool or `releaseThread()` | Deadlock risk |
