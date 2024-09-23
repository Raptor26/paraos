/// @file paraos_work_queue.hpp
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

#ifndef PARAOS_WORK_QUEUE_HPP
#define PARAOS_WORK_QUEUE_HPP

#include <memory>

#include "gsl/gsl"
#include "paraos_queue_blocking.hpp"
#include "paraos_semaphore.hpp"
#include "paraos_thread.hpp"

namespace paraos {

class WorkItem {
 public:
  WorkItem() {}
  virtual ~WorkItem() = default;

  virtual void Run() = 0;
};

/// @brief We send this class if need send signal for destroy work queue thread.
class WorkItemForDestroyThread final : public WorkItem {
 public:
  WorkItemForDestroyThread() = default;
  ~WorkItemForDestroyThread() = default;
  void Run() override {};
};

/// @brief Contained params for execute user function.
template <const std::size_t SIZE = 10>
class WorkQueue final {
  using item_type = std::unique_ptr<WorkItem>;
  using queue_type = QueueBlocking<item_type, SIZE>;

 public:
  /// @brief Call this method only if WorkQueue() initialized with
  /// if_need_start_thread == false.
  void StartThread() {
    if (if_need_start_thread_) {
      worker_thread_.Start();

      const CriticalSection critical;
      is_thread_started_ = true;
    }
  }

  WorkQueue(
      const std::string name, std::size_t stack_depth, ThreadPriority priority,
      bool if_need_start_thread = true)
      : worker_thread_{name, stack_depth, priority, *this},
        if_need_start_thread_{if_need_start_thread} {
    StartThread();
  }

//   // Конструктор копирования
//   WorkQueue(const WorkQueue& other) {}

//   // Конструктор перемещения, noexcept - для оптимизации при использовании
//   // стандартных контейнеров
//   WorkQueue(WorkQueue&& other) noexcept {}

//   // Оператор присваивания копированием (copy assignment)
//   WorkQueue& operator=(const WorkQueue& other) {
//     if (this == &other) return *this;

//     return *this;
//   }

//   // Оператор присваивания перемещением (move assignment)
//   WorkQueue& operator=(WorkQueue&& other) noexcept {
//     if (this == &other) return *this;

//     return *this;
//   }

  auto Push(item_type&& work_item, std::size_t delay_ms = max_delay) {
    if (if_we_need_destroy_thread_) {
      return false;
    } else {
      return queue_.Push(std::move(work_item), delay_ms);
    }
  }

  ~WorkQueue() {
    decltype(is_thread_started_) is_thread_started;

    {
      const CriticalSection critical;
      is_thread_started = is_thread_started_;
    }

    if (is_thread_started) {
      {
        // Atomic send empty object and set if_we_need_destroy_thread_ flag
        const CriticalSection critical;
        Push(std::make_unique<WorkItemForDestroyThread>());
        if_we_need_destroy_thread_ = true;
      }

      // Wait signal only if scheduler is star, otherwise we stack forever
      // (in unit tests for example)
      if (Thread::IsSchedulerStarted()) {
        // We need destroy object when none of the user's request are
        // processing. For this reason we wait signal by Run().
        worker_thread_complete_.Take(max_delay);
      }
    }

    paraosTRACE_MESSAGE("~WorkQueue");
  }

 private:
  /// @brief Class for creating thread context, in which we call user
  /// functions.
  class WorkerThread final : public Thread {
   public:
    WorkerThread(
        const std::string name, std::size_t stack_depth,
        ThreadPriority priority, WorkQueue& parent, bool is_joinable = false)
        : Thread{name, stack_depth, priority, is_joinable}, parent_{parent} {}

    ~WorkerThread() {}

   private:
    void Run() override {
      constexpr bool is_need_while{true};
      while (is_need_while) {
        {
          auto item = parent_.queue_.Pop(max_delay);

          // if we use max_delay, then expect non empty item (witch wrapped by
          // std::optional<T>)
          Expects(item.has_value());
          if (item) {
            // item contained unique_ptr inside std::optional. ->get() take
            // unique_ptr, ->Run() call method from unique_ptr
            item->get()->Run();
          }
        }

        // Only this consumer can read items for queue instance. If we find
        // that necessary to destroy class object (with thread), we break
        // while cycle and send signal for Dtor thats mean none of the
        // requests are currently being processing and will not be processing
        // in future. const CriticalSection critical;
        if (parent_.if_we_need_destroy_thread_) {
          break;
        }
      }

      // We ready for destruct thread. Send signal for ~WorkerThread()
      parent_.worker_thread_complete_.Give();

      paraosTRACE_MESSAGE("~WorkerThread Run() complete");
    }

    WorkQueue& parent_;
  };

  /// @brief Support Dtor without race conditions.
  SemaphoreBinary worker_thread_complete_;

  queue_type queue_;

  WorkerThread worker_thread_;

  /// @brief If true, that's mean we can't push new item in queue and class
  /// will be destroyed soon with thread.
  bool if_we_need_destroy_thread_{false};

  bool if_need_start_thread_;

  bool is_thread_started_{false};
};

}  // namespace paraos

#endif /* PARAOS_WORK_QUEUE_HPP */
