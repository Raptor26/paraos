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
/// @brief FreeRTOS implementation of paraos::jthread.

#ifndef PARAOS_JTHREAD_HPP
#define PARAOS_JTHREAD_HPP

#include <cstddef>
#include <functional>
#include <tuple>
#include <type_traits>
#include <utility>

#include "FreeRTOS.h"
#include "etl/atomic.h"
#include "paraos_attr.h"
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
  /// @brief Construct a thread and start execution.
  ///
  /// The callable receives the provided arguments followed by a
  /// paraos::stop_token as its last argument.
  ///
  /// @tparam Function Callable type.
  /// @tparam Args Argument types.
  /// @param[in] f Callable to run in the new thread.
  /// @param[in] args Arguments to forward to the callable.
  template <typename Function, typename... Args>
  explicit jthread(Function&& f, Args&&... args) {
    using decayed_function = std::decay_t<Function>;
    using decayed_args = std::tuple<std::decay_t<Args>...>;

    auto* invoker = new Invoker<decayed_function, decayed_args>(
        std::forward<Function>(f), std::forward<Args>(args)...);

    context_ = new Context{};
    context_->invoker = invoker;

    xTaskCreate(
        RunTask, "jthread",
        paraos::ConvertStackSizeInWords(paraos::GetStackMinimumSizeInBytes()),
        context_, static_cast<UBaseType_t>(paraos::ThreadPriority::kNormal),
        &context_->handle);

    if (context_->handle == nullptr) {
      delete invoker;
      delete context_;
      context_ = nullptr;
      ETL_ASSERT(false, ETL_ERROR(paraos::thread_not_created_exception));
    }
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

 private:
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
        : func_(std::forward<F>(f)),
          args_(std::forward<ArgsIn>(args)...) {}

    void Invoke(stop_token token) override {
      std::apply(
          [&](auto&&... captured_args) {
            std::invoke(std::move(func_),
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
