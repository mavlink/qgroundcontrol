// AirMap Platform SDK
// Copyright © 2018 AirMap, Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the License);
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//   http://www.apache.org/licenses/LICENSE-2.0
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an AS IS BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#ifndef AIRMAP_VERSION_H_
#define AIRMAP_VERSION_H_

#include <airmap/date_time.h>
#include <airmap/optional.h>
#include <airmap/visibility.h>

#include <cstdint>

namespace airmap {

/// Version bundles up information describing a specific version of the AirMap
/// client library. We follow semantic versioning guidelines (see https://semver.org).
struct AIRMAP_EXPORT Version {
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
