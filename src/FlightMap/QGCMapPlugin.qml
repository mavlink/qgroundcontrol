pragma Singleton

import QtLocation

/// The QtLocation plugin of every map. QtLocation gives each Plugin object a mapping engine of its own, so the maps
/// that share this one share its tile and texture caches.
Plugin {
    name: "QGroundControl"
}
