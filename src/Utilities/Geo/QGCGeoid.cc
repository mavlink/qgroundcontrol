#include "QGCGeoid.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iterator>
#include <limits>

namespace {
#include "EGM96Grid.inc"
}  // namespace

double QGCGeo::geoidSeparation(double latitudeDegrees, double longitudeDegrees)
{
    if (!std::isfinite(latitudeDegrees) || !std::isfinite(longitudeDegrees) || std::abs(latitudeDegrees) > 90.0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    constexpr double METERS_PER_CENTIMETER = 0.01;
    // Grid cells north of 90S and east of 180W.
    const double north = (latitudeDegrees + 90.0) * EGM96_GRID_CELLS_PER_DEGREE;
    double east = std::fmod(longitudeDegrees + 180.0, 360.0);
    if (east < 0.0) {
        east += 360.0;
    }
    east *= EGM96_GRID_CELLS_PER_DEGREE;
    // The last row and column are the far edge of a cell, which the north pole and a longitude that rounds up to
    // 180E interpolate to.
    const int row = (std::min) (static_cast<int>(north), static_cast<int>(std::size(EGM96_GRID_CENTIMETERS)) - 2);
    const int column = (std::min) (static_cast<int>(east), static_cast<int>(std::size(EGM96_GRID_CENTIMETERS[0])) - 2);
    const auto& southRow = EGM96_GRID_CENTIMETERS[row];
    const auto& northRow = EGM96_GRID_CENTIMETERS[row + 1];
    const double eastward = east - column;
    const double south = southRow[column] + (southRow[column + 1] - southRow[column]) * eastward;
    const double northEdge = northRow[column] + (northRow[column + 1] - northRow[column]) * eastward;
    return (south + (northEdge - south) * (north - row)) * METERS_PER_CENTIMETER;
}
