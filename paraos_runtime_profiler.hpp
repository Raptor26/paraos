/// @file runtime_profiler.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// @copyright Copyright (c) 2024 Stilsoft
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

#ifndef PARAOS_RUNTIME_PROFILER_HPP
#define PARAOS_RUNTIME_PROFILER_HPP

#if defined(_WIN32) || defined(_WIN64) || defined(__linux__) || \
    defined(__unix__)
#include <chrono>
using std::chrono::duration_cast;
using std::chrono::high_resolution_clock;
using time_resolution = std::chrono::microseconds;
using namespace std::chrono_literals;
#endif

#include <cstddef>
#include <cstdint>
#include <limits>

#include "paraos_config.hpp"

namespace paraos {

using cnt_t = std::uint32_t;

struct IProfiler {
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
};

/// @brief Embedded timer interface. Need for get actual timer value and use it
/// for calculate runtime in EmbeddedProfiler() and TimerProfiler().
///
/// @note In more cases, user code create one instance of IEmbeddedTimer
/// inteface and use it with many instances of EmbeddedProfiler() and
/// TimerProfiler().
struct IEmbeddedTimer {
  virtual ~IEmbeddedTimer() = default;
  virtual cnt_t GiveCnt() const = 0;
  virtual cnt_t GiveCntOverflowValue() const = 0;
};

/// @brief Override IEmbeddedTimer interface with zero values.
struct EmbeddedTimerEmpty final : public IEmbeddedTimer {
  cnt_t GiveCnt() const override { return 0u; };
  cnt_t GiveCntOverflowValue() const override { return 0u; };
};

/// Empty profiler -------------------------------------------------------------

/// @brief "Пустой" профилировщик. Используется в качестве профилировщика "по
/// умолчанию".
struct EmptyProfiler final : public IProfiler {
  PARAOS_INLINE_TRIVIAL void Start() override {}
  PARAOS_INLINE_TRIVIAL auto Stop() -> cnt_t override { return 0u; }
  PARAOS_INLINE_TRIVIAL auto LastDuration() -> cnt_t override { return 0u; }
};

/// @brief Empty profiler instance. Use if need set reference on IProfiler
/// without real profiler.
inline EmptyProfiler empty_profiler;

/// Operation system high resolution timer profiler ----------------------------

#if defined(_WIN32) || defined(_WIN64) || defined(__linux__) || \
    defined(__unix__)
/// @brief Профилировщик, предназначенный для использования в операционных
/// системах общего назначения.
struct OsProfiler final {
  PARAOS_INLINE_OPERATIONS void Start() {
    start_ = high_resolution_clock::now();
  }

  std::size_t Stop() {
    end_ = high_resolution_clock::now();
    duration_ = duration_cast<time_resolution>(end_ - start_).count();
    return LastDuration();
  }
  [[nodiscard]] std::size_t LastDuration() const { return duration_; }

  [[nodiscard]] std::size_t LastDurationMs() const {
    return LastDuration() / 1000;
  }

 private:
  decltype(high_resolution_clock::now()) start_;
  decltype(high_resolution_clock::now()) end_;
  decltype(duration_cast<time_resolution>(end_ - start_).count()) duration_{0u};
};
#endif  // #if defined(_WIN32) || defined(_WIN64) || defined(__linux__) ||
        // defined(__unix__)

/// Embedded Profiler ----------------------------------------------------------

struct HightCntDefault {};

/// @brief Шаблон 32-х битного счетчика.
///
/// @warning Пользовательский код не использует данную структуру, она
/// применяется в 'EmbeddedProfiler'.
///
/// @tparam LOW_ADDR Структура, используемая для получения указателя на младшие
/// 16 бит счетчика.
/// @tparam HIGHT_ADDR Структура, используемая для получения указателя на
/// старшие 16 бит счетчика.
template <typename LOW_ADDR, typename HIGHT_ADDR>
struct EmbeddedTimer : public IEmbeddedTimer {
  EmbeddedTimer() = default;

