/// @file example_paraos_socket_udp.cpp
/// @author Vyhodcev Egor (vyhodcev@internet.ru)
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

/// NAME
///     Пример использования UDP сокета из библиотеки Paraos.
///
/// DESCRIPTION
///     Данный модуль предоставляет варианты инициализации UDP сокетов
///     (неблокирующий/с заданным временем ожидания входных данных/бесконечное
///     ожидание входных данных) а также примеры их использования внутри
///     раздельных потоков.
///
///     Для сохранения наглядности примера рекомендуется первым запускать модуль
///     "test_udp_socket.py", например, с помощью команды:
///     `python test_udp_socket.py`. Затем можно запускать пример из данного
///     файла.
///
/// NOTE
///     Обратите внимание, что модуль "test_udp_socket.py" завершается
///     корректно, но через непродолжительный промежуток времени после
///     завершения примера из данного модуля - нет необходимости завершать его
///     принудительно (Ctrl + C).

#include <array>
#include <cstring>
#include <iostream>

#include "paraos_critical.hpp"
#include "paraos_runtime_profiler.hpp"
#include "paraos_socket_udp.hpp"
#include "paraos_thread.hpp"

/// @brief Время ожидания входных данных, задаваемое для неблокирующего сокета,
/// мс.
inline constexpr size_t nonblocking_socket_timeout_ms{0};

/// @brief Время ожидания входных данных, задаваемое для "пустого" сокета, мс.
inline constexpr size_t empty_socket_timeout_ms{0};

/// @brief Время ожидания входных данных, задаваемое для блокирующего сокета с
/// ограниченным временем ожидания, мс.
inline constexpr size_t blocking_socket_timeout_ms{1500};

/// @brief Профилировщик для замера задержки ожидания входных данных у
/// неблокирующего сокета.
paraos::OsProfiler nonblocking_socket_profiler{};

/// @brief Профилировщик для замера задержки ожидания входных данных у
/// "пустого" сокета.
paraos::OsProfiler empty_socket_profiler{};

/// @brief Профилировщик для замера задержки ожидания входных данных у
/// сокета с заданным временем ожидания.
paraos::OsProfiler blocking_socket_profiler{};

/// @brief Профилировщик для замера задержки ожидания входных данных у
/// сокета с неограниченным временем ожидания.
paraos::OsProfiler forever_blocking_socket_profiler{};

/// @brief Размер массива для хранения принятых данных.
inline constexpr size_t array_size = 50;

/// @brief Данные, передаваемые на сервер при обычной работе потока.
std::string handshake_data{"Server handshake"};

/// @brief Данные, передаваемые на сервер в момент завершения потоком своей
/// работы.
std::string disconnect_data{"Disconnecting"};

/// @brief Флаг, который устанавливается в true потоком, использующим
/// неблокирующий сокет, во время завершения своей работы.
bool nonblocking_thread_exit_flag = false;

/// @brief Флаг, который устанавливается в true потоком, использующим
/// сокет с заданным временем ожидания, во время завершения своей работы.
bool blocking_thread_exit_flag = false;

/// @brief Структура потока, использующего неблокирующий сокет.
struct NonBlockingSocketThread : public paraos::Thread {
  NonBlockingSocketThread(
      paraos::UDPSocket* socket_ptr,
      const std::string thread_name = "Non blocking thread")
      : paraos::Thread{thread_name, 512, paraos::ThreadPriority::kNormal},
        socket_ptr_{socket_ptr} {
    SetNeedWhile(true);
    Start();
  }

  void Run() override {
    // Для передачи данных через сокет используется метод Transmit().
    socket_ptr_->Transmit(
        static_cast<void*>(handshake_data.data()), handshake_data.size());

    nonblocking_socket_profiler.Start();
    // Для получения данных через сокет используется метод Receive().
    auto received_bytes_num = socket_ptr_->Receive(
        static_cast<void*>(receiver_array_.data()), receiver_array_.size());

    nonblocking_socket_profiler.Stop();

    auto waiting_time = nonblocking_socket_profiler.LastDurationMs();

    if (received_bytes_num == 0) {
      {
        const paraos::CriticalSection critical_section;
        std::cout << std::endl;

        std::cout << "NON BLOCKING SOCKET TIMED OUT:" << std::endl;
        std::cout << "\tEXPECTED WAITING FOR " << nonblocking_socket_timeout_ms
                  << " ms." << std::endl;
        std::cout << "\tACTUAL WAITED FOR " << waiting_time << " ms."
                  << std::endl;
      }

      timed_out_counter_++;
      if (timed_out_counter_ == 10) {
        socket_ptr_->Transmit(
            static_cast<void*>(disconnect_data.data()), disconnect_data.size());

        std::cout << "\t~NON BLOCKING thread EXITING~" << std::endl;
        SetNeedWhile(false);
        nonblocking_thread_exit_flag = true;
      }
    } else {
      {
        const paraos::CriticalSection critical_section;
        std::cout << std::endl;

        std::cout << "NON BLOCKING SOCKET GOT DATA FROM THE SERVER: "
                  << receiver_array_.data() << std::endl;
        std::cout << "\tEXPECTED WAITING FOR " << nonblocking_socket_timeout_ms
                  << " ms." << std::endl;
        std::cout << "\tACTUAL WAITED FOR " << waiting_time << " ms."
                  << std::endl;
      }
    }

    paraos::Thread::SleepMs(70);
  }

