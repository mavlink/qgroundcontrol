// Copyright (C) 2016 Research In Motion.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKFLICKABLEBEHAVIOR_H
#define QQUICKFLICKABLEBEHAVIOR_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

/* ### Platform specific flickable mechanics are defined either here, or in
       mkspec files. Long-term (QtQuick 3) Flickable needs to allow such
       mechanic details to be controlled via QML so that platforms can easily
       load custom behavior at QML compile time.
*/

// The maximum number of pixels a flick can overshoot
#ifndef QML_FLICK_OVERSHOOT
#define QML_FLICK_OVERSHOOT 150
#endif

// The number of samples to use in calculating the velocity of a flick
#ifndef QML_FLICK_SAMPLEBUFFER
#define QML_FLICK_SAMPLEBUFFER 3
#endif

// The number of samples to discard when calculating the flick velocity.
// Touch panels often produce inaccurate results as the finger is lifted.
#ifndef QML_FLICK_DISCARDSAMPLES
#define QML_FLICK_DISCARDSAMPLES 0
#endif

// How much faster to decelerate when overshooting
#ifndef QML_FLICK_OVERSHOOTFRICTION
#define QML_FLICK_OVERSHOOTFRICTION 8
#endif

// Multiflick acceleration minimum flick velocity threshold
#ifndef QML_FLICK_MULTIFLICK_THRESHOLD
#define QML_FLICK_MULTIFLICK_THRESHOLD 1250
#endif

// If the time (ms) between the last move and the release exceeds this, then velocity will be zero.
#ifndef QML_FLICK_VELOCITY_DECAY_TIME
#define QML_FLICK_VELOCITY_DECAY_TIME 50
#endif

// Multiflick acceleration minimum contentSize/viewSize ratio
#ifndef QML_FLICK_MULTIFLICK_RATIO
#define QML_FLICK_MULTIFLICK_RATIO 10
#endif

// Multiflick acceleration maximum velocity multiplier
#ifndef QML_FLICK_MULTIFLICK_MAXBOOST
#define QML_FLICK_MULTIFLICK_MAXBOOST 3.0
#endif

// Really slow flicks can be annoying.
const qreal _q_MinimumFlickVelocity = 75.0;

// If QQuickFlickablePrivate::wheelDeceleration (perhaps overridden via QT_QUICK_FLICKABLE_WHEEL_DECELERATION)
// is greater than this, we switch to proportional wheel scrolling: no "acceleration" at all.
const qreal _q_MaximumWheelDeceleration = 14999;

#endif //QQUICKFLICKABLEBEHAVIOR_H
