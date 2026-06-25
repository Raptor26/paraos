/// @file paraos_base.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
#ifndef PARAOS_DEFERRED_DELETE_HPP
#define PARAOS_DEFERRED_DELETE_HPP

#include "paraos_config.hpp"

namespace paraos {

using base_callback = void (*)();

class base {
 public:
  explicit base(base_callback callback_ptr = nullptr)
      : callback_ptr_{callback_ptr} {}

  virtual ~base() noexcept {
    if (callback_ptr_ != nullptr) {
      callback_ptr_();
    }
  }

  /// @brief Five rule.
  base(base &&other) = delete;
  auto operator=(base &&other) -> base & = delete;
  auto operator=(const base &other) -> base & = delete;
  base(const base &other) = delete;

 private:
  base_callback callback_ptr_;
};

using Base PARAOS_DEPRECATED("use paraos::base") = base;

}  // namespace paraos

#endif /* PARAOS_DEFERRED_DELETE_HPP */
