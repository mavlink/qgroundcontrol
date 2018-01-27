#ifndef AIRMAP_CREDENTIALS_H_
#define AIRMAP_CREDENTIALS_H_

#include <airmap/optional.h>

#include <iosfwd>
#include <string>

namespace airmap {

/// Credentials bundles up all credentials required
/// to use the AirMap SDK and APIs.
struct Credentials {
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
std::istream& operator>>(std::istream& in, Credentials::Type& type);
/// operator<< inserts type into out.
std::ostream& operator<<(std::ostream& out, Credentials::Type type);

}  // namespace airmap

#endif  // AIRMAP_CREDENTIALS_H_
