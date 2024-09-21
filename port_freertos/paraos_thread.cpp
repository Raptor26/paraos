/// @file paraos_thread.cpp
/// @author VyhodcevEgor <vyhodcev@internet.ru>
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

#include "paraos_thread.hpp"

#include "gsl/gsl"
#include "paraos_trace.hpp"
#include "paraos_utils.hpp"

namespace paraos {

Thread::Thread(
    const std::string name, std::size_t stack_depth, ThreadPriority priority,
    bool is_joinable)
    : name_{name},
      stack_depth_{stack_depth},
      priority_{priority},
      is_joinable_{is_joinable} {
  // Now Dtor can delete thread.
  is_thread_complete_sem_.Give();
}

Thread::~Thread() {
  is_thread_makeable_ = false;
  if (handle_) {
    // Dtor free resources only after thread body in MyThreadFunction() complete
    // execute.
    std::size_t delay_ms{4000};

    // Destructor restore execute ony if  MyThreadFunction() complete
    auto is_sem_taken = is_thread_complete_sem_.Take(delay_ms);
    PARAOS_ATTR_UNUSED_VAR(is_sem_taken);

    PARAOS_CHECK_ASSERT(
        is_sem_taken &&
        "If you create thread, you must call Thread::StartScheduler() in "
        "main(), otherwise, destructor can't safely delete thread");

#if 1
    // If we ended up here, that's mean MyThreadFunction() complete and
    // ExitThread() was called. That's mean thread already destroyed and handle_
    // == nullptr.
    // If handle_ != nullptr, that's mean user code don't call Thread::Start()
    // and thread don't was created. In this case nothing thread for delete.
    PARAOS_CHECK_ASSERT(handle_ || !handle_);
#endif
  }
  paraosTRACE_MESSAGE("Thread deleted: " << name_);
}

std::string_view Thread::Name() const { return name_; }

TaskHandle_t Thread::Handle() const { return handle_; }

void Thread::DelayMs(std::size_t sleep_ms) {
  vTaskDelay(static_cast<TickType_t>(PARAOS_ConvertMsToTicks(sleep_ms)));
}

void Thread::Start() {
  // if condition below true, destructor not called and we cal make thread.
  if (is_thread_makeable_) {
    // if scheduler already started, thread will be created in running state.
    const paraos::CriticalSection critical;
    Make();
  }
}

auto Thread::Join() -> bool {
  bool join_result{false};

  if (is_joinable_ && handle_) {
    join_result = true;

    /* Wait for the joining thread to finish. Because this call waits forever,
     * it should never fail. */
    is_thread_complete_sem_.Take(max_delay);

    // Give semaphore for destructor, otherwise destructor will be wait this sem
    // forever and can't delete class object which is the heir of the
    // Thread class.
    is_thread_complete_sem_.Give();
  }

  return join_result;
}

#if 0
auto Thread::Detach() -> bool {
  bool detach_result = false;
  if (is_joinable_ == true && is_thread_created_ == true) {
    const paraos::CriticalSection critical;
    auto thread_state = eTaskGetState(handle_);
    if (thread_state != eDeleted && thread_state != eInvalid) {
      if (thread_state == eSuspended) {
        is_thread_created_ = false;
        detach_result = true;
        vTaskDelete(handle_);
      } else {
        is_joinable_ = false;
        detach_result = true;
      }
    }
  }

  return detach_result;
}
#endif

void Thread::Run() {
  // Если сработал данный PARAOS_CHECK_ASSERT, то конструктор производного от
  // Thread класса не успел завершить конструирование объекта до того момента
  // когда планировщик ОС вызвал метод Run() (производные классы всегда должны
  // переопределять метод Run()). Одним из возможных способов решения
  // являются:
  // - Переопределите в производном классе метод Run(). Это самый тривиальный
  //   случай. Возможно вы просто забыли определить тело вашего потока в
  //   методе Run().
  //
  // Пункты ниже рассматривайте только в том случае, если в производном классе
  // определен метод Run() с аннотацией override:
  //
  // - Вызов метода Make() в теле конструктора производного класса. Это
  //   гарантирует, что поток создается после того, как компилятор подставил
  //   указатель на метод Run() из производного класса.
  //
  // - Создание потока в приостановленном состоянии, затем его запуск в теле
  //   конструктора производного класса. Для данного сценария рассуждения
  //   аналогичны пункту выше.
  //
  // - Временное повышение приоритета потока, который создает новый поток. Это
  //   гарантирует, что создающий поток завершит работу конструкторов до того
  //   как планировщик ОС выполнит переключение на выполнение потока
  //   созданного объекта. После завершения создания объекта и его потока,
  //   создающий поток вновь может понизить свой приоритет до исходного
  //   значения.
  PARAOS_CHECK_ASSERT(
      false &&
      "If windows scheduler call this instance, constructor of derived class "
      "not complete its work before scheduler call Run() method");
};

void Thread::StartScheduler() {
  paraosTRACE_MESSAGE("Start Scheduler");
  is_scheduler_started_ = true;
  vTaskStartScheduler();
}

void Thread::StopScheduler() {
  paraosTRACE_MESSAGE("Stop Scheduler");
  is_scheduler_started_ = false;
  vTaskEndScheduler();
}

void Thread::DeleteAll() {
  // In freeRTOS this method is placebo. Need for consistence API with windows
  // and unix ports.
}

bool Thread::SetPriority(const ThreadPriority priority) {
  bool result = false;

  if (priority < ThreadPriority::kMaxNum) {
    priority_ = priority;
    vTaskPrioritySet(handle_, static_cast<UBaseType_t>(priority));
    result = true;
  }

  return result;
}

auto Thread::SetNeedWhile(bool status) -> bool {
  is_need_while_ = status;

  return true;
}

auto Thread::IsSchedulerStarted() -> bool { return is_scheduler_started_; }

void Thread::Make() {
  const paraos::CriticalSection critical;

  // Guard to prevent double thread creation for single 'Thread' object.
  if (!handle_) {
    // Sem was given in Ctor. Now we take sem. That's mean, Dtor can delete
    // object only after MyThreadFunction() complete.
    constexpr std::size_t delay_ms{0u};
    auto is_sem_taken = is_thread_complete_sem_.Take(delay_ms);
    PARAOS_ATTR_UNUSED_VAR(is_sem_taken);

    // If is_sem_taken == false, it's mean error in thread Ctor/Dtor logic.
    PARAOS_CHECK_ASSERT(is_sem_taken && "Sem always must taken");

    xTaskCreate(
        MyThreadFunction, name_.c_str(), stack_depth_, this,
        static_cast<UBaseType_t>(priority_), &handle_);

    PARAOS_CHECK_ASSERT(handle_ && "Thread not created");
  }
}

auto Thread::IsNeedWhile() const -> bool { return is_need_while_; }

void Thread::ExitThread() {
  is_thread_complete_sem_.Give();

  vTaskDelete(nullptr);
  handle_ = nullptr;
}

void Thread::MyThreadFunction(void *lpParam) {
  Thread *thread = static_cast<Thread *>(lpParam);

  // Нужно ли выполнение в теле бесконечного цикла задается при создании
  // потока в конструкторе ThreadBase()
  do {
    thread->Run();
  } while (thread->IsNeedWhile());

  thread->ExitThread();
}

}  // namespace paraos