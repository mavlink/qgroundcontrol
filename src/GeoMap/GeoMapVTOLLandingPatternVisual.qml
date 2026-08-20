/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QGroundControl.GeoMap

/// VTOL landing pattern visual for GeoMap: just the shared base (VTOL has no
/// landing-area/glide-slope polygon, unlike Fixed Wing)
GeoMapLandingPatternVisuals {
    landingLabel: qsTr("Land")
}
