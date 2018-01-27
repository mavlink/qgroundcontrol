#ifndef AIRMAP_OUTCOME_H_
#define AIRMAP_OUTCOME_H_

#include <type_traits>

namespace airmap {

/// Outcome models a return value from a function XOR an error object
/// describing the error condition if no value can be returned.
template <typename Value, typename Error>
class Outcome {
 public:
  /// @cond
  static_assert(not std::is_same<Value, Error>::value, "Value and Error must not be the same type");
  static_assert(std::is_copy_constructible<Value>::value && std::is_move_constructible<Value>::value,
                "Value must be copy- and move-constructible");
  static_assert(std::is_copy_constructible<Error>::value && std::is_move_constructible<Error>::value,
                "Error must be copy- and move-constructible");
  /// @endcond

  /// Outcome initializes a new instance with value.
  explicit Outcome(const Value& value) : type{Type::value} {
    new (&data.value) Value{value};
  }

  /// Outcome initializes a new instance with error.
  explicit Outcome(const Error& error) : type{Type::error} {
    new (&data.error) Error{error};
  }

  /// Outcome initializes a new instance with the value or error of 'other'.
  Outcome(const Outcome& other) : type{other.type} {
    switch (type) {
      case Type::error:
        new (&data.error) Error{other.data.error};
        break;
      case Type::value:
        new (&data.value) Value{other.data.value};
        break;
    }
  }

  /// Outcome initializes a new instance with the value or error of 'other'.
  Outcome(Outcome&& other) : type{other.type} {
    switch (type) {
      case Type::error:
        new (&data.error) Error{other.data.error};
        break;
      case Type::value:
        new (&data.value) Value{other.data.value};
        break;
    }
  }

  /// Outcome assigns the value or error contained in 'other' to this instance.
  Outcome& operator=(const Outcome& other) {
    switch (type) {
      case Type::error: {
        (&data.error)->~Error();
        break;
      }
      case Type::value: {
        (&data.value)->~Value();
        break;
      }
    }

    type = other.type;

    switch (type) {
      case Type::error: {
        new (&data.error) Error{other.data.error};
        break;
      }
      case Type::value: {
        new (&data.value) Value{other.data.value};
        break;
      }
    }

    return *this;
  }

  /// Outcome assigns the value or error contained in 'other' to this instance.
  Outcome& operator=(Outcome&& other) {
    switch (type) {
      case Type::error: {
        (&data.error)->~Error();
        break;
      }
      case Type::value: {
        (&data.value)->~Value();
        break;
      }
    }

    type = other.type;

    switch (type) {
      case Type::error: {
        new (&data.error) Error{other.data.error};
        break;
      }
      case Type::value: {
        new (&data.value) Value{other.data.value};
        break;
      }
    }

    return *this;
  }

  /// ~Outcome frees up the contained error or value contained in this instance.
  ~Outcome() {
    switch (type) {
      case Type::error:
        data.error.~Error();
        break;
      case Type::value:
        data.value.~Value();
        break;
    }
  }

  /// operator bool returns true if a value is contained in this instance.
  explicit operator bool() const {
    return !has_error();
  }

  /// has_error returns true if this instance carries an error.
  inline bool has_error() const {
    return type == Type::error;
  }

  /// has_value returns true if this instance carries a value.
  inline bool has_value() const {
    return type == Type::value;
  }

  /// error returns an immutable reference to the Error contained in this instance.
  /// The result of this call is undefined if has_error() returns false.
  inline const Error& error() const {
    return data.error;
  }

  /// value returns an immutable reference to the Value contained in this instance.
  /// The result of this call is undefined if has_value() returns false.
  inline const Value& value() const {
    return data.value;
  }

 private:
  enum class Type { value, error };

  Type type;
  union Data {
    Data() : value{} {
    }

    ~Data() {
    }

    Value value;
    Error error;
  } data;
};

}  // namespace airmap

#endif  // AIRMAP_OUTCOME_H_
