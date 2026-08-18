/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QtMinMax>

#include <cmath>

/// Bilinear sample at a unit-UV position within a w x h sample grid (origin
/// NW corner), sample-center convention, clamped at grid edges. `at(x, y)`
/// returns the sample value as double.
///
/// The one shared interpolation for all elevation samplers: cross-source
/// vertex identity requires bit-identical arithmetic, so samplers must go
/// through this template rather than reimplementing it.
template <typename SampleAt>
double bilinearAtUV(int w, int h, double u, double v, SampleAt at)
{
    if ((w <= 0) || (h <= 0)) {
        return 0.0;
    }
    const double px = (u * w) - 0.5;
    const double py = (v * h) - 0.5;
    const int x0 = qBound(0, static_cast<int>(std::floor(px)), w - 1);
    const int y0 = qBound(0, static_cast<int>(std::floor(py)), h - 1);
    const int x1 = qMin(x0 + 1, w - 1);
    const int y1 = qMin(y0 + 1, h - 1);
    const double fx = qBound(0.0, px - x0, 1.0);
    const double fy = qBound(0.0, py - y0, 1.0);

    const double north = (at(x0, y0) * (1.0 - fx)) + (at(x1, y0) * fx);
    const double south = (at(x0, y1) * (1.0 - fx)) + (at(x1, y1) * fx);
    return (north * (1.0 - fy)) + (south * fy);
}
