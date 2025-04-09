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
#include "etl/timer.h"
#include "paraos_config.hpp"
#include "paraos_thread_sequence.hpp"

namespace paraos {

/// @brief Enumeration representing the operational modes of the status LED.
enum class StatusLedMode : uint8_t {
  kEnable = 0,  ///< LED is enabled continuously.
  kDisable,     ///< LED is disabled.
  kIdle,        ///< LED is blinking as idle mode.
  kBlink,       ///< LED is blinking as default mode.
  kError,       ///< LED indicates an error condition.

  /// Must be the last element in the enumeration.
  kMaxNumb,
};

/// @brief Interface for controlling a status LED.
struct IStatusLed {
  /// @brief Enables the LED.
  virtual void Enable() = 0;

  /// @brief Disables the LED.
  virtual void Disable() = 0;
};

/// @brief Class representing a status LED with various operational modes.
class StatusLed {
  using delegate_type = etl::delegate<void(void)>;

  static constexpr int blink_mode_max_numb =
      static_cast<int>(StatusLedMode::kMaxNumb);

  /// @brief Frequency used to calculate the period before disabling the LED
  /// after enabling it. This value is used in the Blink() method.
  static constexpr float enable_freq{20.0};

  /// @brief Frequency used to calculate the period before enabling the LED
  /// after disabling it. This value is used in the Blink() method.
  static constexpr float disable_freq{1.0};

  /// @brief Frequency of blinking in error mode.
  static constexpr float error_blink_freq{1.0};

 public:
  /// @brief Constructs a StatusLed object.
  ///
  /// @param io_addr Reference to the IStatusLed interface for hardware control.
  /// @param thread_sequence Reference to the thread sequence manager.
  /// @param blink_mode Initial blinking mode of the LED (default: kIdle).
  StatusLed(
      IStatusLed &io_addr, IThreadSequence &thread_sequence,
      StatusLedMode blink_mode = StatusLedMode::kIdle)
      : io_{io_addr}, thread_sequence_{thread_sequence} {
    NewBlinkMode(blink_mode);
  }

  /// @brief Destructor for the StatusLed class.
  virtual ~StatusLed();

  /// @brief Sets a new blinking mode for the LED.
  ///
  /// @param new_blink_mode The new blinking mode to set.
  /// @return True if the mode was successfully changed, false otherwise.
  auto NewBlinkMode(StatusLedMode new_blink_mode) -> bool {
    bool is_new_blink_mode_set{false};
    thread_sequence_.Unregister(id_);

    auto blink_mode = static_cast<int>(new_blink_mode);
    id_ = thread_sequence_.Register(
        delegate_[blink_mode].delegate_, delegate_[blink_mode].freq_,
        delegate_[blink_mode].is_continuous_);

    if (id_ != etl::timer::id::NO_TIMER) {
      is_new_blink_mode_set = true;
    }

    return is_new_blink_mode_set;
  }

  // Deleted copy and move constructors and assignment operators to prevent
  // copying.
  StatusLed(StatusLed &&other) = delete;
  auto operator=(StatusLed &&other) -> StatusLed & = delete;
  auto operator=(const StatusLed &other) -> StatusLed & = delete;
  StatusLed(const StatusLed &other) = delete;

 private:
  /// @brief Enables the LED.
  PARAOS_INLINE_TRIVIAL void Enable() { io_.Enable(); }

  /// @brief Disables the LED.
  PARAOS_INLINE_TRIVIAL void Disable() { io_.Disable(); }

  /// @brief Toggles the LED between enabled and disabled states.
  void Idle() {
    if (is_led_enable_) {
      is_led_enable_ = false;
      io_.Disable();
    } else {
      is_led_enable_ = true;
      io_.Enable();
    }
  }

  /// @brief Controls the blinking behavior of the LED.
  ///
  /// Alternates between enabling and disabling the LED based on the configured
  /// frequencies.
  void Blink() {
    if (is_led_enable_) {
      io_.Disable();
      thread_sequence_.SetFreq(id_, disable_freq);

      // After a period of time, specified by the disable_freq, Blink() enables
      // the LED again.
      is_led_enable_ = false;
    } else {
      io_.Enable();
      thread_sequence_.SetFreq(id_, enable_freq);

      // After a period of time, specified by the enable_freq, Blink() disables
      // the LED.
      is_led_enable_ = true;
    }
  }

  /// @brief Puts the LED into error mode.
  ///
  /// In error mode, the LED behaves similarly to the Idle mode.
  PARAOS_INLINE_TRIVIAL void Error() { Idle(); }

 private:
  /// Reference to the hardware interface.
  IStatusLed &io_;

  /// Reference to the thread sequence manager.
  IThreadSequence &thread_sequence_;

  /// Timer ID for the LED control.
  etl::timer::id::type id_{etl::timer::id::NO_TIMER};

  /// Indicates whether the LED is currently enabled.
  bool is_led_enable_{false};

  /// @brief Structure representing a delegate for LED operations.
  struct StatusLedDelegate {
    delegate_type delegate_;  ///< Delegate function for the operation.
    float freq_;  ///< Frequency of the operation. Set to 0.0 if need calls at
                  ///< once.
    bool is_continuous_;  ///< Indicates whether the operation is continuous.
  };

  /// @brief Array of delegates for each LED mode.
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