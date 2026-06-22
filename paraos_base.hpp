/// @file paraos_base.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
#ifndef PARAOS_DEFERRED_DELETE_HPP
#define PARAOS_DEFERRED_DELETE_HPP

namespace paraos {

using base_callback = void (*)();

class Base {
 public:
  explicit Base(base_callback callback_ptr = nullptr)
      : callback_ptr_{callback_ptr} {}

  virtual ~Base() noexcept {
    if (callback_ptr_ != nullptr) {
      callback_ptr_();
    }
  }

  /// @brief Five rule.
  Base(Base &&other) = delete;
  auto operator=(Base &&other) -> Base & = delete;
  auto operator=(const Base &other) -> Base & = delete;
  Base(const Base &other) = delete;

 private:
  base_callback callback_ptr_;
};

}  // namespace paraos

#endif /* PARAOS_DEFERRED_DELETE_HPP */
