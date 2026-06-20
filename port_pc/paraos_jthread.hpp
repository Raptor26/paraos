/// @file paraos_jthread.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// @copyright (c) 2025 Stilsoft
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
///
/// @brief PC implementation of paraos::jthread as a thin wrapper over
///        std::jthread (C++20).

#ifndef PARAOS_JTHREAD_HPP
#define PARAOS_JTHREAD_HPP

#include <stop_token>
#include <thread>
#include <type_traits>
#include <utility>

#ifdef PARAOS_LIKE_UNIX
#include <pthread.h>
#include <unistd.h>
#elif defined(PARAOS_LIKE_WINAPI)
#include <windows.h>
#endif

#include "paraos_attr.h"
#include "paraos_thread_common.hpp"

namespace paraos {

/// @brief Wrapper around std::stop_token.
class stop_token {
 public:
  /// @brief Default-construct an empty stop token.
  stop_token() = default;

  /// @brief Construct from an existing std::stop_token.
  explicit stop_token(std::stop_token token) noexcept
      : token_(std::move(token)) {}

  /// @brief Check whether a stop request has been made.
  [[nodiscard]] auto stop_requested() const noexcept -> bool {
    return token_.stop_requested();
  }

  /// @brief Check whether this token can receive a stop request.
  [[nodiscard]] auto stop_possible() const noexcept -> bool {
    return token_.stop_possible();
  }

 private:
  std::stop_token token_{};
};

/// @brief Wrapper around std::stop_source.
class stop_source {
 public:
  /// @brief Default-construct a stop source.
  stop_source() = default;

  /// @brief Construct from an existing std::stop_source.
  explicit stop_source(std::stop_source source) noexcept
      : source_(std::move(source)) {}

  /// @brief Request a stop.
  [[nodiscard]] auto request_stop() noexcept -> bool {
    return source_.request_stop();
  }

  /// @brief Get a stop token associated with this source.
  [[nodiscard]] auto get_token() const noexcept -> stop_token {
    return stop_token{source_.get_token()};
  }

  /// @brief Check whether a stop request has been made.
  [[nodiscard]] auto stop_requested() const noexcept -> bool {
    return source_.stop_requested();
  }

  /// @brief Check whether this source can issue a stop request.
  [[nodiscard]] auto stop_possible() const noexcept -> bool {
    return source_.stop_possible();
  }

 private:
  std::stop_source source_{};
};

/// @brief std::jthread-style thread wrapper for PC platforms.
class jthread {
 public:
  /// @brief Construct a thread with default attributes and start execution.
  ///
  /// The callable receives the provided arguments followed by a
  /// paraos::stop_token as its last argument.
  ///
  /// @tparam Function Callable type.
  /// @tparam Args Argument types.
  /// @param[in] f Callable to run in the new thread.
  /// @param[in] args Arguments to forward to the callable.
  /// @brief Construct a thread with default attributes and start execution.
  ///
  /// The callable receives the provided arguments followed by a
  /// paraos::stop_token as its last argument.
  ///
  /// @tparam Function Callable type.
  /// @tparam Args Argument types.
  /// @param[in] f Callable to run in the new thread.
  /// @param[in] args Arguments to forward to the callable.
  template <typename Function, typename... Args,
            typename = std::enable_if_t<
                !std::is_same_v<std::decay_t<Function>, ThreadAttr>>>
  explicit jthread(Function&& f, Args&&... args) {
    MakeThread(ThreadAttr{}, std::forward<Function>(f),
               std::forward<Args>(args)...);
  }

  /// @brief Construct a thread with the specified attributes and start
  /// execution.
  ///
  /// The callable receives the provided arguments followed by a
  /// paraos::stop_token as its last argument.
  ///
  /// @tparam Function Callable type.
  /// @tparam Args Argument types.
  /// @param[in] attr Thread attributes (name, stack depth, priority).
  /// @param[in] f Callable to run in the new thread.
  /// @param[in] args Arguments to forward to the callable.
  template <typename Function, typename... Args>
  explicit jthread(const ThreadAttr& attr, Function&& f, Args&&... args) {
    MakeThread(attr, std::forward<Function>(f), std::forward<Args>(args)...);
  }

  /// @brief Copy operations are disabled.
  jthread(const jthread& other) = delete;
  auto operator=(const jthread& other) -> jthread& = delete;

  /// @brief Move construction is defaulted.
  jthread(jthread&& other) noexcept = default;

  /// @brief Move assignment is defaulted.
  auto operator=(jthread&& other) noexcept -> jthread& = default;

  /// @brief Destructor requests stop and joins if the thread is joinable.
  ~jthread() {
    if (thread_.joinable()) {
      thread_.request_stop();
      thread_.join();
    }
  }

  /// @brief Request the running thread to stop.
  [[nodiscard]] auto request_stop() noexcept -> bool {
    return thread_.request_stop();
  }

  /// @brief Block until the thread finishes execution.
  void join() { thread_.join(); }

  /// @brief Check whether the thread is joinable.
  [[nodiscard]] auto joinable() const noexcept -> bool {
    return thread_.joinable();
  }

 private:
  /// @brief Common implementation for both constructors.
  template <typename Function, typename... Args>
  void MakeThread(const ThreadAttr& attr, Function&& f, Args&&... args) {
    attr_ = attr;
    thread_ = std::jthread(
        [func = std::forward<Function>(f),
         ...captured_args = std::forward<Args>(args)](
            std::stop_token std_token) mutable -> void {
          std::invoke(std::move(func), std::move(captured_args)...,
                      stop_token{std::move(std_token)});
        });
    ApplyAttr();
  }

  /// @brief Apply platform-specific thread attributes after creation.
  void ApplyAttr() {
#ifdef PARAOS_LIKE_UNIX
    if (thread_.joinable()) {
      sched_param param{};
#ifdef __APPLE__
      param.sched_priority = MapPriorityToSchedRange(attr_.priority);
#else
      param.sched_priority = static_cast<int>(attr_.priority);
#endif
      // Ignore return value: changing priority requires privileges; failing
      // here must not break user code.
      (void)pthread_setschedparam(thread_.native_handle(), SCHED_RR, &param);
    }
#elif defined(PARAOS_LIKE_WINAPI)
    if (thread_.joinable()) {
      (void)SetThreadPriority(thread_.native_handle(),
                              static_cast<int>(attr_.priority));
    }
#endif
  }

#ifdef PARAOS_LIKE_UNIX
  /// @brief Map public ThreadPriority enum values to the valid macOS SCHED_RR
  /// range. macOS SCHED_RR priorities start at 15 (not 1), so the raw enum
  /// value would be rejected by pthread_setschedparam(). This mapping keeps the
  /// public enum unchanged while producing a valid priority for the underlying
  /// POSIX scheduler.
  [[nodiscard]] static auto MapPriorityToSchedRange(
      paraos::ThreadPriority priority) -> int {
    const auto min = sched_get_priority_min(SCHED_RR);
    const auto max = sched_get_priority_max(SCHED_RR);

    constexpr auto k_min_enum =
        static_cast<int>(paraos::ThreadPriority::kIdle);
    constexpr auto k_max_enum =
        static_cast<int>(paraos::ThreadPriority::kRealTime);
    const auto prior = static_cast<int>(priority);

    if (max <= min) {
      return min;
    }

    const auto mapped =
        (min + (((prior - k_min_enum) * (max - min)) /
                (k_max_enum - k_min_enum)));
    return mapped;
  }
#endif

  ThreadAttr attr_{};
  std::jthread thread_{};
};

}  // namespace paraos

#endif /* PARAOS_JTHREAD_HPP */
