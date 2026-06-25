/// @file paraos_runtime_profiler.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
#ifndef PARAOS_RUNTIME_PROFILER_HPP
#define PARAOS_RUNTIME_PROFILER_HPP

#if defined(_WIN32) || defined(_WIN64) || defined(__linux__) || \
    defined(__unix__) || defined(__APPLE__)
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

using count_type = std::uint32_t;
using cnt_t PARAOS_DEPRECATED("use count_type") = count_type;

/// @brief Количество микросекунд в одной миллисекунде.
constexpr count_type us_in_ms{1000};

struct profiler_base {
  /// @brief Start timer.
  virtual void start() = 0;

  /// @brief Stop timer and calculate time between start() and stop() calls.
  ///
  /// @return Time between start() and stop() calls.
  virtual auto stop() -> count_type = 0;

  /// @brief Returned value, which calculated wen user calls stop().
  ///
  /// @return Time between last start() and stop() calls.
  virtual auto last_duration() -> count_type = 0;

  profiler_base() = default;
  profiler_base(const profiler_base&) = delete;
  auto operator=(const profiler_base&) -> profiler_base& = delete;
  profiler_base(profiler_base&&) = default;
  auto operator=(profiler_base&&) -> profiler_base& = default;

  virtual ~profiler_base() = default;
};

using IProfiler PARAOS_DEPRECATED("use paraos::profiler_base") = profiler_base;

/// @brief RAII class for automaticaly start and stop profiler.
///
/// @note In some cases, we need profiler some section. In this case user
/// manually run start() and stop() methods. Disadvantage of this approach:
/// - if throw exception between start() and stop(), stop() never will calling.
/// - if need get full runtime of some method, we can't get runtime with return
/// operator.
///
/// Using RAII class you will overcome these disadvantages.
///
/// @example For usage example see TEST_F(Profiler, RAII) in
/// test_runtime_profiler.cpp
class profiler_raii final {
 public:
  explicit profiler_raii(profiler_base &profiler) : profiler_{profiler} {
    profiler_.start();
  }

  ~profiler_raii() { profiler_.stop(); }

  profiler_raii(profiler_raii &&other) = delete;
  auto operator=(profiler_raii &&other) -> profiler_raii & = delete;
  auto operator=(const profiler_raii &other) -> profiler_raii & = delete;
  profiler_raii(const profiler_raii &other) = delete;

 private:
  profiler_base &profiler_;
};

using ProfilerRAII PARAOS_DEPRECATED("use paraos::profiler_raii") =
    profiler_raii;

/// @brief RAII class for calculate period between profiler_period_raii calls.
///
/// @note If you need calculate period calling some bloc code, use
/// profiler_period_raii class.
class profiler_period_raii final {
 public:
  explicit profiler_period_raii(profiler_base &profiler) {
    profiler.stop();
    profiler.start();
  }

  ~profiler_period_raii() = default;

  profiler_period_raii(profiler_period_raii &&other) = delete;
  auto operator=(profiler_period_raii &&other)
      -> profiler_period_raii & = delete;
  auto operator=(const profiler_period_raii &other)
      -> profiler_period_raii & = delete;
  profiler_period_raii(const profiler_period_raii &other) = delete;
};

using ProfilerPeriodRAII PARAOS_DEPRECATED("use paraos::profiler_period_raii") =
    profiler_period_raii;

/// @brief Embedded timer interface. Need for get actual timer value and use it
/// for calculate runtime in embedded_profiler() and timer_profiler().
///
/// @note In more cases, user code create one instance of embedded_timer_base
/// inteface and use it with many instances of embedded_profiler() and
/// timer_profiler().
struct embedded_timer_base {
  virtual ~embedded_timer_base() = default;
  [[nodiscard]] virtual auto give_count() const -> count_type = 0;
  [[nodiscard]] virtual auto give_count_overflow_value() const -> count_type = 0;

  /// @brief Five rule.
  embedded_timer_base(embedded_timer_base &&other) = delete;
  auto operator=(embedded_timer_base &&other)
      -> embedded_timer_base & = delete;
  auto operator=(const embedded_timer_base &other)
      -> embedded_timer_base & = delete;
  embedded_timer_base(const embedded_timer_base &other) = delete;

 protected:
  embedded_timer_base() = default;
};

using IEmbeddedTimer PARAOS_DEPRECATED("use paraos::embedded_timer_base") =
    embedded_timer_base;

