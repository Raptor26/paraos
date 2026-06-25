/// @file paraos_status_led.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
#ifndef PARAOS_STATUS_LED_HPP
#define PARAOS_STATUS_LED_HPP

#include <array>

#include "etl/delegate.h"
#include "etl/timer.h"
#include "paraos_config.hpp"
#include "paraos_thread_sequence.hpp"

namespace paraos {

/// @brief Enumeration representing the operational modes of the status LED.
enum class status_led_mode : uint8_t {
  enable = 0,   ///< LED is enabled continuously.
  disable = 1,  ///< LED is disabled.
  idle = 2,     ///< LED is blinking as idle mode.
  blink = 3,    ///< LED is blinking as default mode.
  error = 4,    ///< LED indicates an error condition.

  /// Must be the last element in the enumeration.
  max_count = 5,

  kEnable = enable,
  kDisable = disable,
  kIdle = idle,
  kBlink = blink,
  kError = error,
  kMaxNumb = max_count,
};
using StatusLedMode PARAOS_DEPRECATED("use paraos::status_led_mode") =
    status_led_mode;

/// @brief Interface for controlling a status LED.
struct status_led_base {
  /// @brief Enables the LED.
  virtual void enable() = 0;

  /// @brief Disables the LED.
  virtual void disable() = 0;

  status_led_base() = default;
  status_led_base(const status_led_base&) = delete;
  auto operator=(const status_led_base&) -> status_led_base& = delete;
  status_led_base(status_led_base&&) = delete;
  auto operator=(status_led_base&&) -> status_led_base& = delete;

  /// @brief Destructor for the status_led_base interface.
  virtual ~status_led_base() = default;
};
using IStatusLed PARAOS_DEPRECATED("use paraos::status_led_base") =
    status_led_base;

/// @brief Class representing a status LED with various operational modes.
class status_led {
  using delegate_type = etl::delegate<void(void)>;

  static constexpr int blink_mode_max_numb =
      static_cast<int>(status_led_mode::max_count);

  /// @brief Frequency used to calculate the period before disabling the LED
  /// after enabling it. This value is used in the blink() method.
  static constexpr float enable_freq{20.0};

  /// @brief Frequency used to calculate the period before enabling the LED
  /// after disabling it. This value is used in the blink() method.
  static constexpr float disable_freq{1.0};

  /// @brief Frequency of blinking in error mode.
  static constexpr float error_blink_freq{1.0};

 public:
  /// @brief Constructs a status_led object.
  ///
  /// @param io_addr Reference to the status_led_base interface for hardware
  /// control.
  /// @param thread_sequence Reference to the thread sequence manager.
  /// @param blink_mode Initial blinking mode of the LED (default: idle).
  status_led(
      status_led_base& io_addr, thread_sequence_base& thread_sequence,
      status_led_mode blink_mode = status_led_mode::idle)
      : io_{io_addr}, thread_sequence_{thread_sequence} {
    set_blink_mode(blink_mode);
  }

  /// @brief Destructor for the status_led class.
  virtual ~status_led() = default;

  /// @brief Sets a new blinking mode for the LED.
  ///
  /// @param new_blink_mode The new blinking mode to set.
  /// @return True if the mode was successfully changed, false otherwise.
  auto set_blink_mode(status_led_mode new_blink_mode) -> bool {
    bool is_new_blink_mode_set{false};
    thread_sequence_.unregister_delegate(id_);

    auto blink_mode = static_cast<int>(new_blink_mode);
    id_ = thread_sequence_.register_delegate(
        delegate_[blink_mode].delegate_, delegate_[blink_mode].freq_,
        delegate_[blink_mode].is_continuous_);

    if (id_ != etl::timer::id::NO_TIMER) {
      is_new_blink_mode_set = true;
    }

    return is_new_blink_mode_set;
  }

  PARAOS_DEPRECATED("use set_blink_mode()")
  auto NewBlinkMode(status_led_mode new_blink_mode) -> bool {
    return set_blink_mode(new_blink_mode);
  }

  // Deleted copy and move constructors and assignment operators to prevent
  // copying.
  status_led(status_led&& other) = delete;
  auto operator=(status_led&& other) -> status_led& = delete;
  auto operator=(const status_led& other) -> status_led& = delete;
  status_led(const status_led& other) = delete;

 private:
  /// @brief Toggles the LED between enabled and disabled states.
  void idle() {
    if (is_led_enable_) {
      is_led_enable_ = false;
      disable();
    } else {
      is_led_enable_ = true;
      enable();
    }
  }

  /// @brief Controls the blinking behavior of the LED.
  ///
  /// Alternates between enabling and disabling the LED based on the configured
  /// frequencies.
  void blink() {
    if (is_led_enable_) {
      disable();
      thread_sequence_.set_frequency(id_, disable_freq);

      // After a period of time, specified by the disable_freq, blink() enables
      // the LED again.
      is_led_enable_ = false;
    } else {
      enable();
      thread_sequence_.set_frequency(id_, enable_freq);

      // After a period of time, specified by the enable_freq, blink() disables
      // the LED.
      is_led_enable_ = true;
    }
  }

  /// @brief Puts the LED into error mode.
  ///
  /// In error mode, the LED behaves similarly to the idle mode.
  PARAOS_INLINE_TRIVIAL void error() { idle(); }

  /// @brief Enables the LED.
  PARAOS_INLINE_TRIVIAL void enable() { io_.enable(); }

  /// @brief Disables the LED.
  PARAOS_INLINE_TRIVIAL void disable() { io_.disable(); }

  /// Reference to the hardware interface.
  status_led_base& io_;

  /// Reference to the thread sequence manager.
  thread_sequence_base& thread_sequence_;

  /// Timer ID for the LED control.
  etl::timer::id::type id_{etl::timer::id::NO_TIMER};

  /// Indicates whether the LED is currently enabled.
  bool is_led_enable_{false};

  /// @brief Structure representing a delegate for LED operations.
  struct status_led_delegate {
    delegate_type delegate_;  ///< Delegate function for the operation.
    float freq_;  ///< Frequency of the operation. Set to 0.0 if need calls at
                  ///< once.
    bool is_continuous_;  ///< Indicates whether the operation is continuous.
  };

  /// @brief Array of delegates for each LED mode.
  std::array<status_led_delegate, blink_mode_max_numb> delegate_ = {
      {{delegate_type::create<status_led, &status_led::enable>(*this), 0.0F,
        false},
       {delegate_type::create<status_led, &status_led::disable>(*this), 0.0F,
        false},
       {delegate_type::create<status_led, &status_led::idle>(*this), 1.0F,
        true},
       {delegate_type::create<status_led, &status_led::blink>(*this), 0.0F,
        true},
       {delegate_type::create<status_led, &status_led::error>(*this),
        error_blink_freq, true}}};
};
using StatusLed PARAOS_DEPRECATED("use paraos::status_led") = status_led;

}  // namespace paraos

#endif /* PARAOS_STATUS_LED_HPP */
