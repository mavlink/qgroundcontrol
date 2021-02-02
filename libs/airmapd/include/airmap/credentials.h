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
#ifndef AIRMAP_CREDENTIALS_H_
#define AIRMAP_CREDENTIALS_H_

#include <airmap/optional.h>
#include <airmap/visibility.h>

#include <iosfwd>
#include <string>

namespace airmap {

/// Credentials bundles up all credentials required
/// to use the AirMap SDK and APIs.
struct AIRMAP_EXPORT Credentials {
  enum class Type { anonymous, oauth };

  /// Anonymous bundles up all attributes needed to
  /// authenticate anonymously with the AirMap services.
  struct Anonymous {
    std::string id;
  };

  /// OAuth bundles up all attributes needed to authenticate
  /// with username/password with the AirMap services.
  struct OAuth {
    std::string username;
    std::string password;
    std::string client_id;
    std::string device_id;
  };

  std::string api_key;    ///< Use this api key when accessing the AirMap services
  Optional<OAuth> oauth;  /// Optional attributes for authenticating with username/password with the AirMap services
  Optional<Anonymous> anonymous;  /// Optional attributes for authenticating anonymously with the AirMap services
};

/// operator>> extracts type from in.
AIRMAP_EXPORT std::istream& operator>>(std::istream& in, Credentials::Type& type);
/// operator<< inserts type into out.
AIRMAP_EXPORT std::ostream& operator<<(std::ostream& out, Credentials::Type type);

}  // namespace airmap

#endif  // AIRMAP_CREDENTIALS_H_
