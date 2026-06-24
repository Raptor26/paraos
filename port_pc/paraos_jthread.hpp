/// @file paraos_jthread.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
#ifndef PARAOS_JTHREAD_HPP
#define PARAOS_JTHREAD_HPP

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <stop_token>
#include <thread>
#include <type_traits>
#include <unordered_set>
#include <utility>

#ifdef PARAOS_LIKE_UNIX
#include <pthread.h>
#include <unistd.h>
#elif defined(PARAOS_LIKE_WINAPI)
// NOLINTBEGIN(llvm-include-order)
// clang-format off
// winsock2.h must be included before windows.h.
#include <winsock2.h>
#include <windows.h>
#ifdef __MINGW32__
#include <pthread.h>
#endif
// clang-format on
// NOLINTEND(llvm-include-order)
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
  std::stop_token token_;
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
///
/// PC threads emulate FreeRTOS scheduler semantics: threads created before
/// start_scheduler() block until the scheduler is started; end_scheduler()
/// stops and joins all active threads.
class jthread {
 public:
  /// @brief Default-construct an empty non-joinable thread.
  jthread() noexcept = default;

  /// @brief Construct a thread with default attributes.
  ///
  /// The callable receives the provided arguments followed by a
  /// paraos::stop_token as its last argument.
  ///
  /// @tparam Function Callable type.
  /// @tparam Args Argument types.
  /// @param[in] func Callable to run in the new thread.
  /// @param[in] args Arguments to forward to the callable.
  template <typename Function, typename... Args>
    requires(!std::is_same_v<std::decay_t<Function>, ThreadAttr>)
  explicit jthread(Function&& func, Args&&... args) {
    MakeThread(
        ThreadAttr{}, std::forward<Function>(func),
        std::forward<Args>(args)...);
  }

  /// @brief Construct a thread with the specified attributes.
  ///
  /// The callable receives the provided arguments followed by a
  /// paraos::stop_token as its last argument.
  ///
  /// @tparam Function Callable type.
  /// @tparam Args Argument types.
  /// @param[in] attr Thread attributes (name, stack depth, priority).
  /// @param[in] func Callable to run in the new thread.
  /// @param[in] args Arguments to forward to the callable.
  template <typename Function, typename... Args>
  explicit jthread(const ThreadAttr& attr, Function&& func, Args&&... args) {
    MakeThread(attr, std::forward<Function>(func), std::forward<Args>(args)...);
  }

  /// @brief Copy operations are disabled.
  jthread(const jthread& other) = delete;
  auto operator=(const jthread& other) -> jthread& = delete;

  /// @brief Move construction transfers context ownership.
  jthread(jthread&& other) noexcept = default;

  /// @brief Move assignment transfers context ownership.
  auto operator=(jthread&& other) noexcept -> jthread& {
    if (this != &other) {
      if (joinable()) {
        (void)request_stop();
        join();
      }
      if (context_ != nullptr) {
        const std::scoped_lock lock{s_registry_mutex};
        s_registry.erase(context_.get());
      }
      context_ = std::move(other.context_);
    }
    return *this;
  }

  /// @brief Destructor requests stop and joins if the thread is joinable.
  ~jthread() {
    if (context_ == nullptr) {
      return;
    }
    if (joinable() && std::this_thread::get_id() != context_->owner_id) {
      (void)request_stop();
      {
        const std::scoped_lock lock{context_->gate_mtx};
        context_->should_run = false;
        context_->gate_open = true;
      }
      context_->gate_cv.notify_one();
      join();
    }
    const std::scoped_lock lock{s_registry_mutex};
    s_registry.erase(context_.get());
  }

  /// @brief Request the running thread to stop.
  [[nodiscard]] auto request_stop() noexcept -> bool {
    if (context_ == nullptr) {
      return false;
    }
    return context_->thread.request_stop();
  }

  /// @brief Block until the thread finishes execution.
  void join() {
    if (context_ == nullptr) {
      return;
    }
    if (std::this_thread::get_id() == context_->owner_id) {
      return;
    }
    if (context_->thread.joinable()) {
      context_->thread.join();
    }
  }

  /// @brief Check whether the thread is joinable.
  [[nodiscard]] auto joinable() const noexcept -> bool {
    return context_ != nullptr && context_->thread.joinable();
  }

  /// @brief Start the scheduler and release all waiting threads.
  ///
  /// Threads created before this call begin executing user code. Subsequent
  /// calls are idempotent and have no effect once the scheduler has been
  /// started or stopped.
  static void start_scheduler() {
    if (s_scheduler_state.exchange(SchedulerState::kRunning) !=
        SchedulerState::kNotStarted) {
      return;
    }
    std::vector<Context*> contexts;
    {
      const std::scoped_lock lock{s_registry_mutex};
      contexts.assign(s_registry.begin(), s_registry.end());
    }
    for (auto* ctx : contexts) {
      OpenGate(ctx, true);
    }
  }

  /// @brief Check whether the scheduler is running.
  [[nodiscard]] static auto is_scheduler_running() noexcept -> bool {
    return s_scheduler_state.load() == SchedulerState::kRunning;
  }

