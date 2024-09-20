/// @file paraos_status_led.cpp
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

#include "paraos_status_led.hpp"

namespace paraos {
StatusLed::StatusLed(
    IStatusLed &io, IThreadSequence &thread_sequence, StatusLedMode blink_mode)
    : io_{io}, thread_sequence_{thread_sequence} {
  NewBlinkMode(blink_mode);
}

StatusLed::~StatusLed() {}

auto StatusLed::NewBlinkMode(StatusLedMode new_blink_mode) -> bool {
  bool is_new_blink_mode_set{false};
  thread_sequence_.Unregistered(id_);

  auto blink_mode = static_cast<int>(new_blink_mode);
  id_ = is_new_blink_mode_set = thread_sequence_.Registered(
      delegate_[blink_mode].delegate_, delegate_[blink_mode].freq_,
      delegate_[blink_mode].is_continuous_);

  if (id_ != etl::timer::id::NO_TIMER) {
    is_new_blink_mode_set = true;
  }

  return is_new_blink_mode_set;
}

void StatusLed::Enable() { io_.Enable(); }

void StatusLed::Disable() { io_.Disable(); }

void StatusLed::Idle() {
  // Toggle led.
  if (is_led_enable_) {
    is_led_enable_ = false;
    io_.Disable();
  } else {
    is_led_enable_ = true;
    io_.Enable();
  }
}

void StatusLed::Blink() {
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

void StatusLed::Error() { Idle(); }
}  // namespace paraos
