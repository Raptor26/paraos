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
// Can't use duration cast etc. directly because these namings are platform
// dependent. Also, long names with namespaces are bad for readability.
// NOLINTBEGIN(google-global-names-in-headers)
using std::chrono::duration_cast;
using std::chrono::high_resolution_clock;
using time_resolution = std::chrono::microseconds;
using namespace std::chrono_literals;
// NOLINTEND(google-global-names-in-headers)
#endif

#include <cstddef>
#include <cstdint>
#include <limits>

#include "paraos_config.hpp"

namespace paraos {

using cnt_t = std::uint32_t;

/// @brief Количество микросекунд в одной миллисекунде.
constexpr cnt_t us_in_ms{1000};

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

/// @brief RAII class for automaticaly start and stop profiler.
///
/// @note In some cases, we need profiler some section. In this case user
/// manually run Start() and Stop() methods. Disadvantage of this approach:
/// - if throw exception between Start() and Stop(), Stop() never will calling.
/// - if need get full runtime of some method, we can't get runtime with return
/// operator.
///
/// Using RAII class you will overcome these disadvantages.
///
/// @example For usage example see TEST_F(Profiler, RAII) in
/// test_runtime_profiler.cpp
class ProfilerRAII final {
 public:
  explicit ProfilerRAII(IProfiler &profiler) : profiler_{profiler} {
    profiler_.Start();
  }

  ~ProfilerRAII() { profiler_.Stop(); }

  ProfilerRAII(ProfilerRAII &&other) = delete;
  auto operator=(ProfilerRAII &&other) -> ProfilerRAII & = delete;
  auto operator=(const ProfilerRAII &other) -> ProfilerRAII & = delete;
  ProfilerRAII(const ProfilerRAII &other) = delete;

 private:
  IProfiler &profiler_;
};

/// @brief RAII class for calculate period between ProfilerPeriodRAII calls.
///
/// @note If you need calculate period calling some bloc code, use
/// ProfilerPeriodRAII class.
class ProfilerPeriodRAII final {
 public:
  explicit ProfilerPeriodRAII(IProfiler &profiler) {
    profiler.Stop();
    profiler.Start();
  }

  ~ProfilerPeriodRAII() = default;

  ProfilerPeriodRAII(ProfilerPeriodRAII &&other) = delete;
  auto operator=(ProfilerPeriodRAII &&other) -> ProfilerPeriodRAII & = delete;
  auto operator=(const ProfilerPeriodRAII &other)
      -> ProfilerPeriodRAII & = delete;
  ProfilerPeriodRAII(const ProfilerPeriodRAII &other) = delete;
};

/// @brief Embedded timer interface. Need for get actual timer value and use it
/// for calculate runtime in EmbeddedProfiler() and TimerProfiler().
///
/// @note In more cases, user code create one instance of IEmbeddedTimer
/// inteface and use it with many instances of EmbeddedProfiler() and
/// TimerProfiler().
struct IEmbeddedTimer {
  virtual ~IEmbeddedTimer() = default;
  [[nodiscard]] virtual auto GiveCnt() const -> cnt_t = 0;
  [[nodiscard]] virtual auto GiveCntOverflowValue() const -> cnt_t = 0;

  /// @brief Five rule.
  IEmbeddedTimer(IEmbeddedTimer &&other) = delete;
  auto operator=(IEmbeddedTimer &&other) -> IEmbeddedTimer & = delete;
  auto operator=(const IEmbeddedTimer &other) -> IEmbeddedTimer & = delete;
  IEmbeddedTimer(const IEmbeddedTimer &other) = delete;

 protected:
  IEmbeddedTimer() = default;
};

/// @brief Override IEmbeddedTimer interface with zero values.
struct EmbeddedTimerEmpty final : public IEmbeddedTimer {
  [[nodiscard]] auto GiveCnt() const -> cnt_t override { return 0U; };
  [[nodiscard]] auto GiveCntOverflowValue() const -> cnt_t override {
    return 0U;
  };
};

/// Empty profiler -------------------------------------------------------------

/// @brief "Пустой" профилировщик. Используется в качестве профилировщика "по
/// умолчанию".
struct EmptyProfiler final : public IProfiler {
  PARAOS_INLINE_TRIVIAL void Start() override {}
  PARAOS_INLINE_TRIVIAL auto Stop() -> cnt_t override { return 0U; }
  PARAOS_INLINE_TRIVIAL auto LastDuration() -> cnt_t override { return 0U; }
};

/// @brief Empty profiler instance. Use if need set reference on IProfiler
/// without real profiler.
inline EmptyProfiler empty_profiler;

/// Operation system high resolution timer profiler ----------------------------

#if defined(_WIN32) || defined(_WIN64) || defined(__linux__) || \
    defined(__unix__)
/// @brief Профилировщик, предназначенный для использования в операционных
/// системах общего назначения.
struct OsProfiler final : public IProfiler {
  PARAOS_INLINE_OPERATIONS void Start() override {
    start_ = high_resolution_clock::now();
  }

  auto Stop() -> cnt_t override {
    end_ = high_resolution_clock::now();
    duration_ = duration_cast<time_resolution>(end_ - start_).count();
    return LastDuration();
  }

  auto LastDuration() -> cnt_t override { return duration_; }

  auto LastDurationMs() -> cnt_t {
    return static_cast<cnt_t>(LastDuration() / us_in_ms);
  }

 private:
  decltype(high_resolution_clock::now()) start_;
  decltype(high_resolution_clock::now()) end_;
  decltype(duration_cast<time_resolution>(end_ - start_).count()) duration_{0U};
};

/// @brief Profiler timer if run on operation system (like as windows or linux).
struct OsTimer final : public IEmbeddedTimer {
  OsTimer() {
    // Write current time in private field. Useful when user calls GiveCnt().
    start_ = high_resolution_clock::now();
  }

