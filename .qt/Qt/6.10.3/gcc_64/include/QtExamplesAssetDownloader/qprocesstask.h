// Copyright (C) 2024 Jarek Kobus
// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef TASKING_QPROCESSTASK_H
#define TASKING_QPROCESSTASK_H

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

#include <QtCore/QProcess>

QT_BEGIN_NAMESPACE

#if QT_CONFIG(process)

namespace Tasking {

// Deleting a running QProcess may block the caller thread up to 30 seconds and issue warnings.
// To avoid these issues we move the running QProcess into a separate thread
// managed by the internal ProcessReaper, instead of deleting it immediately.
// Inside the ProcessReaper's thread we try to finish the process in a most gentle way:
// we call QProcess::terminate() with 500 ms timeout, and if the process is still running
// after this timeout passed, we call QProcess::kill() and wait for the process to finish.
// All these handlings are done is a separate thread, so the main thread doesn't block at all
// when the QProcessTask is destructed.
// Finally, on application quit, QProcessDeleter::deleteAll() should be called in order
// to synchronize all the processes being still potentially reaped in a separate thread.
// The call to QProcessDeleter::deleteAll() is blocking in case some processes
// are still being reaped.
// This strategy seems most sensible, since when passing the running QProcess into the
// ProcessReaper we don't block immediately, but postpone the possible (not certain) block
// until the end of an application.
// In this way we terminate the running processes in the most safe way and keep the main thread
// responsive. That's a common case when the running application wants to terminate the QProcess
// immediately (e.g. on Cancel button pressed), without keeping and managing the handle
// to the still running QProcess.

// The implementation of the internal reaper is inspired by the Utils::ProcessReaper taken
// from the QtCreator codebase.

class TASKING_EXPORT QProcessDeleter
{
public:
    // Blocking, should be called after all QProcessAdapter instances are deleted.
    static void deleteAll();
    void operator()(QProcess *process);
};

class TASKING_EXPORT QProcessAdapter : public TaskAdapter<QProcess, QProcessDeleter>
{
private:
    void start() final {
        connect(task(), &QProcess::finished, this, [this] {
            const bool success = task()->exitStatus() == QProcess::NormalExit
                                 && task()->error() == QProcess::UnknownError
                                 && task()->exitCode() == 0;
            Q_EMIT done(toDoneResult(success));
        });
        connect(task(), &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
            if (error != QProcess::FailedToStart)
                return;
            Q_EMIT done(DoneResult::Error);
        });
        task()->start();
    }
};

using QProcessTask = CustomTask<QProcessAdapter>;

} // namespace Tasking

#endif // QT_CONFIG(process)

QT_END_NAMESPACE

#endif // TASKING_QPROCESSTASK_H