  /// @brief Деструктор "по умолчанию".
  /// @note Деструктор явно не объявлен виртуальным т.к. "EmbeddedTimer" и
  /// "EmbeddedProfiler" не выделяют динамических ресурсов.
  ~EmbeddedTimer() = default;

  EmbeddedTimer(const EmbeddedTimer &other) = default;
  EmbeddedTimer &operator=(const EmbeddedTimer &other) = default;

  EmbeddedTimer(EmbeddedTimer &&other) = default;
  EmbeddedTimer &operator=(EmbeddedTimer &&other) = default;

  /// @brief Возвращает значение 32-х битного аппаратного счетчика на момент
  /// вызова.
  /// @return Возвращает переменную типа 'cnt_t' содержащую значение аппаратного
  /// счетчика на момент вызова.
  PARAOS_INLINE_TRIVIAL cnt_t GiveCnt() const override {
    constexpr cnt_t hight_mask = 0xFFFF0000;
    constexpr cnt_t low_mask = 0x0000FFFF;
    constexpr cnt_t bites_shift = 16u;
    return (
        (((static_cast<cnt_t>(*hight)) << bites_shift) & hight_mask) |
        ((static_cast<cnt_t>(*low)) & low_mask));
  }

  PARAOS_INLINE_TRIVIAL cnt_t GiveCntOverflowValue() const override {
    return std::numeric_limits<std::uint32_t>::max();
  }

 private:
  volatile std::uint16_t *hight = HIGHT_ADDR{}();
  volatile std::uint16_t *low = LOW_ADDR{}();
};

/// @brief Специализация шаблона счетчика в случае использования 16-ти битного
/// счетчика (вместо 32-х битного).
/// @warning Пользовательский код не использует данную структуру, она
/// применяется в 'EmbeddedProfiler'.
/// @tparam LOW_ADDR Структура, используемая для получения указателя на младшие
/// 16 бит счетчика.
template <typename LOW_ADDR>
struct EmbeddedTimer<LOW_ADDR, HightCntDefault> : public IEmbeddedTimer {
  EmbeddedTimer() = default;

  /// @brief Деструктор "по умолчанию".
  /// @note Деструктор явно не объявлен виртуальным т.к. "EmbeddedTimer" и
  /// "EmbeddedProfiler" не выделяют динамических ресурсов.
  ~EmbeddedTimer() = default;

  EmbeddedTimer(const EmbeddedTimer &other) = default;
  EmbeddedTimer &operator=(const EmbeddedTimer &other) = default;

  EmbeddedTimer(EmbeddedTimer &&other) = default;
  EmbeddedTimer &operator=(EmbeddedTimer &&other) = default;

  /// @brief Возвращает значение 16-х битного аппаратного счетчика на момент
  /// вызова.
  /// @return Возвращает переменную типа 'cnt_t' содержащую значение аппаратного
  /// счетчика на момент вызова.
  PARAOS_INLINE_TRIVIAL cnt_t GiveCnt() const override {
    return static_cast<cnt_t>(*low);
  }

  PARAOS_INLINE_TRIVIAL cnt_t GiveCntOverflowValue() const override {
    return std::numeric_limits<std::uint16_t>::max();
  }