 private:
  paraos::UDPSocket* socket_ptr_;
  std::array<uint8_t, array_size> receiver_array_{0};
  size_t timed_out_counter_{0};
};

/// @brief Структура потока, использующего "пустой" неблокирующий сокет,
/// необходима для проверки неблокирующего режима у сокета.
struct EmptySocketThread : public paraos::Thread {
  EmptySocketThread(
      paraos::UDPSocket* socket_ptr,
      const std::string thread_name = "Empty thread")
      : paraos::Thread{thread_name, 512, paraos::ThreadPriority::kBelowNormal},
        socket_ptr_{socket_ptr} {
    SetNeedWhile(true);
    Start();
  }

  void Run() override {
    empty_socket_profiler.Start();
    // Для получения данных через сокет используется метод Receive().
    auto received_bytes_num = socket_ptr_->Receive(
        static_cast<void*>(receiver_array_.data()), receiver_array_.size());

    empty_socket_profiler.Stop();

    auto waiting_time = empty_socket_profiler.LastDurationMs();

    if (received_bytes_num == 0) {
      {
        const paraos::CriticalSection critical_section;
        std::cout << std::endl;

        std::cout << "EMPTY SOCKET TIMED OUT:" << std::endl;
        std::cout << "\tEXPECTED WAITING FOR " << empty_socket_timeout_ms
                  << " ms." << std::endl;
        std::cout << "\tACTUAL WAITED FOR " << waiting_time << " ms."
                  << std::endl;
      }
    }

    cycle_counter_++;

    if (cycle_counter_ == 20) {
      std::cout << "\t~EMPTY SOCKET EXITING.~" << std::endl;
      SetNeedWhile(false);
    }

    paraos::Thread::SleepMs(800);
  }

 private:
  paraos::UDPSocket* socket_ptr_;

  std::array<uint8_t, array_size> receiver_array_{0};

  size_t cycle_counter_{0};
};

/// @brief Структура потока, использующего сокет с заданным временем ожидания
/// входных данных.
struct BlockingSocketThread : public paraos::Thread {
  BlockingSocketThread(
      paraos::UDPSocket* socket_ptr,
      const std::string thread_name = "Blocking thread")
      : paraos::Thread{thread_name, 512, paraos::ThreadPriority::kNormal},
        socket_ptr_{socket_ptr} {
    SetNeedWhile(true);
    Start();
  }

  void Run() override {
    // Для передачи данных через сокет используется метод Transmit().
    socket_ptr_->Transmit(
        static_cast<void*>(handshake_data.data()), handshake_data.size());

    blocking_socket_profiler.Start();

    // Для получения данных через сокет используется метод Receive().
    auto received_bytes_num = socket_ptr_->Receive(
        static_cast<void*>(receiver_array_.data()), receiver_array_.size());

    blocking_socket_profiler.Stop();

    auto waiting_time = blocking_socket_profiler.LastDurationMs();

    if (received_bytes_num != 0) {
      {
        const paraos::CriticalSection critical_section;
        std::cout << std::endl;

        std::cout << "BLOCKING SOCKET GOT DATA FROM THE SERVER: "
                  << receiver_array_.data() << std::endl;
        std::cout << "\tEXPECTED WAITING FOR " << blocking_socket_timeout_ms
                  << " ms." << std::endl;
        std::cout << "\tACTUAL WAITED FOR " << waiting_time << " ms."
                  << std::endl;
      }

      data_counter_++;
      if (data_counter_ == 7) {
        socket_ptr_->Transmit(
            static_cast<void*>(disconnect_data.data()), disconnect_data.size());

        std::cout << "\t~BLOCKING THREAD EXITING~" << std::endl;
        SetNeedWhile(false);
        blocking_thread_exit_flag = true;
      }
    } else {
      {
        const paraos::CriticalSection critical_section;
        std::cout << std::endl;

        std::cout << "BLOCKING SOCKET TIMED OUT:" << std::endl;
        std::cout << "\tEXPECTED WAITING FOR " << blocking_socket_timeout_ms
                  << " ms." << std::endl;
        std::cout << "\tACTUAL WAITED FOR " << waiting_time << " ms."
                  << std::endl;
      }
    }

    paraos::Thread::SleepMs(50);
  }

 private:
  paraos::UDPSocket* socket_ptr_;
  std::array<uint8_t, array_size> receiver_array_{0};
  size_t data_counter_{0};
};

/// @brief Структура потока, использующего сокет с неограниченным временем
/// ожидания входных данных.
struct ForeverBlockingSocketThread : public paraos::Thread {
  ForeverBlockingSocketThread(
      paraos::UDPSocket* socket_ptr,
      const std::string thread_name = "Forever blocking thread")
      : paraos::Thread{thread_name, 512, paraos::ThreadPriority::kNormal},
        socket_ptr_{socket_ptr} {
    SetNeedWhile(true);
    Start();
  }

