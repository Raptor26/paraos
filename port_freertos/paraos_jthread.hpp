/// @file paraos_jthread.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
#ifndef PARAOS_JTHREAD_HPP
#define PARAOS_JTHREAD_HPP

#include <cstddef>
#include <cstdlib>
#include <functional>
#include <tuple>
#include <type_traits>
#include <utility>

#include "FreeRTOS.h"
#include "etl/atomic.h"
#include "paraos_attr.h"
#include "paraos_check.h"
#include "paraos_exceptions.hpp"
#include "paraos_semaphore.hpp"
#include "paraos_thread_common.hpp"
#include "paraos_thread_exceptions.hpp"
#include "paraos_utils.hpp"
#include "task.h"

namespace paraos {

/// @brief Stop token for FreeRTOS jthread.
class stop_token {
 public:
  /// @brief Default-construct an empty stop token.
  stop_token() = default;

  /// @brief Construct from an atomic stop flag pointer.
  explicit stop_token(etl::atomic_bool* flag) noexcept : flag_(flag) {}

  /// @brief Check whether a stop request has been made.
  [[nodiscard]] auto stop_requested() const noexcept -> bool {
    return flag_ != nullptr && flag_->load();
  }

  /// @brief Check whether this token can receive a stop request.
  [[nodiscard]] auto stop_possible() const noexcept -> bool {
    return flag_ != nullptr;
  }

 private:
  etl::atomic_bool* flag_{nullptr};
};

/// @brief Stop source for FreeRTOS jthread.
class stop_source {
 public:
  /// @brief Default-construct a stop source.
  stop_source() = default;

  /// @brief Construct from an atomic stop flag pointer.
  explicit stop_source(etl::atomic_bool* flag) noexcept : flag_(flag) {}

  /// @brief Request a stop.
  [[nodiscard]] auto request_stop() noexcept -> bool {
    if (flag_ != nullptr) {
      flag_->store(true);
      return true;
    }
    return false;
  }

  /// @brief Get a stop token associated with this source.
  [[nodiscard]] auto get_token() const noexcept -> stop_token {
    return stop_token{flag_};
  }

  /// @brief Check whether a stop request has been made.
  [[nodiscard]] auto stop_requested() const noexcept -> bool {
    return flag_ != nullptr && flag_->load();
  }

  /// @brief Check whether this source can issue a stop request.
  [[nodiscard]] auto stop_possible() const noexcept -> bool {
    return flag_ != nullptr;
  }

