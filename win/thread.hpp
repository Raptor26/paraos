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
#include <unordered_set>
#include <vector>

namespace paraos {
using thread_handle = HANDLE;

constexpr DWORD max_thread{64};

struct ThreadBase {
  virtual ~ThreadBase() = default;

  virtual void Processing() = 0;

  thread_handle handles_storage_ = nullptr;
};

class Thread {
  /// @brief Буфер дескрипторов созданных потоков. По умолчанию, все потоки
  /// создаются в приостановленном состоянии. При вызове StartScheduler() с
  /// помощью записанных в вектор дескрипторов выполняется запуск всех
  /// созданных потоков.
  static inline std::vector<thread_handle> handles_storage_;
  static_assert(std::is_pointer_v<thread_handle> == true);

 public:
  ~Thread() { CloseAllHandles(); }

  thread_handle Make(ThreadBase& threadable) {
    DWORD thread_id;

    threadable.handles_storage_ = CreateThread(
        nullptr, 0, CallPoint, reinterpret_cast<void*>(&threadable),
        CREATE_SUSPENDED, &thread_id);

    auto handle = CreateThread(nullptr, 0, CallPoint,
                               reinterpret_cast<void*>(&threadable),
                               CREATE_SUSPENDED, &thread_id);

    if (threadable.handles_storage_ != nullptr) {
      CriticalSection critical;

      try {
        handles_storage_.push_back(threadable.handles_storage_);

      } catch (std::bad_alloc& exception) {
        std::cerr << "Thead::Make() vector bad alloc: " << exception.what();
        auto close_status = CloseHandle(threadable.handles_storage_);
        assert(close_status != 0);
        threadable.handles_storage_ = nullptr;
      }
    }

    return threadable.handles_storage_;
  }

  void StartScheduler() noexcept {
    for (auto handle : handles_storage_) {
      ResumeThread(handle);
    }

    // В POSIX мы бы вызвали join для каждого потока
    WaitForMultipleObjects(handles_storage_.size(), handles_storage_.data(),
                           TRUE, INFINITE);

    // К данной точке выполнения программы все потоки завершили свое
    // выполнение.
    CloseAllHandles();
  }

  /// @brief
  /// @note После вызова Delete(), handle становиться невалидным.
  /// @param handle
  /// @return
  bool Delete(const thread_handle handle) {
    bool is_thread_deleted = false;
    CriticalSection critical;

    // Перед удалением потока необходимо убедиться что его дескриптор
    // присутствует в хранилище
    if (auto iter = std::find(handles_storage_.cbegin(),
                              handles_storage_.cend(), handle);
        iter != handles_storage_.cend()) {
      if (CloseHandle(*iter) == TRUE) {
        // Т.к повторный вызов CloseHandle() для закрытого дескриптора является
        // ошибкой, то необходимо исключить возможность повторного вызова
        // CloseHandle() для закрытого потока. Для этого удалим из хранилища
        // дескриптор завершенного потока.
        handles_storage_.erase(iter);

        is_thread_deleted = true;
      }
    }

    return is_thread_deleted;
  }

 private:
  void CloseAllHandles() {
    while (handles_storage_.size() != 0u) {
      CriticalSection critical;

      /// Т.к. вызов Delete() вызывает erase(), что инвалидирует итератор, то
      /// используется цикл while() в котором на каждой итерации берется новый
      /// итератор, содержащий указатель на дескриптор потока который нужно
      /// удалить
      auto iter_begin = handles_storage_.crbegin();
      const bool is_thread_deleted = Delete(*iter_begin);
      assert(is_thread_deleted == true);
    }
  }

  /// @brief
  /// @param params
  /// @return
  static DWORD WINAPI CallPoint(LPVOID params) {
    auto ptr_this = reinterpret_cast<ThreadBase*>(params);
    ptr_this->Processing();

    // Если бы использовался freeRTOS, то вызвали "vTaskDelete(nullptr)"
    return 0;
  }
};

inline Thread ThreadFactory;
}  // namespace paraos

#endif /* THREAD_HPP */
