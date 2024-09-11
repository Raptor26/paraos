/// @file paraos_sequence_thread.hpp
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

#ifndef PARAOS_THREAD_SEQUENCE_HPP
#define PARAOS_THREAD_SEQUENCE_HPP

#include "gsl/gsl"
#include "paraos_bool_atomic.hpp"
#include "paraos_functor.hpp"
#include "paraos_runtime_profiler.hpp"
#include "paraos_semaphore.hpp"
#include "paraos_thread.hpp"

constexpr std::size_t thread_sequence_element_max_numb{20};

namespace paraos {

#if AP_SCHEDULER_EXTENDED_TASKINFO_ENABLED
#define AP_SCHEDULER_NAME_INITIALIZER(_clazz, _name) \
  .name_ = #_clazz "::" #_name,
#define AP_FAST_NAME_INITIALIZER(_clazz, _name) \
  .name_ = #_clazz "::" #_name "*",
#else
#define AP_SCHEDULER_NAME_INITIALIZER(_clazz, _name) .name_ = #_name,
#define AP_FAST_NAME_INITIALIZER(_clazz, _name) .name_ = #_name "*",
#endif
#define LOOP_RATE 0

/*
  useful macro for creating scheduler task table
 */
#define SCHED_TASK_CLASS(                                             \
    classname, classptr, func, rate_hz, max_time_micros, priority)    \
  {.function_ = FUNCTOR_BIND(classptr, &classname::func, void),       \
   AP_SCHEDULER_NAME_INITIALIZER(classname, func).rate_hz_ = rate_hz, \
   .max_time_micros_ = max_time_micros,                               \
   .priority_ = priority}

/*
  useful macro for creating the fastloop task table
 */
#define FAST_TASK_CLASS(classname, classptr, func)              \
  {.function_ = FUNCTOR_BIND(classptr, &classname::func, void), \
   AP_FAST_NAME_INITIALIZER(classname, func).rate_hz_ = 0,      \
   .max_time_micros_ = 0,                                       \
   .priority_ = 0}

FUNCTOR_TYPEDEF(task_fn_t, void);

/// @brief Class for contained params for scheduling one method for execute.
/// Most likely, user want scheduling many method/functions, In this case, user
/// create const array of TaskSequence filetime of which no less than the
/// lifetime of the program. Created array user code put in one of parameters
/// passed in `SequenceThread` ctor.
/// @example For example, see test_paraos_thread_sequence.cpp.
struct TaskSequence {
  task_fn_t function_;
  const char *name_;
  float rate_hz_;
  uint16_t max_time_micros_;
  uint8_t priority_;
};

/// @brief Class for contained runtime info about one task. User code must
/// define array of TaskSequenceInfo, dimension of which equal the dimension
/// TaskSequence array, also defined by user code.
/// @example For example, see test_paraos_thread_sequence.cpp.
struct TaskSequenceInfo {
  size_t last_call_;
};

/// @brief Class for execute scheduling methods from task_sequence struct array.
/// All task for scheduling runs in the one thread context.
/// @tparam PROFILER - class for runtime profiler.
/// @example For example, see test_paraos_thread_sequence.cpp.
template <typename PROFILER = EmptyProfiler>
class SequenceThread : private Thread {
 public:
  /// @brief Ctor of SequenceThread class.
  /// @param[in] name: Name of the thead, in which context will be execute tasks
  /// from task_sequence array.
  /// @param[in] stack_depth: Stack depth for execute all tasks from
  /// task_sequence array.
  /// @param[in] priority: Thread priority for for execute all tasks from
  /// task_sequence array.
  /// @param[in] task_sequence: array of tasks sequence, defined by user code.
  /// @param[in, out] task_sequence_info: array for contained SequenceThread
  /// class info about each task in 'task_sequence' array. That's mean,
  /// dimensions `task_sequence` and `task_sequence_info` arrays must be equal.
  SequenceThread(
      const std::string name, const std::size_t stack_depth,
      const ThreadPriority priority,
      const gsl::span<const TaskSequence> task_sequence,
      const gsl::span<TaskSequenceInfo> task_sequence_info)
      : task_sequence_{task_sequence},
        task_sequence_info_{task_sequence_info},
        Thread{name, stack_depth, priority} {
    assert(
        task_sequence.size() == task_sequence_info.size() &&
        "Dimensions `task_sequence` and `task_sequence_info` arrays must be "
        "equal");

    // Run() method must call in forever loop periodical.
    Thread::SetNeedWhile(true);

    // Method below create thread and scheduling it's for execute in RTOS (or
    // windows/unix).
    Thread::Start();
  }

  ~SequenceThread() { Break(); }

  /// @brief Force break thread execute. Useful in unit tests.
  void Break() {
    const paraos::CriticalSection critical;

    // Run() no more called.
    Thread::SetNeedWhile(false);

    // Give notify for last call all registered methods task_sequence_. It's
    // necessary for resume Run() from blocking mode and complete one iteration.
    // After Run() complete, thread wrapper can safely delete thread (because
    // above we call Thread::SetNeedWhile(false)) and the thead object can be
    // safely deleted in thead dtor.
    NotifyGive();
  }

  /// @brief Give notify for start new cycle of scheduling tasks, written in
  /// task_sequence_.
  /// @note User code must call this method at regular intervals, for example -
  /// in a timer overflow interrupt.
  /// @return
  bool NotifyGive() { return new_cycle_ready_sem_.Give(); }

  /// @brief Method calls periodical in thread loop
  void Run() override {
    // Wait semaphore before try run all methods in array. It's allows call
    // methods with a user-defined period (period with witch user code calls
    // the method NotifyGive()).
    new_cycle_ready_sem_.Take(max_delay);

    for (std::size_t i = 0u, array_size = task_sequence_.size(); i < array_size;
         ++i) {
      task_sequence_[i].function_();
    }
  }

 private:
  SemaphoreBinary new_cycle_ready_sem_;

  const gsl::span<const TaskSequence> task_sequence_;
  const gsl::span<TaskSequenceInfo> task_sequence_info_;
};

}  // namespace paraos

#endif /* PARAOS_THREAD_SEQUENCE_HPP */