 private:
  etl::atomic_bool* flag_{nullptr};
};

/// @brief jthread implementation for FreeRTOS.
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
  template <typename Function, typename... Args>
    requires(!std::is_same_v<std::decay_t<Function>, ThreadAttr>)
  explicit jthread(Function&& func, Args&&... args) {
    MakeThread(
        ThreadAttr{}, std::forward<Function>(func),
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
  jthread(jthread&& other) noexcept : context_(other.context_) {
    other.context_ = nullptr;
  }

  /// @brief Move assignment transfers context ownership.
  auto operator=(jthread&& other) noexcept -> jthread& {
    if (this != &other) {
      if (joinable()) {
        request_stop();
        join();
      }
      delete context_;
      context_ = other.context_;
      other.context_ = nullptr;
    }
    return *this;
  }

  /// @brief Destructor requests stop and joins if the thread is joinable.
  ~jthread() {
    if (joinable()) {
      request_stop();
      join();
    }
    delete context_;
  }

  /// @brief Request the running thread to stop.
  [[nodiscard]] auto request_stop() noexcept -> bool {
    if (context_ != nullptr) {
      context_->stop_flag.store(true);
      return true;
    }
    return false;
  }

  /// @brief Block until the thread finishes execution.
  void join() {
    if (context_ != nullptr) {
      context_->join_sem.Take();
    }
  }

  /// @brief Check whether the thread is joinable.
  [[nodiscard]] auto joinable() const noexcept -> bool {
    return context_ != nullptr && context_->handle != nullptr;
  }

  /// @brief Start the FreeRTOS scheduler.
  ///
  /// Delegates to `vTaskStartScheduler()`. The calling task does not return
  /// while the scheduler is running.
  ///
  /// @note Calling this method more than once is a programming error; the
  ///   repeated-call guard is asserted in debug builds.
  /// @warning Runtime validation of FreeRTOS tasks is not available on the
  ///   macOS POSIX simulator; on that host this call is validated by
  ///   compilation only.
  static void start_scheduler() {
    PARAOS_CHECK_ASSERT(xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED);
    vTaskStartScheduler();
  }

  /// @brief Check whether the FreeRTOS scheduler is running.
  ///
  /// @return `true` if the scheduler has been started, `false` otherwise.
  [[nodiscard]] static auto is_scheduler_running() noexcept -> bool {
    return xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED;
  }

  /// @brief Stop the FreeRTOS scheduler.
  ///
  /// If the scheduler is running, calls `vTaskEndScheduler()` and returns
  /// `true`. If the scheduler is not running, returns `false` without calling
  /// `vTaskEndScheduler()`.
  ///
  /// @note The behavior of `vTaskEndScheduler()` depends on the target
  ///   FreeRTOS port's implementation of `vPortEndScheduler()`; callers must
  ///   ensure the target port supports scheduler shutdown.
  /// @warning Runtime validation of FreeRTOS tasks is not available on the
  ///   macOS POSIX simulator; on that host this call is validated by
  ///   compilation only.
  [[nodiscard]] static auto end_scheduler() noexcept -> bool {
    if (xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED) {
      return false;
    }
#define PARAOS_FORCE_EXIT_FROM_SCHEDULER
#ifdef PARAOS_FORCE_EXIT_FROM_SCHEDULER
    // В UNIX/Windows vTaskEndScheduler() не завершает корректно потоки, поэтому
    // нужно принудительно выйти из программы.
    std::exit(0);
#else
    vTaskEndScheduler();
#endif
#undef PARAOS_FORCE_EXIT_FROM_SCHEDULER
    return true;
  }

 private:
  /// @brief Common implementation for both constructors.
  template <typename Function, typename... Args>
  void MakeThread(const ThreadAttr& attr, Function&& func, Args&&... args) {
    using decayed_function = std::decay_t<Function>;
    using decayed_args = std::tuple<std::decay_t<Args>...>;

    auto* invoker = new Invoker<decayed_function, decayed_args>(
        std::forward<Function>(func), std::forward<Args>(args)...);

    context_ = new Context{false, invoker, {}, nullptr, attr};

    xTaskCreate(
        RunTask, attr.thread_name.data(),
        paraos::ConvertStackSizeInWords(attr.stack_depth), context_,
        static_cast<UBaseType_t>(attr.priority), &context_->handle);

    if (context_->handle == nullptr) {
      delete invoker;
      delete context_;
      context_ = nullptr;
      ETL_ASSERT(false, ETL_ERROR(paraos::thread_not_created_exception));
    }
  }

  /// @brief Abstract invoker base.
  class InvokerBase {
   public:
    virtual ~InvokerBase() = default;

    /// @brief Invoke the stored callable with the given stop token.
    virtual void Invoke(stop_token token) = 0;
  };

  /// @brief Templated invoker that owns the callable and arguments.
  template <typename Function, typename ArgsTuple>
  class Invoker : public InvokerBase {
   public:
    template <typename F, typename... ArgsIn>
    Invoker(F&& f, ArgsIn&&... args)
        : func_(std::forward<F>(f)), args_(std::forward<ArgsIn>(args)...) {}

    void Invoke(stop_token token) override {
      std::apply(
          [&](auto&&... captured_args) {
            std::invoke(
                std::move(func_),
                std::forward<decltype(captured_args)>(captured_args)...,
                std::move(token));
          },
          std::move(args_));
    }

   private:
    Function func_;
    ArgsTuple args_;
  };

  /// @brief Context shared between the jthread object and the FreeRTOS task.
  struct Context {
    etl::atomic_bool stop_flag{false};
    InvokerBase* invoker{nullptr};
    SemaphoreBinary join_sem;
    TaskHandle_t handle{nullptr};
    ThreadAttr attr{};
  };

  static void RunTask(void* param) {
    auto* ctx = static_cast<Context*>(param);
    if (ctx != nullptr && ctx->invoker != nullptr) {
      ctx->invoker->Invoke(stop_token{&ctx->stop_flag});
    }
    if (ctx != nullptr) {
      ctx->join_sem.Give();
    }
    vTaskDelete(nullptr);
  }

  Context* context_{nullptr};
};

}  // namespace paraos

#endif /* PARAOS_JTHREAD_HPP */
