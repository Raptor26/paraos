#ifndef PARAOS_STATUS_LED_HPP
#define PARAOS_STATUS_LED_HPP

#include "etl/delegate.h"
#include "paraos_thread_sequence.hpp"

namespace paraos {

enum class StatusLedMode {
  kEnable,
  kDisable,
  kIdle,
  kBlink,
  kError,

  /// Must be end of enum.
  kMaxNumb,
};

struct IStatusLed {
  virtual void Enable() const = 0;
  virtual void Disable() const = 0;
};

class StatusLed {
  using delegate_type = etl::delegate<void(void)>;

 public:
  StatusLed(
      IStatusLed &io, IThreadSequence &thread_sequence,
      StatusLedMode blink_mode = StatusLedMode::kIdle)
      : io_{io}, thread_sequence_{thread_sequence} {
    NewBlinkMode(blink_mode);
  }

  virtual ~StatusLed() {}

  auto NewBlinkMode(StatusLedMode new_blink_mode) -> bool {
    bool is_new_blink_mode_set{false};
    thread_sequence_.Unregistered(id_);
    id_ = is_new_blink_mode_set = thread_sequence_.Registered(
        delegates_arr_[static_cast<int>(new_blink_mode)],
        delegates_freq_arr_[static_cast<int>(new_blink_mode)],
        delegates_is_continuous_arr[static_cast<int>(new_blink_mode)]);

    if (id_ != etl::timer::id::NO_TIMER) {
      is_new_blink_mode_set = true;
    }

    return is_new_blink_mode_set;
  }

 private:
  void Enable() { io_.Enable(); }

  void Disable() { io_.Disable(); }

  void Idle() {
    // Toggle led.
    if (is_led_enable_) {
      is_led_enable_ = false;
      io_.Disable();
    } else {
      is_led_enable_ = true;
      io_.Enable();
    }
  }

  void Blink() {
    if (is_led_enable_) {
      io_.Disable();
      thread_sequence_.SetFreq(id_, disable_freq);

      // After a period of time, specified by the disable_freq, Blink() enable
      // led again.
      is_led_enable_ = false;
    } else {
      io_.Enable();
      thread_sequence_.SetFreq(id_, enable_freq);

      // After a period of time, specified by the enable_freq, Blink() disable
      // led.
      is_led_enable_ = true;
    }
  }

  void Error() { Idle(); }

 private:
  IStatusLed &io_;
  IThreadSequence &thread_sequence_;
  etl::timer::id::type id_{etl::timer::id::NO_TIMER};

  bool is_led_enable_{false};

  delegate_type delegates_arr_[static_cast<int>(StatusLedMode::kMaxNumb)] = {
      delegate_type::create<StatusLed, &StatusLed::Enable>(*this),
      delegate_type::create<StatusLed, &StatusLed::Disable>(*this),
      delegate_type::create<StatusLed, &StatusLed::Idle>(*this),
      delegate_type::create<StatusLed, &StatusLed::Blink>(*this),
      delegate_type::create<StatusLed, &StatusLed::Error>(*this)};

  float delegates_freq_arr_[static_cast<int>(StatusLedMode::kMaxNumb)] = {
      0.0,    // StatusLed::Enable
      0.0,    // StatusLed::Disable
      1.0,    // StatusLed::Idle
      0.0,    // StatusLed::Blink
      10.0};  // StatusLed::Error

  /// @brief Freq calculated to period befor disable led after called enable.
  /// Used in Blink().
  static constexpr float enable_freq{20.0};

  /// @brief Freq calculated to period befor enable led after called disable.
  /// Used in Blink().
  static constexpr float disable_freq{1.0};

  bool delegates_is_continuous_arr[static_cast<int>(StatusLedMode::kMaxNumb)] =
      {false,  // StatusLed::Enable
       false,  // StatusLed::Disable
       true,   // StatusLed::Idle
       true,   // StatusLed::Blink
       true};  // StatusLed::Error
};

}  // namespace paraos

#endif /* PARAOS_STATUS_LED_HPP */
