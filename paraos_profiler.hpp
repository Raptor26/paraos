/// @file paraos_profiler.hpp
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

#ifndef PARAOS_PROFILER_HPP
#define PARAOS_PROFILER_HPP

#include "paraos_check.h"
#include "paraos_config.hpp"
#include "paraos_runtime_profiler.hpp"

namespace paraos {

constexpr inline std::uint16_t runtime_profiler_hight_default{0};

/// @brief Struct with counters pointers. Use for construct 'RuntimeProfiler'
/// class.
struct RuntimeProfilerAttr {
  const std::uint16_t *low_;
  const std::uint16_t *hight_ = nullptr;
};

struct IRuntimeProfiler {
  /// @brief Start timer.
  virtual void Start() = 0;

  /// @brief Stop timer and calculate time between Start() and Stop() calls.
  ///
  /// @return Time between Start() and Stop() calls.
  virtual auto Stop() -> cnt_t = 0;

  /// @brief Returned value, which calculated wen user calls Stop().
  ///
  /// @return Time between last Start() and Stop() calls.
  virtual auto LastDuration() -> cnt_t = 0;

  virtual void ConnectTimer(const RuntimeProfilerAttr &timer) = 0;
};

/// @brief  Runtime profiler, may be 16 or 32 bits counter.
struct RuntimeProfiler final : public IRuntimeProfiler {
  RuntimeProfiler(
      const std::uint16_t *low, const std::uint16_t *hight = nullptr)
      : low_{low}, hight_{hight} {
    // low ptr can't be nullptr.
    PARAOS_CHECK_ASSERT(low);
  }

  RuntimeProfiler(RuntimeProfilerAttr &timer)
      : RuntimeProfiler{timer.low_, timer.hight_} {}

  virtual ~RuntimeProfiler() = default;

  /// @brief Start timer. When Stop() will calls, duration between Start() and
  /// Stop() calls be calculate.
  PARAOS_INLINE_OPERATIONS void Start() override {
    start_ = GiveCnt();

    // Необходимо сбросить счетчик переполнений чтобы при повторном вызове
    // Stop() не учитывать уже учтенное переполнение.
    overflow_cnt_ = 0u;
  }

  /// @brief Calculate duration between Start() and Stop() calls.
  /// @return Duration between Start() and Stop() calls.
  auto Stop() -> cnt_t override {
    cnt_t stop = GiveCnt();

    if (start_ > stop) {
      ++overflow_cnt_;
    }

    // Вычисление периода между вызовами Start() и Stop() с учетом переполнения.
    const auto overflow_value = GiveCntOverflowValue();

    duration_ = static_cast<cnt_t>(
        (overflow_value * overflow_cnt_) + (stop - start_) + overflow_cnt_);

    return LastDuration();
  }

  PARAOS_INLINE_TRIVIAL auto LastDuration() -> cnt_t override {
    return static_cast<cnt_t>(duration_);
  }

  void ConnectTimer(const RuntimeProfilerAttr &timer) override {
    low_ = timer.low_;
    hight_ = timer.hight_;
  }

 private:
  cnt_t GiveCnt() {
    if (hight_) {
      return GiveCnt32();
    } else {
      return GiveCnt16();
    }
  }

  cnt_t GiveCnt32() {
    constexpr cnt_t hight_mask = 0xFFFF0000;
    constexpr cnt_t low_mask = 0x0000FFFF;
    constexpr cnt_t bites_shift = 16u;
    return (
        (((static_cast<cnt_t>(*hight_)) << bites_shift) & hight_mask) |
        ((static_cast<cnt_t>(*low_)) & low_mask));
  }

  cnt_t GiveCnt16() { return static_cast<cnt_t>(*low_); }

  PARAOS_INLINE_TRIVIAL cnt_t GiveCntOverflowValue() const {
    if (hight_) {
      return GiveCntOverflowValue32();
    } else {
      return GiveCntOverflowValue16();
    }
  }

  constexpr PARAOS_INLINE_TRIVIAL cnt_t GiveCntOverflowValue32() const {
    return std::numeric_limits<std::uint32_t>::max();
  }

  constexpr PARAOS_INLINE_TRIVIAL cnt_t GiveCntOverflowValue16() const {
    return std::numeric_limits<std::uint16_t>::max();
  }

 private:
  const std::uint16_t *low_;
  const std::uint16_t *hight_;

  cnt_t start_{0u};
  cnt_t duration_{0u};
  cnt_t overflow_cnt_{0u};
};

}  // namespace paraos

#endif /* PARAOS_PROFILER_HPP */