  ~OsTimer() override = default;

  [[nodiscard]] auto GiveCnt() const -> cnt_t override {
    auto end = high_resolution_clock::now();

    // Calculate durations between OsTimer constructor and now time when
    // user code calls GiveCnt(). We don't use
    // 'high_resolution_clock::now().time_since_epoch().count()' because in this
    // case return value be in ticks, but we want microseconds.
    auto duration = duration_cast<time_resolution>(end - start_).count();

    // In this case we tracking counter overflow, because duration has large bit
    // depth relative cnt_t type (imitate cnt_t type bits counter with hardware
    // overflow).
    return static_cast<cnt_t>(duration % std::numeric_limits<cnt_t>::max());
  };

  [[nodiscard]] auto GiveCntOverflowValue() const -> cnt_t override {
    return std::numeric_limits<cnt_t>::max();
  };

  /// @brief Five rule.
  OsTimer(OsTimer &&other) = delete;
  auto operator=(OsTimer &&other) -> OsTimer & = delete;
  auto operator=(const OsTimer &other) -> OsTimer & = delete;
  OsTimer(const OsTimer &other) = delete;

 private:
  decltype(high_resolution_clock::now()) start_;
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
  ~EmbeddedTimer() override = default;

  EmbeddedTimer(const EmbeddedTimer &other) = default;
  auto operator=(const EmbeddedTimer &other) -> EmbeddedTimer & = default;

  EmbeddedTimer(EmbeddedTimer &&other) noexcept = default;
  auto operator=(EmbeddedTimer &&other) noexcept -> EmbeddedTimer & = default;

  /// @brief Возвращает значение 32-х битного аппаратного счетчика на момент
  /// вызова.
  /// @return Возвращает переменную типа 'cnt_t' содержащую значение аппаратного
  /// счетчика на момент вызова.
  [[nodiscard]] PARAOS_INLINE_TRIVIAL auto GiveCnt() const -> cnt_t override {
    constexpr cnt_t hight_mask = 0xFFFF0000;
    constexpr cnt_t low_mask = 0x0000FFFF;
    constexpr cnt_t bites_shift = 16U;
    return (
        (((static_cast<cnt_t>(*hight)) << bites_shift) & hight_mask) |
        ((static_cast<cnt_t>(*low)) & low_mask));
  }

  [[nodiscard]] PARAOS_INLINE_TRIVIAL auto GiveCntOverflowValue() const
      -> cnt_t override {
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
  ~EmbeddedTimer() override = default;

  EmbeddedTimer(const EmbeddedTimer &other) = default;
  auto operator=(const EmbeddedTimer &other) -> EmbeddedTimer & = default;

  EmbeddedTimer(EmbeddedTimer &&other) noexcept = default;
  auto operator=(EmbeddedTimer &&other) noexcept -> EmbeddedTimer & = default;

  /// @brief Возвращает значение 16-х битного аппаратного счетчика на момент
  /// вызова.
  /// @return Возвращает переменную типа 'cnt_t' содержащую значение аппаратного
  /// счетчика на момент вызова.
  [[nodiscard]] PARAOS_INLINE_TRIVIAL auto GiveCnt() const -> cnt_t override {
    return static_cast<cnt_t>(*low);
  }

  [[nodiscard]] PARAOS_INLINE_TRIVIAL auto GiveCntOverflowValue() const
      -> cnt_t override {
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
    overflow_cnt_ = 0U;
  }

  auto Stop() -> cnt_t override {
    const cnt_t stop = EmbeddedTimer<LOW_ADDR, HIGHT_ADDR>::GiveCnt();

    if (start_ > stop) {
      ++overflow_cnt_;
    }

    // Вычисление периода между вызовами Start() и Stop() с учетом переполнения.
    const auto &overflow_value =
        EmbeddedTimer<LOW_ADDR, HIGHT_ADDR>::GiveCntOverflowValue();
    duration_ =
        (overflow_value * overflow_cnt_) + (stop - start_) + overflow_cnt_;

    return LastDuration();
  }

  PARAOS_INLINE_TRIVIAL auto LastDuration() -> cnt_t override {
    return duration_;
  }

 private:
  cnt_t start_{0U};
  cnt_t duration_{0U};
  cnt_t overflow_cnt_{0U};
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
  explicit TimerProfiler(const IEmbeddedTimer &timer = embedded_timer_empty)
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
    overflow_cnt_ = 0U;
  }

  /// @brief Stop timer.
  ///
  /// @return Value between Start() and Stop() calls.
  auto Stop() -> cnt_t override {
    const cnt_t stop = timer_->GiveCnt();

    if (start_ > stop) {
      ++overflow_cnt_;
    }

    // Вычисление периода между вызовами Start() и Stop() с учетом переполнения.
    const auto &overflow_value = timer_->GiveCntOverflowValue();
    duration_ =
        (overflow_value * overflow_cnt_) + (stop - start_) + overflow_cnt_;

    return LastDuration();
  }

  /// @brief Return value between Start() and Stop() calls.
  ///
  /// @return Value between Start() and Stop() calls.
  PARAOS_INLINE_TRIVIAL auto LastDuration() -> cnt_t override {
    return duration_;
  }

 private:
  cnt_t start_{0U};
  cnt_t duration_{0U};
  cnt_t overflow_cnt_{0U};

  /// @brief Interface for get actual counter value on each time.
  const IEmbeddedTimer *timer_;
};

}  // namespace paraos

#endif /* PARAOS_RUNTIME_PROFILER_HPP */