/// @brief Override embedded_timer_base interface with zero values.
struct embedded_timer_empty final : public embedded_timer_base {
  [[nodiscard]] auto give_count() const -> count_type override { return 0U; };
  [[nodiscard]] auto give_count_overflow_value() const -> count_type override {
    return 0U;
  };
};

/// @brief Empty embedded timer instance. Use if need set reference on
/// embedded_timer_base without real timer.
inline embedded_timer_empty embedded_timer_empty_instance;

using EmbeddedTimerEmpty PARAOS_DEPRECATED(
    "use struct paraos::embedded_timer_empty") = struct embedded_timer_empty;

/// Empty profiler -------------------------------------------------------------

/// @brief "Пустой" профилировщик. Используется в качестве профилировщика "по
/// умолчанию".
struct empty_profiler final : public profiler_base {
  PARAOS_INLINE_TRIVIAL void start() override {}
  PARAOS_INLINE_TRIVIAL auto stop() -> count_type override { return 0U; }
  PARAOS_INLINE_TRIVIAL auto last_duration() -> count_type override { return 0U; }
};

/// @brief Empty profiler instance. Use if need set reference on profiler_base
/// without real profiler.
inline empty_profiler empty_profiler_instance;

using EmptyProfiler PARAOS_DEPRECATED("use struct paraos::empty_profiler") =
    struct empty_profiler;

/// Operation system high resolution timer profiler ----------------------------

#if defined(_WIN32) || defined(_WIN64) || defined(__linux__) || \
    defined(__unix__) || defined(__APPLE__)
/// @brief Профилировщик, предназначенный для использования в операционных
/// системах общего назначения.
struct os_profiler final : public profiler_base {
  PARAOS_INLINE_OPERATIONS void start() override {
    start_ = high_resolution_clock::now();
  }

  auto stop() -> count_type override {
    end_ = high_resolution_clock::now();
    duration_ = duration_cast<time_resolution>(end_ - start_).count();
    return last_duration();
  }

  auto last_duration() -> count_type override { return duration_; }

  auto last_duration_ms() -> count_type {
    return static_cast<count_type>(last_duration() / us_in_ms);
  }

  PARAOS_DEPRECATED("use last_duration_ms()") auto LastDurationMs()
      -> count_type {
    return last_duration_ms();
  }

 private:
  decltype(high_resolution_clock::now()) start_;
  decltype(high_resolution_clock::now()) end_;
  decltype(duration_cast<time_resolution>(end_ - start_).count()) duration_{0U};
};

using OsProfiler PARAOS_DEPRECATED("use paraos::os_profiler") = os_profiler;

/// @brief Profiler timer if run on operation system (like as windows or linux).
struct os_timer final : public embedded_timer_base {
  os_timer() {
    // Write current time in private field. Useful when user calls give_count().
    start_ = high_resolution_clock::now();
  }

  ~os_timer() override = default;

  [[nodiscard]] auto give_count() const -> count_type override {
    auto end = high_resolution_clock::now();

    // Calculate durations between os_timer constructor and now time when
    // user code calls give_count(). We don't use
    // 'high_resolution_clock::now().time_since_epoch().count()' because in this
    // case return value be in ticks, but we want microseconds.
    auto duration = duration_cast<time_resolution>(end - start_).count();

    // In this case we tracking counter overflow, because duration has large bit
    // depth relative count_type type (imitate count_type type bits counter with
    // hardware overflow).
    return static_cast<count_type>(duration % std::numeric_limits<count_type>::max());
  };

  [[nodiscard]] auto give_count_overflow_value() const -> count_type override {
    return std::numeric_limits<count_type>::max();
  };

  /// @brief Five rule.
  os_timer(os_timer &&other) = delete;
  auto operator=(os_timer &&other) -> os_timer & = delete;
  auto operator=(const os_timer &other) -> os_timer & = delete;
  os_timer(const os_timer &other) = delete;

 private:
  decltype(high_resolution_clock::now()) start_;
};

using OsTimer PARAOS_DEPRECATED("use paraos::os_timer") = os_timer;
#endif  // #if defined(_WIN32) || defined(_WIN64) || defined(__linux__) ||
        // defined(__unix__)

/// Embedded Profiler ----------------------------------------------------------

struct high_count_default {};

