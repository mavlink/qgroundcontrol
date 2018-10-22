#ifndef AIRMAP_ERROR_H_
#define AIRMAP_ERROR_H_

#include <airmap/do_not_copy_or_move.h>
#include <airmap/optional.h>

#include <cstdint>

#include <iosfwd>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace airmap {

/// Error models an error raised by an AirMap component.
struct Error {
  /// Value is a discriminated union type wrapping up multiple atomic types and
  /// their composition into a vector or a dictionary.
  class Value {
   public:
    /// Type enumerates all datatypes that can be wrapped in a Value.
    enum class Type {
      undefined,       ///< Marks the undefined type.
      boolean,         ///< Marks a boolean type.
      integer,         ///< Marks an integer type with 64 bits.
      floating_point,  ///< Marks a double-precision floating point number.
      string,          ///< Marks a string.
      blob,            ///< Marks a binary blob.
      dictionary,      ///< Marks a dictionary of values.
      vector           ///< Marks a vector of values.
    };

    /// Value initializes a new Value instance of type undefined.
    explicit Value();
    /// Value initializes a new Value instance of type boolean with 'value'.
    explicit Value(bool value);
    /// Value initializes a new Value instance of type integer with 'value'.
    explicit Value(std::int64_t value);
    /// Value initializes a new Value instance of type floating_point with 'value'.
    explicit Value(double value);
    /// Value initializes a new Value instance of type string with 'value'.
    explicit Value(const std::string& value);
    /// Value initializes a new Value instance of type blob with 'value'.
    explicit Value(const std::vector<std::uint8_t>& value);
    /// Value initializes a new Value instance of type dictionary with 'value'.
    explicit Value(const std::map<Value, Value>& value);
    /// Value initializes a new Value instance of type vector with 'value'.
    explicit Value(const std::vector<Value>& value);
    /// Value copy-constructs a value from 'other'.
    Value(const Value& other);
    /// Value move-constructs a value from 'other'.
    Value(Value&&);
    /// ~Value cleans up all resources by a Value instance.
    ~Value();

    /// operator= assigns type and value from rhs.
    Value& operator=(const Value& rhs);
    /// operator= moves type and value from rhs.
    Value& operator=(Value&& rhs);

    /// type returns the Type of this Value instance.
    Type type() const;

    /// boolean returns the boolean value of this Value instance.
    /// The behavior in case of type() != Type::boolean is undefined.
    bool boolean() const;
    /// integer returns the boolean value of this Value instance.
    /// The behavior in case of type() != Type::integer is undefined.
    std::int64_t integer() const;
    /// floating_point returns the floating point value of this Value instance.
    /// The behavior in case of type() != Type::floating_point is undefined.
    double floating_point() const;
    /// string returns the string value of this Value instance.
    /// The behavior in case of type() != Type::string is undefined.
    const std::string& string() const;
    /// blob returns the blob value of this Value instance.
    /// The behavior in case of type() != Type::blob is undefined.
    const std::vector<std::uint8_t> blob() const;
    /// dictionary returns the dictionary value of this Value instance.
    /// The behavior in case of type() != Type::dictionary is undefined.
    const std::map<Value, Value>& dictionary() const;
    /// vector returns the vector value of this Value instance.
    /// The behavior in case of type() != Type::vector is undefined.
    const std::vector<Value>& vector() const;

   private:
    union Details {
      Details();
      ~Details();

      bool boolean;
      std::int64_t integer;
      double floating_point;
      std::string string;
      std::vector<std::uint8_t> blob;
      std::map<Value, Value> dictionary;
      std::vector<Value> vector;
    };

    Value& construct(bool value);
    Value& construct(std::int64_t value);
    Value& construct(double value);
    Value& construct(const std::string& value);
    Value& construct(std::string&& value);
    Value& construct(const std::vector<std::uint8_t>& value);
    Value& construct(std::vector<std::uint8_t>&& value);
    Value& construct(const std::map<Value, Value>& value);
    Value& construct(std::map<Value, Value>&& value);
    Value& construct(const std::vector<Value>& value);
    Value& construct(std::vector<Value>&& value);
    Value& construct(const Value& value);
    Value& construct(Value&& value);
    Value& destruct();

    Type type_;
    Details details_;
  };

  /// Error initializes a new error instance with 'message'.
  explicit Error();

  /// Error initializes a new error instance with 'message'.
  explicit Error(const std::string& message);

  /// message returns the message describing an error condition.
  const std::string& message() const;
  /// message sets the message of the Error instance to 'message'.
  Error message(const std::string& message) const;
  /// message sets the message of the Error instance to 'message'.
  Error& message(const std::string& message);

  /// description returns the optional description of an error condition.
  const Optional<std::string>& description() const;
  /// clear_description resets the description of the Error instance.
  Error clear_description() const;
  /// clear_description resets the description of the Error instance.
  Error& clear_description();
  /// description sets the description of the Error instance to 'description'.
  Error description(const std::string& description) const;
  /// description sets the description of the Error instance to 'description'.
  Error& description(const std::string& description);

  /// values returns the additional values describing an error condition.
  const std::map<Value, Value>& values() const;
  /// clear_values resets the values of the Error instance.
  Error clear_values() const;
  /// clear_values resets the values of the Error instance.
  Error& clear_values();
  /// value adds the pair (key, value) to the additional values describing an error condition.
  Error value(const Value& key, const Value& value) const;
  /// value adds the pair (key, value) to the additional values describing an error condition.
  Error& value(const Value& key, const Value& value);

 private:
  std::string message_;                ///< Short, human-readable message.
  Optional<std::string> description_;  ///< Detailed description of the error, meant to be used for defect analysis.
  std::map<Value, Value> values_;      ///< Dictionary of additional data attached to the error.
};

/// operator== returns true if both type and value of lhs and rhs compare equal.
bool operator==(const Error::Value& lhs, const Error::Value& rhs);
/// operator< returns true if type and value of lhs compare < than type and value of rhs.
bool operator<(const Error::Value& lhs, const Error::Value& rhs);
/// operator<< inserts 'value' into 'out'.
std::ostream& operator<<(std::ostream& out, const Error::Value& value);
/// operator<< inserts 'error' into 'out'.
std::ostream& operator<<(std::ostream& out, const Error& error);

}  // namespace airmap

#endif  // AIRMAP_ERROR_H_