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
struct ISRbool final {
  ISRbool(bool is_success, bool is_need_switch_context = false)
      : is_success_{is_success},
        is_need_switch_context_{is_need_switch_context} {}

  ISRbool() : ISRbool{false, false} {}

  ~ISRbool() = default;

  /// --------------------------------------------------------------------------
  /// Five rule
  /// --------------------------------------------------------------------------

  ISRbool(const ISRbool &other) noexcept = default;
  ISRbool(ISRbool &&other) noexcept = default;
  ISRbool &operator=(const ISRbool &other) noexcept = default;
  ISRbool &operator=(ISRbool &&other) noexcept = default;

  /// @brief Bool flag. This value returned 'operator bool()', just like simple
  /// bool variable.
  bool is_success_{false};

  /// @brief Is true, we need switch scheduler context. Useful when
  /// semaphore/mutex api called from ISR.
  bool is_need_switch_context_{false};

  /// @brief  Behavior like as simple bool variable.
  operator bool() const { return is_success_; }
};
}  // namespace paraos

#endif /* PAROAS_ISR_HPP */