using HightCntDefault PARAOS_DEPRECATED("use paraos::high_count_default") =
    high_count_default;

/// @brief Шаблон 32-х битного счетчика.
///
/// @warning Пользовательский код не использует данную структуру, она
/// применяется в 'embedded_profiler'.
///
/// @tparam LOW_ADDR Структура, используемая для получения указателя на младшие
/// 16 бит счетчика.
/// @tparam HIGHT_ADDR Структура, используемая для получения указателя на
/// старшие 16 бит счетчика.
template <typename LOW_ADDR, typename HIGHT_ADDR>
struct embedded_timer : public embedded_timer_base {
  embedded_timer() = default;

  /// @brief Деструктор "по умолчанию".
  /// @note Деструктор явно не объявлен виртуальным т.к. "embedded_timer" и
  /// "embedded_profiler" не выделяют динамических ресурсов.
  ~embedded_timer() override = default;

  embedded_timer(const embedded_timer &other) = default;
  auto operator=(const embedded_timer &other) -> embedded_timer & = default;

  embedded_timer(embedded_timer &&other) noexcept = default;
  auto operator=(embedded_timer &&other) noexcept -> embedded_timer & = default;

  /// @brief Возвращает значение 32-х битного аппаратного счетчика на момент
  /// вызова.
  /// @return Возвращает переменную типа 'count_type' содержащую значение аппаратного
  /// счетчика на момент вызова.
  [[nodiscard]] PARAOS_INLINE_TRIVIAL auto give_count() const -> count_type override {
    constexpr count_type hight_mask = 0xFFFF0000;
    constexpr count_type low_mask = 0x0000FFFF;
    constexpr count_type bites_shift = 16U;
    return (
        (((static_cast<count_type>(*hight)) << bites_shift) & hight_mask) |
        ((static_cast<count_type>(*low)) & low_mask));
  }

  [[nodiscard]] PARAOS_INLINE_TRIVIAL auto give_count_overflow_value() const
      -> count_type override {
    return std::numeric_limits<std::uint32_t>::max();
  }

 private:
  volatile std::uint16_t *hight = HIGHT_ADDR{}();
  volatile std::uint16_t *low = LOW_ADDR{}();
};

/// @brief Специализация шаблона счетчика в случае использования 16-ти битного
/// счетчика (вместо 32-х битного).
/// @warning Пользовательский код не использует данную структуру, она
/// применяется в 'embedded_profiler'.
/// @tparam LOW_ADDR Структура, используемая для получения указателя на младшие
/// 16 бит счетчика.
template <typename LOW_ADDR>
struct embedded_timer<LOW_ADDR, high_count_default> : public embedded_timer_base {
  embedded_timer() = default;

  /// @brief Деструктор "по умолчанию".
  /// @note Деструктор явно не объявлен виртуальным т.к. "embedded_timer" и
  /// "embedded_profiler" не выделяют динамических ресурсов.
  ~embedded_timer() override = default;

  embedded_timer(const embedded_timer &other) = default;
  auto operator=(const embedded_timer &other) -> embedded_timer & = default;

  embedded_timer(embedded_timer &&other) noexcept = default;
  auto operator=(embedded_timer &&other) noexcept -> embedded_timer & = default;

  /// @brief Возвращает значение 16-х битного аппаратного счетчика на момент
  /// вызова.
  /// @return Возвращает переменную типа 'count_type' содержащую значение аппаратного
  /// счетчика на момент вызова.
  [[nodiscard]] PARAOS_INLINE_TRIVIAL auto give_count() const -> count_type override {
    return static_cast<count_type>(*low);
  }

  [[nodiscard]] PARAOS_INLINE_TRIVIAL auto give_count_overflow_value() const
      -> count_type override {
    return std::numeric_limits<std::uint16_t>::max();
  }

 private:
  volatile std::uint16_t *low = LOW_ADDR{}();
};

template <typename LOW_ADDR, typename HIGHT_ADDR = high_count_default>
using EmbeddedTimer PARAOS_DEPRECATED("use paraos::embedded_timer") =
    embedded_timer<LOW_ADDR, HIGHT_ADDR>;

