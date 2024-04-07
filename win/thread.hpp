/// @file thread.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// @brief
///
/// @version 0.1.0
/// @date 2024-04-06
///
/// @copyright Copyright (c) 2024 Mickle Isaev
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

#ifndef THREAD_HPP
#define THREAD_HPP

#include <assert.h>
#include <windows.h>

#include <algorithm>
#include <functional>
#include <iostream>
#include <new>
#include <type_traits>
#include <vector>

namespace paraos {
using thread_handle = HANDLE;

constexpr DWORD max_thread{64};

struct ThreadBase {
  virtual ~ThreadBase() = default;

  virtual void Processing() = 0;

  thread_handle handle_ = nullptr;
};

class Thread {
  /// @brief Буфер дескрипторов созданных потоков. По умолчанию, все потоки
  /// создаются в приостановленном состоянии. При вызове StartScheduler() с
  /// помощью записанных в вектор дескрипторов выполняется запуск всех
  /// созданных потоков.
  static inline std::vector<thread_handle> handle_;
  static_assert(std::is_pointer_v<thread_handle> == true);

  //   static inline std::vector<ThreadBase> thead_;

 public:
  thread_handle Make(ThreadBase& threadable) {
    DWORD thread_id;

    threadable.handle_ = CreateThread(nullptr, 0, CallPoint,
                                      reinterpret_cast<void*>(&threadable),
                                      CREATE_SUSPENDED, &thread_id);

    if (threadable.handle_ != nullptr) {
      try {
        handle_.push_back(threadable.handle_);
      } catch (std::bad_alloc& exception) {
        std::cerr << "bad_alloc detected: " << exception.what();

        auto close_status = CloseHandle(threadable.handle_);
        assert(close_status != 0);
      }
    }

    return threadable.handle_;
  }

  /// @brief Освобождает память, выделенную под поток только в том случае,
  /// если поток завершил свое выполнение.
  /// @details В многопоточном программировании неизвестно в какой точке
  /// выполнения находится поток. Может случится такая ситуация, при которой
  /// поток, который мы хотим уничтожить, запросил динамические ресурсы.
  /// Тогда, перед его уничтожением, необходимо убедиться что поток освободил
  /// ресурсы, иначе будет утечка памяти. Стандартизированных механизмов для
  /// подобной проверки нет. По этой причине считаем, что поток готов к
  /// уничтожению только в том случае, если он вызвал оператор return, т.е.
  /// завершился штатным с точки зрения разработчика образом.
  /// @param[in] threadable: Ссылка на класс, которой содержит поток, ресурсы
  /// которого необходимо освободить.
  /// @return Возвращает true в случае, если поток уже завершил свое
  /// выполнение к моменту вызова IfThreadCompleteThenFree() и ресурсы,
  /// выделенные под поток успешно освобождены.
  /// @return false - в случае, если поток не завершил свою работу и его
  /// ресурсы не освобождены.
  bool IfThreadCompleteThenFree(ThreadBase& threadable) {
#if 0
    if (threadable.SetTerminateSignal() == true) {
      // В win api необходимо вернуться из функции потока. Это будет
      // эквивалентно его удалению. В деструкторе ThreadBase должен выдаваться
      // семафор, сигнализирующий о том что поток завершил свое выполнение.
    } else {
      // НПоток не освободил занимаемые им ресурсы. Или не переопределены
      // SetTerminateSignal() и IsReadyToTerminate()
      //   assert(true == false);
    }
#endif

    return false;
  }

  /// @brief Ожидает завершение выполнения потока и затем освобождает
  /// выделенные под него ресурсы.
  /// @param[in] threadable: Ссылка на класс, которой содержит поток, ресурсы
  /// которого необходимо освободить.
  /// @return
  bool WaitTheadCompleteThenFree(ThreadBase& threadable) { return false; }

  void StartScheduler() noexcept {
    for (auto handle : handle_) {
      ResumeThread(handle);
    }

    WaitForMultipleObjects(handle_.size(), handle_.data(), TRUE, INFINITE);

    // К данной точке выполнения программы все потоки завершили свое
    // выполнение.
    for (auto handle : handle_) {
      CloseHandle(handle);
    };

    handle_.clear();
  }

  /// @brief
  /// @note Т.к. вектор handle_ содержит указатели, которые не могут выбросить
  /// исключение, то операция handle_.erase() является noexcept. Других методов,
  /// которые могли бы выбросить исключения нет. По этой причине CallPoint()
  /// помечена noexcept.
  /// @param params
  /// @return
  static DWORD WINAPI CallPoint(LPVOID params) noexcept {
    auto ptr_this = reinterpret_cast<ThreadBase*>(params);
    ptr_this->Processing();

    int return_code{0};
    auto handle = ptr_this->handle_;
    if (auto iter = std::find(handle_.cbegin(), handle_.cend(), handle);
        iter != handle_.cend()) {
      // todo Добавить критическую секцию
      CloseHandle(*iter);
      handle_.erase(iter);
      ptr_this->handle_ = nullptr;
    } else {
      // По каким-то причинам, мы не нашли дескриптор потока в контейнере
      // handle_
      return_code = 1;
    }

    assert(return_code == 0);

    // Если бы использовался freeRTOS, то вызвали "vTaskDelete(nullptr)"
    return return_code;
  }

  template <class C, typename Ret, typename... Args>
  Ret Invoke(Ret (C::*method)(Args...), C* instance, Args... args) {
    return std::invoke(method, instance, std::forward<Args>(args)...);
  }

#if 0
  hThreadArray[i] =
      CreateThread(NULL,                  // default security attributes
                   0,                     // use default stack size
                   MyThreadFunction,      // thread function name
                   pDataArray[i],         // argument to thread function
                   0,                     // use default creation flags
                   &dwThreadIdArray[i]);  // returns the thread identifier
#endif
};

inline Thread ThreadFactory;
}  // namespace paraos

#endif /* THREAD_HPP */
