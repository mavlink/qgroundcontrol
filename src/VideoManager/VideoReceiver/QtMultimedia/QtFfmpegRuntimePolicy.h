#pragma once

#include <QtCore/QStringList>

/// Process-level defaults that make QGC's QtMultimedia usage land on the
/// FFmpeg backend and prefer platform HW decode paths without overriding
/// operator-supplied environment choices.
class QtFfmpegRuntimePolicy final
{
public:
    QtFfmpegRuntimePolicy() = delete;

    static void applyDefaults();
    [[nodiscard]] static QStringList diagnosticLines();
};
