/// @file paraos_status_led.hpp
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

#ifndef PARAOS_STATUS_LED_HPP
#define PARAOS_STATUS_LED_HPP

#include <array>

#include "etl/delegate.h"
#include "paraos_thread_sequence.hpp"

namespace paraos {

enum class StatusLedMode : uint8_t {
  kEnable = 0,
  kDisable,
  kIdle,
  kBlink,
  kError,

  /// Must be end of enum.
  kMaxNumb,
};

struct IStatusLed {
  virtual void Enable() = 0;
  virtual void Disable() = 0;
};

class StatusLed {
  using delegate_type = etl::delegate<void(void)>;

  static constexpr int blink_mode_max_numb =
      static_cast<int>(StatusLedMode::kMaxNumb);

  /// @brief Freq calculated to period befor disable led after enable was
  /// called. Used in Blink().
  static constexpr float enable_freq{20.0};

  /// @brief Freq calculated to period befor enable led after disable was
  /// called. Used in Blink().
  static constexpr float disable_freq{1.0};

  /// @brief Freq of blinking in error mode.
  static constexpr float error_blink_freq{1.0};

 public:
  StatusLed(
      IStatusLed &io_addr, IThreadSequence &thread_sequence,
      StatusLedMode blink_mode = StatusLedMode::kIdle);

  virtual ~StatusLed();

  auto NewBlinkMode(StatusLedMode new_blink_mode) -> bool;

  StatusLed(StatusLed &&other) = delete;
  auto operator=(StatusLed &&other) -> StatusLed & = delete;
  auto operator=(const StatusLed &other) -> StatusLed & = delete;
  StatusLed(const StatusLed &other) = delete;

 private:
  void Enable();

  void Disable();

  void Idle();

  void Blink();

  void Error();

 private:
  IStatusLed &io_;
  IThreadSequence &thread_sequence_;
  etl::timer::id::type id_{etl::timer::id::NO_TIMER};

  bool is_led_enable_{false};

  struct StatusLedDelegate {
    delegate_type delegate_;
    float freq_;
    bool is_continuous_;
  };

  std::array<StatusLedDelegate, blink_mode_max_numb> delegate_ = {
      {{delegate_type::create<StatusLed, &StatusLed::Enable>(*this), 0.0,
        false},
       {delegate_type::create<StatusLed, &StatusLed::Disable>(*this), 0.0,
        false},
       {delegate_type::create<StatusLed, &StatusLed::Idle>(*this), 1.0, true},
       {delegate_type::create<StatusLed, &StatusLed::Blink>(*this), 0.0, true},
       {delegate_type::create<StatusLed, &StatusLed::Error>(*this),
        error_blink_freq, true}}};
};

}  // namespace paraos

#endif /* PARAOS_STATUS_LED_HPP */
