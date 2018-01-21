#ifndef AIRMAP_TOKEN_H_
#define AIRMAP_TOKEN_H_

#include <airmap/optional.h>

#include <chrono>
#include <iosfwd>
#include <string>

namespace airmap {

/// Token models an authentication token required to access the AirMap services.
class Token {
 public:
  /// Type enumerates all known token types.
  enum class Type {
    unknown,    ///< Marks the unknown token type.
    anonymous,  ///< The token contains an Anonymous instance.
    oauth,      ///< The token contains an OAuth instance.
    refreshed   ///< The token contains a Refreshed instance.
  };

  /// Anonymous models a token for an anonymous authentication with the AirMap services.
  struct Anonymous {
    std::string id;  ///< The authentication id.
  };

  /// OAuth models a token for an authentication with OAuth credentials with the AirMap services.
  struct OAuth {
    enum class Type { bearer };
    Type type;            ///< The type of the OAuth token.
    std::string refresh;  ///< The refresh token for subsequent renewal requests.
    std::string id;       ///< The id token.
    std::string access;   ///< The access token.
  };

  /// Refreshed models a token for a refreshed authentication with OAuth credentials with the AirMap services.
  struct Refreshed {
    OAuth::Type type;                 ///< The type of the Refreshed token.
    std::chrono::seconds expires_in;  ///< The token expires in 'expires_in' seconds.
    std::string id;                   ///< The id token.
    Optional<OAuth> original_token;   ///< The original token used for renewal.
  };

  /// load_from_json reads a Token instance from the input stream 'in'.
  static Token load_from_json(std::istream& in);

  /// Token constructs a new invalid instance.
  explicit Token();
  /// Token constructs a new instance with Type::anonymous.
  explicit Token(const Anonymous& anonymous);
  /// Token constructs a new instance with Type::oauth.
  explicit Token(const OAuth& oauth);
  /// Token constructs a new instance with Type::refreshed.
  explicit Token(const Refreshed& refreshed);
  /// @cond
  Token(const Token& token);
  Token(Token&& token);
  ~Token();
  Token& operator=(const Token& token);
  Token& operator=(Token&& token);
  /// @endcond

  /// type returns the Type of this Token instance.
  Type type() const;
  /// id returns the common id of this Token instance.
  const std::string& id() const;
  /// anonymous returns the details for a Type::anonymous Token instance.
  const Anonymous& anonymous() const;
  /// anonymous returns the details for a Type::anonymous Token instance.
  Anonymous& anonymous();
  /// oauth returns the details for a Type::oauth Token instance.
  const OAuth& oauth() const;
  /// oauth returns the details for a Type::oauth Token instance.
  OAuth& oauth();
  /// refreshed returns the details for a Type::refreshed Token instance.
  const Refreshed& refreshed() const;
  /// refreshed returns the details for a Type::refreshed Token instance.
  Refreshed& refreshed();

 private:
  union Data {
    Data();
    ~Data();

    Anonymous anonymous;
    OAuth oauth;
    Refreshed refreshed;
  };

  Token& construct(const Token& token);
  Token& construct(const Anonymous& anonymous);
  Token& construct(const OAuth& oauth);
  Token& construct(const Refreshed& refreshed);

  Token& destruct();

  Type type_;
  Data data_;
};

/// operator<< inserts type into out.
std::ostream& operator<<(std::ostream& out, Token::Type type);
/// operator>> extracts type from in.
std::istream& operator>>(std::istream& in, Token::Type& type);
/// operator== returns true iff lhs equals rhs.
bool operator==(const Token::OAuth& lhs, const Token::OAuth& rhs);
/// operator== returns true iff lhs equals rhs.
bool operator==(Token::OAuth::Type lhs, Token::OAuth::Type rhs);
/// operator== returns true iff lhs equals rhs.
bool operator==(const Token::Refreshed& lhs, const Token::Refreshed& rhs);

}  // namespace airmap

#endif  // AIRMAP_TOKEN_H_
