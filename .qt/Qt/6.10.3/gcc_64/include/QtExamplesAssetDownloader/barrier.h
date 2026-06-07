// Copyright (C) 2024 Jarek Kobus
// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef TASKING_BARRIER_H
#define TASKING_BARRIER_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "tasking_global.h"

#include "tasktree.h"

QT_BEGIN_NAMESPACE

namespace Tasking {

class TASKING_EXPORT Barrier final : public QObject
{
    Q_OBJECT

public:
    void setLimit(int value);
    int limit() const { return m_limit; }

    void start();
    void advance(); // If limit reached, stops with true
    void stopWithResult(DoneResult result); // Ignores limit

    bool isRunning() const { return m_current >= 0; }
    int current() const { return m_current; }
    std::optional<DoneResult> result() const { return m_result; }

Q_SIGNALS:
    void done(DoneResult success);

private:
    std::optional<DoneResult> m_result = {};
    int m_limit = 1;
    int m_current = -1;
};

using BarrierTask = SimpleCustomTask<Barrier>;

template <int Limit = 1>
class SharedBarrier
{
public:
    static_assert(Limit > 0, "SharedBarrier's limit should be 1 or more.");
    SharedBarrier() : m_barrier(new Barrier) {
        m_barrier->setLimit(Limit);
        m_barrier->start();
    }
    Barrier *barrier() const { return m_barrier.get(); }

private:
    std::shared_ptr<Barrier> m_barrier;
};

template <int Limit = 1>
using MultiBarrier = Storage<SharedBarrier<Limit>>;

// Can't write: "MultiBarrier barrier;". Only "MultiBarrier<> barrier;" would work.
// Can't have one alias with default type in C++17, getting the following error:
// alias template deduction only available with C++20.
using SingleBarrier = MultiBarrier<1>;

template <int Limit>
ExecutableItem waitForBarrierTask(const MultiBarrier<Limit> &sharedBarrier)
{
    return BarrierTask([sharedBarrier](Barrier &barrier) {
        SharedBarrier<Limit> *activeBarrier = sharedBarrier.activeStorage();
        if (!activeBarrier) {
            qWarning("The barrier referenced from WaitForBarrier element "
                     "is not reachable in the running tree. "
                     "It is possible that no barrier was added to the tree, "
                     "or the barrier is not reachable from where it is referenced. "
                     "The WaitForBarrier task finishes with an error. ");
            return SetupResult::StopWithError;
        }
        Barrier *activeSharedBarrier = activeBarrier->barrier();
        const std::optional<DoneResult> result = activeSharedBarrier->result();
        if (result.has_value()) {
            return *result == DoneResult::Success ? SetupResult::StopWithSuccess
                                                  : SetupResult::StopWithError;
        }
        QObject::connect(activeSharedBarrier, &Barrier::done, &barrier, &Barrier::stopWithResult);
        return SetupResult::Continue;
    });
}

template <typename Signal>
ExecutableItem signalAwaiter(const typename QtPrivate::FunctionPointer<Signal>::Object *sender, Signal signal)
{
    return BarrierTask([sender, signal](Barrier &barrier) {
        QObject::connect(sender, signal, &barrier, &Barrier::advance, Qt::SingleShotConnection);
    });
}

using BarrierKickerGetter = std::function<ExecutableItem(const SingleBarrier &)>;

class TASKING_EXPORT When final
{
public:
    explicit When(const BarrierKickerGetter &kicker) : m_barrierKicker(kicker) {}

private:
    TASKING_EXPORT friend Group operator>>(const When &whenItem, const Do &doItem);

    BarrierKickerGetter m_barrierKicker;
};

} // namespace Tasking

QT_END_NAMESPACE

#endif // TASKING_BARRIER_H