 private:
  volatile std::uint16_t *low = LOW_ADDR{}();
};

/// @brief Профилировщик, предназначенный для использования во встраиваемых
/// системах.
/// @tparam LOW_ADDR Структура, используемая для получения указателя на младшие
/// 16 бит счетчика.
/// @tparam HIGHT_ADDR Структура, используемая для получения указателя на
/// старшие 16 бит счетчика.
template <typename LOW_ADDR, typename HIGHT_ADDR = HightCntDefault>
struct EmbeddedProfiler final : public IProfiler,
                                public EmbeddedTimer<LOW_ADDR, HIGHT_ADDR> {
  PARAOS_INLINE_OPERATIONS void Start() override {
    start_ = EmbeddedTimer<LOW_ADDR, HIGHT_ADDR>::GiveCnt();

    // Необходимо сбросить счетчик переполнений чтобы при повторном вызове
    // Stop() не учитывать уже учтенное переполнение.
    overflow_cnt_ = 0u;
  }

  cnt_t Stop() override {
    cnt_t stop = EmbeddedTimer<LOW_ADDR, HIGHT_ADDR>::GiveCnt();

    if (start_ > stop) {
      ++overflow_cnt_;
    }

    // Вычисление периода между вызовами Start() и Stop() с учетом переполнения.
    const auto &overflow_value =
        EmbeddedTimer<LOW_ADDR, HIGHT_ADDR>::GiveCntOverflowValue();
    duration_ = static_cast<cnt_t>(
        (overflow_value * overflow_cnt_) + (stop - start_) + overflow_cnt_);

    return LastDuration();
  }

  PARAOS_INLINE_TRIVIAL cnt_t LastDuration() override {
    return static_cast<cnt_t>(duration_);
  }

 private:
  cnt_t start_{0u};
  cnt_t duration_{0u};
  cnt_t overflow_cnt_{0u};
};

/// @brief Instance of EmbeddedTimerEmpty with override interface methods which
/// return zero values.
inline EmbeddedTimerEmpty embedded_timer_empty;

/// @brief Runtime profiler based on user definition counter. User code use
/// Ctor() or SetEmbeddedTimer() for connect one embedded timer to the
/// TimerProfiler instance.
///
/// @note TimerProfiler don't use any template argument.
struct TimerProfiler final : public IProfiler {
  /// @brief Ctor with embedded timer reference.

  /// @note Many TimerProfiler instances can use one IEmbeddedTimer instance.
  ///
  /// @param[in] timer: New timer for connect to the profiler. If use default
  /// value, Stop() and LastDuration() return zero value. For this reason user
  /// code must call SetEmbeddedTimer() and set valid embedded timer reference.
  TimerProfiler(const IEmbeddedTimer &timer = embedded_timer_empty)
      : timer_{&timer} {}

  /// @brief Connect new embedded timer to the profiler.
  ///
  /// @note User code must call this method if Ctor use default params. In
  /// otherwise Stop() and LastDuration() return zero value.
  ///
  /// @param[in] timer: New timer for connect to the profiler.
  void SetEmbeddedTimer(const IEmbeddedTimer &timer) { timer_ = &timer; }

  /// @brief Start
  ///
  /// @return None
  PARAOS_INLINE_OPERATIONS void Start() override {
    start_ = timer_->GiveCnt();

    // Необходимо сбросить счетчик переполнений чтобы при повторном вызове
    // Stop() не учитывать уже учтенное переполнение.
    overflow_cnt_ = 0u;
  }

  /// @brief Stop timer.
  ///
  /// @return Value between Start() and Stop() calls.
  cnt_t Stop() override {
    cnt_t stop = timer_->GiveCnt();

    if (start_ > stop) {
      ++overflow_cnt_;
    }

    // Вычисление периода между вызовами Start() и Stop() с учетом переполнения.
    const auto &overflow_value = timer_->GiveCntOverflowValue();
    duration_ = static_cast<cnt_t>(
        (overflow_value * overflow_cnt_) + (stop - start_) + overflow_cnt_);

    return LastDuration();
  }

  /// @brief Return value between Start() and Stop() calls.
  ///
  /// @return Value between Start() and Stop() calls.
  PARAOS_INLINE_TRIVIAL cnt_t LastDuration() override {
    return static_cast<cnt_t>(duration_);
  }

 private:
  cnt_t start_{0u};
  cnt_t duration_{0u};
  cnt_t overflow_cnt_{0u};

  /// @brief Interface for get actual counter value on each time.
  const IEmbeddedTimer *timer_;
};

}  // namespace paraos

#endif /* PARAOS_RUNTIME_PROFILER_HPP */
