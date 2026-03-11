#pragma once

#include <QtGui/QQuaternion>
#include <QtPositioning/QGeoCoordinate>
#include <cstdint>

struct GeoTagData {
    enum class CaptureResult : int8_t {
        NoFeedback = -1,
        Failure = 0,
        Success = 1
    };

    qint64 timestamp = 0;       ///< Seconds since epoch
    qint64 timestampUTC = 0;    ///< Seconds since epoch (UTC)
    uint32_t imageSequence = 0;
    QGeoCoordinate coordinate;
    float groundDistance = 0.f;
    QQuaternion attitude;
    CaptureResult captureResult = CaptureResult::NoFeedback;

    bool isValid() const { return coordinate.isValid() && captureResult == CaptureResult::Success; }
};
