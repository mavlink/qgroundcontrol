#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QSize>

class QImage;

// R6 lower-zoom fallback helpers.

// Crop the sub-square of an ancestor tile that covers descendant tile (x, y) and
// scale it up to tileSize. For level delta d the child occupies a 1/2^d sub-square
// at cell ((x mod 2^d), (y mod 2^d)) of the ancestor. Returns a null QImage on
// invalid input (null ancestor, levelDelta <= 0, or out-of-range cell).
QImage scaleAncestorToChild(const QImage &ancestor, int x, int y, int levelDelta, QSize tileSize);

// Encode a fallback QImage to PNG bytes. Returns empty on failure.
QByteArray encodeFallbackTile(const QImage &image);
