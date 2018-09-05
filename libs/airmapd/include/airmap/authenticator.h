#ifndef AIRMAP_AUTHENTICATOR_H_
#define AIRMAP_AUTHENTICATOR_H_

#include <airmap/credentials.h>
#include <airmap/do_not_copy_or_move.h>
#include <airmap/error.h>
#include <airmap/outcome.h>
#include <airmap/token.h>

#include <chrono>
#include <functional>
#include <stdexcept>
#include <string>

namespace airmap {

/// Authenticator provides functionality to authenticate with the AirMap services.
class Authenticator : DoNotCopyOrMove {
 public:
  /// Scope enumerates all known authentication scopes.
  enum class Scope { access_token = 0, open_id = 1, open_id_offline_access = 2 };
  /// GrantType enumerates all known grant types.
  enum class GrantType {
    password = 0,  ///< The grant is constituted by a password
    bearer   = 1   ///< The grant is constituted by a bearer
  };
  /// Connection enumerates all known types of connection to users.
  enum class Connection {
    username_password_authentication = 0  ///< authentication requires username/password
  };

  /// AuthenticateWithPassword groups together types to ease interaction with
  /// Authenticator::authenticate_with_password.
  struct AuthenticateWithPassword {
    /// Parameters bundles up input parameters.
    struct Params {
      Credentials::OAuth oauth;                    ///< OAuth-specific credentials for this authentication request.
      GrantType grant_type{GrantType::password};   ///< The grant type of this authentication request.
      Scope scope{Scope::open_id_offline_access};  ///< The scope of this authentication request.
      Connection connection{
          Connection::username_password_authentication};  ///< The connection type of the authentication request.
    };

    /// Result models the outcome of calling Authenticator::authenticate_with_password.
    using Result = Outcome<Token::OAuth, Error>;
    /// Callback describes the function signature of the callback that is
    /// invoked when a call to Authenticator::authenticate_with_password finishes.
    using Callback = std::function<void(const Result&)>;
  };

  /// AuthenticateAnonymously groups together types to ease interaction with
  /// Authenticator::authenticate_anonymously.
  struct AuthenticateAnonymously {
    /// The input parameters.
    using Params = Credentials::Anonymous;
    /// Result models the outcome of calling Authenticator::authenticate_anonymously.
    using Result = Outcome<Token::Anonymous, Error>;
    /// Callback describes the function signature of the callback that is
    /// invoked when a call to Authenticator::authenticate_anonymously finishes.
    using Callback = std::function<void(const Result&)>;
  };

  /// RenewAuthentication groups together types to ease interaction with
  /// Authenticator::renew_authentication.
  struct RenewAuthentication {
    /// The input parameters.
    struct Params {
      std::string client_id;                    ///< The app id for which authentication renewal is requested.
      std::string refresh_token;                ///< The refresh token for the authentication renewal request.
      GrantType grant_type{GrantType::bearer};  ///< The grant type of the authentication renewal request.
      Scope scope{Scope::open_id};              ///< The scope of the authentication renewal request.
    };
    /// Result models the outcome of calling Authenticator::renew_authentication.
    using Result = Outcome<Token::Refreshed, Error>;
    /// Callback describes the function signature of the callback that is
    /// invoked when a call to Authenticator::renew_authentication finishes.
    using Callback = std::function<void(const Result&)>;
  };

  /// authenticate_with_password authenticates the user described in 'params' with
  /// the AirMap services and reports the result to 'cb'.
  virtual void authenticate_with_password(const AuthenticateWithPassword::Params& params,
                                          const AuthenticateWithPassword::Callback& cb) = 0;

  /// authenticate_anonymously authenticates an anonymous user described by Params::user_id
  /// with the AirMap services and reports the result to 'cb'.
  virtual void authenticate_anonymously(const AuthenticateAnonymously::Params&,
                                        const AuthenticateAnonymously::Callback&) = 0;

  /// renew_authentication renews a pre-authenticated JWT as given in Params::user_id with
  /// the AirMap services and reports the result to 'cb'.
  virtual void renew_authentication(const RenewAuthentication::Params& params,
                                    const RenewAuthentication::Callback& cb) = 0;

 protected:
  /// @cond
  Authenticator() = default;
  /// @endcond
};

}  // namespace airmap

#endif  // AIRMAP_AUTHENTICATOR_H_
