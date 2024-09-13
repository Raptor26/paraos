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
  // Dtor free resources only after thread body in perform_work() complete
  // execute.
  std::size_t delay_ms{4000};
  auto is_sem_taken = is_thread_complete_sem_.Take(delay_ms);
  PARAOS_ATTR_UNUSED_VAR(is_sem_taken);

  PARAOS_CHECK_ASSERT(
      is_sem_taken &&
      "If you create thread, you must call Thread::StartScheduler() in "
      "main(), otherwise, destructor can't safely delete thread");

  const paraos::CriticalSection critical;

  if (handle_) {
    // Если поток не удалял себя из планировщика, будучи откреплённым от
    // основного потока.
    if (is_thread_created_) {
      vTaskDelete(handle_);
    }

    // Необходимо сбросить дескриптор потока с целью избежать повторного
    // удаления потока
    handle_ = nullptr;

    paraosTRACE_MESSAGE("Thread deleted: " << name_);

    is_thread_created_ = false;
  }
}

std::string_view Thread::Name() { return name_; }

TaskHandle_t Thread::Handle() { return handle_; }

void Thread::DelayMs(std::size_t sleep_ms) {
  vTaskDelay(static_cast<TickType_t>(PARAOS_ConvertMsToTicks(sleep_ms)));
}

void Thread::Start() {
  // if scheduler already started, thread will be created in running state.
  Make();
}

auto Thread::Join() -> bool {
  bool join_result = true;

  /* Make sure pthread is joinable. Otherwise, this function would block
   * forever waiting for an unjoinable thread. */
  if (!is_joinable_ || !is_thread_created_) {
    join_result = false;
  }

  /* Only one thread may attempt to join another. Lock the join mutex
   * to prevent other threads from calling pthread_join on the same thread. */
  if (join_result == true) {
    if (join_mutex_.Lock(0) != true) {
      /* Another thread has already joined the requested thread, which would
       * cause this thread to wait forever. */
      join_result = false;
    }
  }

  if (join_result == true) {
    /* Wait for the joining thread to finish. Because this call waits forever,
     * it should never fail. */
    is_thread_complete_sem_.Take();

    /* Create a critical section to clean up the joined thread. */
    const paraos::CriticalSection critical;

    is_thread_complete_sem_.Give();

    join_mutex_.Unlock();

    /* End the critical section. */
  }

  return join_result;
}

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
  const paraos::CriticalSection critical;
  while (!queue_thread_obj_.empty()) {
    // Мы получаем ссылку на элемент в очереди, при этом при вызове front()
    // элемент из очереди не удаляется
    auto &thread_ptr = queue_thread_obj_.front();

    thread_ptr->~Thread();

    queue_thread_obj_.pop_front();

    // нет необходимости вызывать pop() с целью удаления объекта потока из
    // очереди для queue_thread_obj_. Деструктор ~Thread() самостоятельно
    // удалит ссылку на себя из очереди
  }

  // Если сработало утверждение ниже, то возможно это связано с тем, что в
  // момент извлечения крайнего дескриптора потока из очереди, другой поток
  // поместил новый объект в очередь (критическая секция позволяет избежать
  // подобного состояния)
  PARAOS_CHECK_ASSERT(
      queue_thread_obj_.empty() &&
      "Container for pointers threadable objects must be empty, otherwise "
      "some thread not deleted");
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
  // Guard to prevent double thread creation for single 'Thread' object.
  if (!is_thread_created_) {
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
    is_thread_created_ = true;

    const paraos::CriticalSection critical;
    queue_thread_obj_.push_back(this);
  }
}

auto Thread::IsNeedWhile() const -> bool { return is_need_while_; }

void Thread::ExitThread() {
  is_thread_complete_sem_.Give();
  is_thread_created_ = false;
  vTaskDelete(nullptr);
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