  void Run() override {
    // Для передачи данных через сокет используется метод Transmit().
    socket_ptr_->Transmit(
        static_cast<void*>(handshake_data.data()), handshake_data.size());

    forever_blocking_socket_profiler.Start();

    // Для получения данных через сокет используется метод Receive().
    auto received_bytes_num = socket_ptr_->Receive(
        static_cast<void*>(receiver_array_.data()), receiver_array_.size());

    forever_blocking_socket_profiler.Stop();

    auto waiting_time = forever_blocking_socket_profiler.LastDurationMs();

    if (received_bytes_num != 0) {
      {
        const paraos::CriticalSection critical_section;
        std::cout << std::endl;

        std::cout << "FOREVER BLOCKING SOCKET GOT DATA FROM THE SERVER: "
                  << receiver_array_.data() << std::endl;
        std::cout << "\tEXPECTED WAITING FOREVER" << std::endl;
        std::cout << "\tACTUAL WAITED FOR " << waiting_time << " ms."
                  << std::endl;
      }

    } else {
      // Данный участок кода никогда не должен быть вызван, поскольку сокет
      // имеет неограниченное время ожидания входных данных.
      {
        const paraos::CriticalSection critical_section;
        std::cout << std::endl;

        std::cout << "FOREVER BLOCKING SOCKET TIMED OUT:" << std::endl;
        std::cout << "\tEXPECTED WAITING FOREVER" << std::endl;
        std::cout << "\tACTUAL WAITED FOR " << waiting_time << " ms."
                  << std::endl;
      }
    }

    // Если потоки с ограниченным временем ожидания завершили свою работу,
    // данный поток также завершается.
    if ((nonblocking_thread_exit_flag == true) &&
        (blocking_thread_exit_flag == true)) {
      socket_ptr_->Transmit(
          static_cast<void*>(disconnect_data.data()), disconnect_data.size());

      std::cout
          << "\t~Other threads finished, FOREVER BLOCKING THREAD EXITING.~"
          << std::endl;
      SetNeedWhile(false);
    }
  }

 private:
  paraos::UDPSocket* socket_ptr_;
  std::array<uint8_t, array_size> receiver_array_{0};
};

auto main() -> int {
  // Аттрибуты для инициализации неблокирующего сокета
  paraos::UDPSocketAttrs nonblocking_attrs{};

  nonblocking_attrs.port = 8080;
  // Необходимо указать значение времени ожидания входных данных равное нулю.
  nonblocking_attrs.recv_timeout_ms = nonblocking_socket_timeout_ms;

  // Инициализация неблокирующего сокета.
  paraos::UDPSocket nonblocking_socket{nonblocking_attrs};

  // Инициализация "пустого" неблокирующего сокета, который, в рамках данного
  // примера, не должен получать никаких данных. В таком случае можно будет
  // проверить работу сокета в неблокирующем режиме.
  paraos::UDPSocketAttrs empty_socket_attrs{};

  // Порт "пустого" сервера, который не будет отправлять никакие данные.
  empty_socket_attrs.port = 9090;
  // Необходимо указать значение времени ожидания входных данных равное нулю.
  empty_socket_attrs.recv_timeout_ms = empty_socket_timeout_ms;

  // Инициализация неблокирующего сокета.
  paraos::UDPSocket empty_socket{empty_socket_attrs};

  // Аттрибуты для инициализации сокета с заданным временем ожидания в мс.
  paraos::UDPSocketAttrs blocking_attrs{};

  blocking_attrs.port = 8080;
  // Время ожидания для каждой итерации получения входных данных - полсекунды.
  blocking_attrs.recv_timeout_ms = blocking_socket_timeout_ms;

  // Инициализация сокета с заданным временем ожидания входных данных.
  paraos::UDPSocket blocking_socket{blocking_attrs};

  // Аттрибуты для инициализации сокета с неограниченным временем ожидания
  // входных данных.
  paraos::UDPSocketAttrs forever_blocking_attrs{};

  forever_blocking_attrs.port = 8080;

  // Для инициализации сокета неограниченным временем ожидания
  // входных данных необходимо оставить значение по умолчанию для поля
  // "recv_timeout_ms" в аттрибутах.
  paraos::UDPSocket forever_blocking_socket{forever_blocking_attrs};

  // Аналогично можно явным образом указать в вышеупомянутом поле значение,
  // равное максимальному времени ожидания для используемой платформы:
  forever_blocking_attrs.recv_timeout_ms = paraos::max_delay;

  // Инициализация потоков, работающих с созданными сокетами.
  NonBlockingSocketThread non_blocking_thread{&nonblocking_socket};
  EmptySocketThread empty_socket_thread{&empty_socket};
  BlockingSocketThread blocking_thread{&blocking_socket};
  ForeverBlockingSocketThread forever_blocking_thread{&forever_blocking_socket};

  paraos::Thread::StartScheduler();
  paraos::Thread::DeleteAll();

  return 0;
}