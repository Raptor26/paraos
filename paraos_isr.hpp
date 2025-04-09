/// @file paroas_isr.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// @copyright (c) 2024 Stilsoft
///
/// MIT License:
///
/// Permission is hereby granted, free of charge, to any person obtaining a copy
/// of this software and associated documentation files (the 'Software'), to
/// deal in the Software without restriction, including without limitation the
/// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
/// sell copies of the Software, and to permit persons to whom the Software is
/// furnished to do so, subject to the following conditions:
///
/// The above copyright notice and this permission notice shall be included in
/// all copies or substantial portions of the Software.
///
/// THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
/// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
/// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
/// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
/// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
/// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
/// IN THE SOFTWARE.

#ifndef PAROAS_ISR_HPP
#define PAROAS_ISR_HPP

namespace paraos {

/// @brief Contained bool value and additional bool, specified is need switch
/// RTOS contest.
/// @note Useful in API, which called from isr (xSemaphoreGiveFromISR() in
/// freeRTOS as exapmle). Semaphore::Give() with this class can transfer
/// information in called code, which can call method for switch context RTOS if
/// needed.
///
/// Disabling clang-tidy checks because the variable is_success_ is set using
/// operator=. This overload is used in other constructors without initial
/// initialization of the is_success_ field, which leads to warnings from the
/// static analyzer.
/// NOLINTBEGIN(*-member-init)
struct ISRbool final {
  explicit ISRbool(bool is_success = false, bool is_need_switch_context = false)
      : is_success_{is_success},
        is_need_switch_context_{is_need_switch_context} {}

  ~ISRbool() = default;

  /// --------------------------------------------------------------------------
  /// Five rule
  /// --------------------------------------------------------------------------

  /// @note Don't initialize is_success_ because the field initialize with
  /// operator=.
  ISRbool(const ISRbool &other) noexcept : is_need_switch_context_{false} {
    *this = other;
  };

  /// @note Don't initialize is_success_ because the field initialize with
  /// operator=.
  ISRbool(ISRbool &&other) noexcept : is_need_switch_context_{false} {
    *this = other;
  };

  auto operator=(const ISRbool &other) noexcept -> ISRbool & {
    if (&other != this) {
      is_success_ = other.is_success_;

      if (other.is_need_switch_context_) {
        is_need_switch_context_ = other.is_need_switch_context_;
      }
    }

    return *this;
  };

  auto operator=(ISRbool &&other) noexcept -> ISRbool & {
    *this = other;

    return *this;
  };

  /// @brief  Behavior like as simple bool variable.
  explicit operator bool() const { return is_success_; }

  [[nodiscard]] auto IsNeedSwitchContext() const -> bool {
    return is_need_switch_context_;
  }

  void SetSuccessStatus(bool status) { is_success_ = status; }

  void SetSwitchContextStatus(bool status) { is_need_switch_context_ = status; }

 private:
  /// @brief Bool flag. This value returned 'operator bool()', just like simple
  /// bool variable.
  bool is_success_;

  /// @brief Is true, we need switch scheduler context. Useful when
  /// semaphore/mutex api called from ISR.
  bool is_need_switch_context_;
};
/// NOLINTEND(*-member-init)
}  // namespace paraos

#endif /* PAROAS_ISR_HPP */
