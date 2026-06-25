/// @file paraos_scheduler.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
#ifndef PARAOS_SCHEDULER_HPP
#define PARAOS_SCHEDULER_HPP

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_set>
#include <utility>
#include <vector>

#include "paraos_thread_common.hpp"

namespace paraos {
namespace detail {

/// @brief Shared context for paraos::jthread and paraos::scheduler.
///
/// Lives in the detail namespace so that the scheduler can manage the
/// registry of thread contexts without a friendship cycle.
struct jthread_context {
  std::jthread thread;
  thread_attr attr{};
  std::mutex gate_mtx;
  std::condition_variable gate_cv;
  bool gate_open{false};
  bool should_run{false};
  std::thread::id owner_id{};
};

}  // namespace detail

/// @brief PC scheduler emulation of FreeRTOS scheduler semantics.
///
/// Owns the scheduler lifecycle (start/end) and the registry of threads
/// created before the scheduler starts. Threads register themselves on
/// construction and are released when start() is called.
class scheduler {
 public:
  /// @brief Access the process-wide scheduler instance.
  [[nodiscard]] static auto instance() noexcept -> scheduler& {
    static scheduler instance;
    return instance;
  }

  /// @brief Start the scheduler and release all waiting threads.
  void start() {
    if (state_.exchange(state::kRunning) != state::kNotStarted) {
      return;
    }
    std::vector<detail::jthread_context*> contexts;
    {
      const std::scoped_lock lock{registry_mtx_};
      contexts.assign(registry_.begin(), registry_.end());
    }
    for (auto* ctx : contexts) {
      open_gate(ctx, true);
    }
  }

  /// @brief Stop the scheduler and join all active threads except the caller.
  /// @return true if the scheduler was running and has been stopped.
  [[nodiscard]] auto end() -> bool {
    if (state_.exchange(state::kStopped) != state::kRunning) {
      return false;
    }
    const auto self_id = std::this_thread::get_id();
    std::vector<detail::jthread_context*> contexts;
    {
      const std::scoped_lock lock{registry_mtx_};
      contexts.assign(registry_.begin(), registry_.end());
    }
    for (auto* ctx : contexts) {
      open_gate(ctx, false);
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

  /// @brief Check whether the scheduler is running.
  [[nodiscard]] auto is_running() const noexcept -> bool {
    return state_.load() == state::kRunning;
  }

  /// @brief Register a thread context with the scheduler.
  ///
  /// Called from jthread constructor. If the scheduler has not started yet,
  /// the context remains gated until start() is called.
  void register_context(detail::jthread_context* ctx) {
    const std::scoped_lock lock{registry_mtx_};
    registry_.insert(ctx);
  }

  /// @brief Unregister a thread context from the scheduler.
  ///
  /// Called from jthread destructor and move assignment.
  void unregister_context(detail::jthread_context* ctx) {
    const std::scoped_lock lock{registry_mtx_};
    registry_.erase(ctx);
  }

  /// @brief Open the gate for a single context.
  static void open_gate(detail::jthread_context* ctx, bool should_run) {
    const std::scoped_lock lock{ctx->gate_mtx};
    ctx->should_run = should_run;
    ctx->gate_open = true;
    ctx->gate_cv.notify_one();
  }

 private:
  enum class state : std::uint8_t {
    kNotStarted,
    kRunning,
    kStopped
  };

  std::atomic<state> state_{state::kNotStarted};
  mutable std::mutex registry_mtx_;
  std::unordered_set<detail::jthread_context*> registry_;
};

}  // namespace paraos

#endif /* PARAOS_SCHEDULER_HPP */
