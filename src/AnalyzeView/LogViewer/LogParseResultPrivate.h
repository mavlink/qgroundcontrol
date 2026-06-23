#pragma once

// Private implementation detail shared between LogFileParser.cc and ULogFullHandler.cc.
// Do NOT include this header from any public-facing header.

#include <QtCore/QDateTime>
#include <QtCore/QHash>
#include <QtCore/QPointF>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariantList>
#include <QtCore/QVector>

#include <atomic>
#include <functional>
#include <memory>

/// Callback invoked periodically during async parsing to report byte-position progress.
/// @param progress 0.0–1.0 fraction of the file consumed so far.
using ProgressCallback = std::function<void(float)>;

/// Shared cancellation token. The background parse thread checks this via
/// load(std::memory_order_relaxed) and exits early when it is set to true.
using CancelToken = std::shared_ptr<std::atomic<bool>>;

struct LogParseResult {
    enum class SourceType { Unknown, PX4ULog, APMDataFlash };

    bool ok = false;
    QString errorMessage;
    QStringList availableFields;
    QStringList plottableFields;
    QVariantList parameters;
    QVariantList events;
    QVariantList messages;
    QVariantList modeSegments;
    QVariantList dropouts;
    QHash<QString, QVector<QPointF>> fieldSamples;
    double minTimestamp = -1.0;
    double maxTimestamp = -1.0;
    int sampleCount = 0;
    QString detectedVehicleType;
    SourceType sourceType = SourceType::Unknown;
    int firmwareMajorVersion = -1;
    int firmwareMinorVersion = -1;
    QDateTime startTime;
};