/// @brief Профилировщик, предназначенный для использования во встраиваемых
/// системах.
/// @tparam LOW_ADDR Структура, используемая для получения указателя на младшие
/// 16 бит счетчика.
/// @tparam HIGHT_ADDR Структура, используемая для получения указателя на
/// старшие 16 бит счетчика.
template <typename LOW_ADDR, typename HIGHT_ADDR = high_count_default>
struct embedded_profiler final : public profiler_base,
                                public embedded_timer<LOW_ADDR, HIGHT_ADDR> {
  PARAOS_INLINE_OPERATIONS void start() override {
    start_ = embedded_timer<LOW_ADDR, HIGHT_ADDR>::give_count();

    // Необходимо сбросить счетчик переполнений чтобы при повторном вызове
    // stop() не учитывать уже учтенное переполнение.
    overflow_cnt_ = 0U;
  }

  auto stop() -> count_type override {
    const count_type stop = embedded_timer<LOW_ADDR, HIGHT_ADDR>::give_count();

    if (start_ > stop) {
      ++overflow_cnt_;
    }

    // Вычисление периода между вызовами start() и stop() с учетом переполнения.
    const auto &overflow_value =
        embedded_timer<LOW_ADDR, HIGHT_ADDR>::give_count_overflow_value();
    duration_ =
        (overflow_value * overflow_cnt_) + (stop - start_) + overflow_cnt_;

    return last_duration();
  }

  PARAOS_INLINE_TRIVIAL auto last_duration() -> count_type override {
    return duration_;
  }

 private:
  count_type start_{0U};
  count_type duration_{0U};
  count_type overflow_cnt_{0U};
};

template <typename LOW_ADDR, typename HIGHT_ADDR = high_count_default>
using EmbeddedProfiler PARAOS_DEPRECATED("use paraos::embedded_profiler") =
    embedded_profiler<LOW_ADDR, HIGHT_ADDR>;

/// @brief Runtime profiler based on user definition counter. User code use
/// Ctor() or set_embedded_timer() for connect one embedded timer to the
/// timer_profiler instance.
///
/// @note timer_profiler don't use any template argument.
struct timer_profiler final : public profiler_base {
  /// @brief Ctor with embedded timer reference.
  /// @note Many timer_profiler instances can use one embedded_timer_base instance.
  ///
  /// @param[in] timer: New timer for connect to the profiler. If use default
  /// value, stop() and last_duration() return zero value. For this reason user
  /// code must call set_embedded_timer() and set valid embedded timer reference.
  explicit timer_profiler(
      const embedded_timer_base &timer = embedded_timer_empty_instance)
      : timer_{&timer} {}

  /// @brief Connect new embedded timer to the profiler.
  ///
  /// @note User code must call this method if Ctor use default params. In
  /// otherwise stop() and last_duration() return zero value.
  ///
  /// @param[in] timer: New timer for connect to the profiler.
  void set_embedded_timer(const embedded_timer_base &timer) { timer_ = &timer; }

  PARAOS_DEPRECATED("use set_embedded_timer()") void SetEmbeddedTimer(
      const embedded_timer_base &timer) {
    set_embedded_timer(timer);
  }

  /// @brief Start
  ///
  /// @return None
  PARAOS_INLINE_OPERATIONS void start() override {
    start_ = timer_->give_count();

    // Необходимо сбросить счетчик переполнений чтобы при повторном вызове
    // stop() не учитывать уже учтенное переполнение.
    overflow_cnt_ = 0U;
  }

  /// @brief Stop timer.
  ///
  /// @return Value between start() and stop() calls.
  auto stop() -> count_type override {
    const count_type stop = timer_->give_count();

    if (start_ > stop) {
      ++overflow_cnt_;
    }

    // Вычисление периода между вызовами start() и stop() с учетом переполнения.
    const auto &overflow_value = timer_->give_count_overflow_value();
    duration_ =
        (overflow_value * overflow_cnt_) + (stop - start_) + overflow_cnt_;

    return last_duration();
  }

  /// @brief Return value between start() and stop() calls.
  ///
  /// @return Value between start() and stop() calls.
  PARAOS_INLINE_TRIVIAL auto last_duration() -> count_type override {
    return duration_;
  }

 private:
  count_type start_{0U};
  count_type duration_{0U};
  count_type overflow_cnt_{0U};

  /// @brief Interface for get actual counter value on each time.
  const embedded_timer_base *timer_;
};

using TimerProfiler PARAOS_DEPRECATED("use paraos::timer_profiler") =
    timer_profiler;

}  // namespace paraos

#endif /* PARAOS_RUNTIME_PROFILER_HPP */
