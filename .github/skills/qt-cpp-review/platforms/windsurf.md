---
trigger: model_decision
description: "Review Qt6 C++ code for correctness and best practices"
---

# Qt C++ Review

**Models (MDL)**: begin/end signals for structural changes, not
layoutChanged. dataChanged must pass specific roles. setData() must
emit dataChanged before returning true. roleNames() must match
data() cases.

**Lifecycle (LCY)**: deleteLater() on QNetworkReply in finished
handlers. QObject `new` needs parent. No side effects in Q_ASSERT.
No Q_ASSERT(ptr) as sole null guard.

**Threads (THR)**: No writing QObject members from QtConcurrent::run
without sync. No DirectConnection from worker to main thread. No
mutating models from background threads.

**Errors (ERR)**: Check QFile::open() return. Check
QJsonDocument::fromJson() with isNull(). Check QNetworkReply::error()
before readAll(). Use https:// not http://. Set
setTransferTimeout(). Handle sslErrors signal.

**noexcept (NXC)**: Q_ASSERT checking preconditions is incompatible
with noexcept. Q_ASSERT checking invariants is acceptable.

**Performance (PRF)**: No QRegularExpression in loops. Use
`const auto&` in range-for over shared containers. Use .value() not
operator[] for shared QHash/QMap reads.

**API**: Q_PROPERTY FINAL. Protect min/max: `(std::min)(a,b)`.
Use QDeadlineTimer or std::chrono for timeouts, not ints.
