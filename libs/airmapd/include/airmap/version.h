#ifndef AIRMAP_VERSION_H_
#define AIRMAP_VERSION_H_

#include <airmap/date_time.h>
#include <airmap/optional.h>

#include <cstdint>

namespace airmap {

/// Version bundles up information describing a specific version of the AirMap
/// client library. We follow semantic versioning guidelines (see https://semver.org).
struct Version {
  /// current returns an immutable reference to the version of the client library.
  static const Version& current();

  std::uint32_t major;                 ///< The major version number.
  std::uint32_t minor;                 ///< The minor version number.
  std::uint32_t patch;                 ///< The patch version number.
  Optional<std::string> git_revision;  ///< The git revision from which the release was build.
  Optional<DateTime> build_timestamp;  ///< Marks the time when the library was built.
};

}  // namespace airmap

#endif  // AIRMAP_VERSION_H_
