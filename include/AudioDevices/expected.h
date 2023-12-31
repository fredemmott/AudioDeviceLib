/* Copyright (c) 2019-present, Fred Emmott
 *
 * This source code is licensed under the MIT-style license found in the
 * LICENSE file.
 */

#pragma once

#include <concepts>
#include <optional>
#include <stdexcept>

/* This file contains a simplified implementation of C++23's
 * std::expected. It will be removed when AudioDeviceLib switches
 * to targetting C++23
 */

namespace FredEmmott::Audio {

struct unexpect_t {};
inline constexpr unexpect_t unexpect {};

template <class T>
class bad_expected_access;

template <>
class bad_expected_access<void> : public std::exception {
 public:
  bad_expected_access() = default;

  virtual const char* what() const noexcept override {
    return "Bad expected access";
  };
};

template <class T>
class bad_expected_access : public bad_expected_access<void> {
 public:
  bad_expected_access(T) : bad_expected_access<void>() {
  }

  bad_expected_access() = delete;
};

template <class T, class E>
class expected {
 public:
  using value_type = T;
  using error_type = E;

  expected(value_type v) : mTag(Tag::Value), mValue(std::move(v)) {};
  expected(unexpect_t, error_type e)
    : mTag(Tag::Error), mError(std::move(e)) {};

  template <class TT, class EE>
  explicit expected(const expected<TT, EE>& other) {
    if (other.has_value()) {
      mTag = Tag::Value;
      mValue = other.value();
    } else {
      mTag = Tag::Error;
      mError = other.error();
    }
  }

  ~expected() {
    switch (mTag) {
      case Tag::Value:
        mValue.~value_type();
      case Tag::Error:
        mError.~error_type();
        break;
    }
  }

  constexpr T& value() & {
    throw_if_error();
    return mValue;
  }

  constexpr const T& value() const& {
    throw_if_error();
    return mValue;
  }

  constexpr T&& value() && {
    throw_if_error();
    return std::move(mValue);
  }

  constexpr const T&& value() const&& {
    throw_if_error();
    return std::move(mValue);
  }

  template <class D>
  constexpr T value_or(D&& default_value) const& {
    if (has_value()) {
      return mValue;
    }
    return static_cast<T>(std::forward<D>(default_value));
  }

  template <class F>
  constexpr auto and_then(F&& f) const& {
    if (has_value()) {
      return invoke(std::forward<F>(f), value());
    }
    return *this;
  }

  template <class F>
  constexpr auto and_then(F&& f) & {
    if (has_value()) {
      return invoke(std::forward<F>(f), value());
    }
    return *this;
  }

  constexpr const T* operator->() const noexcept {
    ub_if_error();
    return &mValue;
  }

  constexpr T* operator->() noexcept {
    ub_if_error();
    return &mValue;
  }

  constexpr const T& operator*() const& noexcept {
    ub_if_error();
    return mValue;
  }

  constexpr T& operator*() & noexcept {
    ub_if_error();
    return mValue;
  }

  constexpr const T&& operator*() const&& noexcept {
    ub_if_error();
    return mValue;
  }

  constexpr T&& operator*() && noexcept {
    ub_if_error();
    return mValue;
  }

  constexpr bool has_value() const noexcept {
    return mTag == Tag::Value;
  }

  /* Ban implicit conversion of `Expected<bool>` to `bool`, as it's too easy
   * to make mistakes.
   *
   * Use `has_value()` and `value()` instead.
   */
  constexpr operator bool() const noexcept requires (!std::convertible_to<bool, T>) {
    return has_value();
  }

  constexpr E& error() & noexcept {
    ub_unless_error();
    return mError;
  }

  constexpr const E& error() const& noexcept {
    ub_unless_error();
    return mError;
  }

  constexpr const bool operator==(const T& other) const noexcept {
    if (mTag != Tag::Value) {
      return false;
    }

    return mValue == other;
  }

 private:
  enum class Tag { Value, Error };

  Tag mTag;
  union {
    value_type mValue;
    error_type mError;
  };

  void throw_if_error() const {
    if (mTag == Tag::Error) {
      throw bad_expected_access(mError);
    }
  }

  void ub_unless_error() const {
    // Caller is likely noexcept; throw anyway as:
    // - (a) this *is* undefined behavior
    // - (b) it'll probably end up calling std::terminate with a clear
    //     message, which is pretty nice as far as UB goes
    if (mTag != Tag::Error) {
      throw std::logic_error("calling error() on expected with value");
    }
  }

  void ub_if_error() const {
    // Caller is likely noexcept; throw anyway as:
    // - (a) this *is* undefined behavior
    // - (b) it'll probably end up calling std::terminate with a clear
    //     message, which is pretty nice as far as UB goes
    if (mTag != Tag::Value) {
      throw std::logic_error(
        "Called method that assumes a value, but there is no value");
    }
  }
};

template <class E>
class expected<void, E> {
 public:
  using value_type = void;
  using error_type = E;

  expected() = default;

  expected(unexpect_t, E e) : mError(e) {
  }

  constexpr bool has_value() const noexcept {
    return !mError;
  }

  constexpr operator bool() const noexcept {
    return has_value();
  }

  constexpr void valid() const {
    if (mError) {
      throw bad_expected_access(mError);
    }
  }

  constexpr E& error() & noexcept {
    ub_unless_error();
    return *mError;
  }

  constexpr const E& error() const& noexcept {
    ub_unless_error();
    return *mError;
  }

 private:
  std::optional<E> mError;

  void ub_unless_error() {
    // Caller is likely noexcept; throw anyway as:
    // - (a) this *is* undefined behavior
    // - (b) it'll probably end up calling std::terminate with a clear
    //     message, which is pretty nice as far as UB goes
    if (!mError) {
      throw std::logic_error("calling error() on expected with value");
    }
  }
};

}// namespace FredEmmott::Audio