  /// @brief Stop the scheduler and join all active threads.
  ///
  /// Releases any threads still waiting for the scheduler with
  /// should_run=false, requests stop on every active thread, then joins them.
  /// The calling thread is skipped during join so that a jthread worker may
  /// call end_scheduler() to shut down the scheduler.
  ///
  /// @return `true` if the scheduler was running and has been stopped,
  ///   `false` otherwise.
  [[nodiscard]] static auto end_scheduler() -> bool {
    if (s_scheduler_state.exchange(SchedulerState::kStopped) !=
        SchedulerState::kRunning) {
      return false;
    }
    const auto self_id = std::this_thread::get_id();
    std::vector<Context*> contexts;
    {
      const std::scoped_lock lock{s_registry_mutex};
      contexts.assign(s_registry.begin(), s_registry.end());
    }
    for (auto* ctx : contexts) {
      OpenGate(ctx, false);
    }
    for (auto* ctx : contexts) {
      if (ctx->thread.get_id() == self_id) {
        continue;
      }
      ctx->thread.request_stop();
    }
    for (auto* ctx : contexts) {
      if (ctx->thread.get_id() == self_id) {
        continue;
      }
      if (ctx->thread.joinable()) {
        ctx->thread.join();
      }
    }
    return true;
  }

 private:
  enum class SchedulerState : std::uint8_t { kNotStarted, kRunning, kStopped };

  struct Context {
    std::jthread thread;
    ThreadAttr attr{};
    std::mutex gate_mtx;
    std::condition_variable gate_cv;
    bool gate_open{false};
    bool should_run{false};
    std::thread::id owner_id{};
  };

  static void OpenGate(Context* ctx, bool should_run) {
    const std::scoped_lock lock{ctx->gate_mtx};
    ctx->should_run = should_run;
    ctx->gate_open = true;
    ctx->gate_cv.notify_one();
  }

  static void WaitForGate(Context* ctx) {
    std::unique_lock lock{ctx->gate_mtx};
    ctx->gate_cv.wait(lock, [ctx]() -> bool { return ctx->gate_open; });
  }

  /// @brief Common implementation for both constructors.
  template <typename Function, typename... Args>
  void MakeThread(const ThreadAttr& attr, Function&& func, Args&&... args) {
    context_ = std::make_unique<Context>();
    context_->attr = attr;

    {
      const std::scoped_lock lock{s_registry_mutex};
      s_registry.insert(context_.get());
    }

    const auto state = s_scheduler_state.load();
    if (state != SchedulerState::kNotStarted) {
      OpenGate(context_.get(), state == SchedulerState::kRunning);
    }

    auto* ctx = context_.get();
    context_->thread = std::jthread(
        [func = std::forward<Function>(func),
         ... captured_args = std::forward<Args>(args),
         ctx](std::stop_token std_token) mutable -> void {
          WaitForGate(ctx);
          if (!ctx->should_run) {
            return;
          }
          std::invoke(
              std::move(func), std::move(captured_args)...,
              stop_token{std::move(std_token)});
        });
    context_->owner_id = context_->thread.get_id();
    ApplyAttr();
  }

  /// @brief Apply platform-specific thread attributes after creation.
  void ApplyAttr() {
#ifdef PARAOS_LIKE_UNIX
    if (context_->thread.joinable()) {
      sched_param param{};
#ifdef __APPLE__
      param.sched_priority = MapPriorityToSchedRange(context_->attr.priority);
#else
      param.sched_priority = static_cast<int>(context_->attr.priority);
#endif
      // Ignore return value: changing priority requires privileges; failing
      // here must not break user code.
      (void)pthread_setschedparam(
          context_->thread.native_handle(), SCHED_RR, &param);
    }
#elif defined(PARAOS_LIKE_WINAPI)
    if (context_->thread.joinable()) {
#ifdef __MINGW32__
      // MinGW's libstdc++ uses pthread_t as native_handle_type.
      // Convert it to the underlying Win32 handle before calling
      // SetThreadPriority().
      HANDLE handle = pthread_gethandle(
          static_cast<pthread_t>(context_->thread.native_handle()));
      if (handle != nullptr) {
        (void)SetThreadPriority(
            handle, static_cast<int>(context_->attr.priority));
      }
#else
      (void)SetThreadPriority(
          context_->thread.native_handle(),
          static_cast<int>(context_->attr.priority));
#endif
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

    constexpr auto k_min_enum = static_cast<int>(paraos::ThreadPriority::kIdle);
    constexpr auto k_max_enum =
        static_cast<int>(paraos::ThreadPriority::kRealTime);
    const auto prior = static_cast<int>(priority);

    if (max <= min) {
      return min;
    }

    const auto mapped =
        (min +
         (((prior - k_min_enum) * (max - min)) / (k_max_enum - k_min_enum)));
    return mapped;
  }
#endif

  inline static std::atomic<SchedulerState> s_scheduler_state{
      SchedulerState::kNotStarted};
  inline static std::mutex s_registry_mutex;
  inline static std::unordered_set<Context*> s_registry;

  std::unique_ptr<Context> context_;
};

}  // namespace paraos

#endif /* PARAOS_JTHREAD_HPP */
