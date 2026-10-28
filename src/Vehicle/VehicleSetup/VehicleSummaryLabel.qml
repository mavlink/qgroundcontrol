import QtQuick

import QGroundControl
import QGroundControl.Controls

/// Label that lines up with a neighbouring icon or dot when vertically centered. The text box has more room above
/// the capital letters than below the baseline, which would otherwise leave the text low.
QGCLabel {
    id:             control
    // Distance the capital letters sit below the middle of the text box; the padding lifts them by half of it
    bottomPadding:  Math.max(0, 2 * (baselineOffset + capitals.tightBoundingRect.y + capitals.tightBoundingRect.height / 2 - contentHeight / 2))

    TextMetrics {
        id:     capitals
        font:   control.font
        text:   "H"
    }
}
