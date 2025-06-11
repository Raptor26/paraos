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

// NOLINTBEGIN(misc-include-cleaner, readability-magic-numbers)
#include <array>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>

#include "paraos_critical.hpp"
#include "paraos_runtime_profiler.hpp"
#include "paraos_socket_udp.hpp"
#include "paraos_thread.hpp"
#include "paraos_thread_common.hpp"
#include "paraos_utils.hpp"

/// @brief Время ожидания входных данных, задаваемое для неблокирующего сокета,
/// мс.
inline constexpr paraos::delay_type nonblocking_socket_timeout_ms{0};

/// @brief Время ожидания входных данных, задаваемое для "пустого" сокета, мс.
inline constexpr paraos::delay_type empty_socket_timeout_ms{0};

/// @brief Время ожидания входных данных, задаваемое для блокирующего сокета с
/// ограниченным временем ожидания, мс.
inline constexpr paraos::delay_type blocking_socket_timeout_ms{1500};

/// @brief Размер массива для хранения принятых данных.
inline constexpr size_t array_size{50};

/// @brief Размер стека у потоков в данном примере.
inline constexpr size_t threads_stack_size{512};

/// @brief Порт сервера, который принимает и отправляет данные.
inline constexpr uint16_t server_port{8080};

/// @brief Порт "пустого" сервера.
inline constexpr uint16_t empty_server_port{9090};

/// @brief Тайм-аут потока сокета с заданным временем ожидания, мс.
inline constexpr paraos::delay_type blocking_sock_thread_delay_ms{50};

/// @brief Тайм-аут потока "пустого" сокета, мс.
inline constexpr paraos::delay_type empty_sock_thread_delay_ms{800};

/// @brief Тайм-аут потока неблокирующего сокета, мс.
inline constexpr paraos::delay_type nonblocking_sock_thread_delay_ms{70};

/// @brief Количество итераций работы неблокирующего сокета.
inline constexpr size_t nonblocking_socket_iterations_count{10};

/// @brief Количество итераций работы "пустого" сокета.
inline constexpr size_t empty_socket_iterations_count{20};

/// @brief Количество итераций работы сокета с заданным временем ожидания.
inline constexpr size_t blocking_socket_iterations_count{7};

namespace {
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
}  // namespace

/// @brief Структура потока, использующего неблокирующий сокет.
struct NonBlockingSocketThread {
  explicit NonBlockingSocketThread(
      paraos::UDPSocket* socket_ptr, const paraos::ThreadAttr& attr)
      : thread_{attr}, socket_ptr_{socket_ptr} {
    thread_.RegisterDelegate(
        paraos::thread_delegate_type::create<
            NonBlockingSocketThread, &NonBlockingSocketThread::Run>(*this));
  }

  void Run() {
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
        std::cout << "\n";

        std::cout << "NON BLOCKING SOCKET TIMED OUT:" << "\n";
        std::cout << "\tEXPECTED WAITING FOR " << nonblocking_socket_timeout_ms
                  << " ms." << "\n";
        std::cout << "\tACTUAL WAITED FOR " << waiting_time << " ms."
                  << "\n";
      }

      timed_out_counter_++;
      if (timed_out_counter_ == nonblocking_socket_iterations_count) {
        socket_ptr_->Transmit(
            static_cast<void*>(disconnect_data.data()), disconnect_data.size());

        std::cout << "\t~NON BLOCKING thread EXITING~" << "\n";
        thread_.Finished();
        nonblocking_thread_exit_flag = true;
      }
    } else {
      {
        const paraos::CriticalSection critical_section;
        std::cout << "\n";

        std::cout << "NON BLOCKING SOCKET GOT DATA FROM THE SERVER: "
                  << receiver_array_.data() << "\n";
        std::cout << "\tEXPECTED WAITING FOR " << nonblocking_socket_timeout_ms
                  << " ms." << "\n";
        std::cout << "\tACTUAL WAITED FOR " << waiting_time << " ms."
                  << "\n";
      }
    }

    paraos::Thread::DelayMs(nonblocking_sock_thread_delay_ms);
  }

 private:
  paraos::Thread thread_;

  paraos::UDPSocket* socket_ptr_;

  std::array<uint8_t, array_size> receiver_array_{0};

  size_t timed_out_counter_{0};
};

/// @brief Структура потока, использующего "пустой" неблокирующий сокет,
/// необходима для проверки неблокирующего режима у сокета.
struct EmptySocketThread {
  explicit EmptySocketThread(
      paraos::UDPSocket* socket_ptr, const paraos::ThreadAttr& attr)
      : thread_{attr}, socket_ptr_{socket_ptr} {
    thread_.RegisterDelegate(
        paraos::thread_delegate_type::create<
            EmptySocketThread, &EmptySocketThread::Run>(*this));
  }

  void Run() {
    empty_socket_profiler.Start();
    // Для получения данных через сокет используется метод Receive().
    auto received_bytes_num = socket_ptr_->Receive(
        static_cast<void*>(receiver_array_.data()), receiver_array_.size());

    empty_socket_profiler.Stop();

    auto waiting_time = empty_socket_profiler.LastDurationMs();

    if (received_bytes_num == 0) {
      {
        const paraos::CriticalSection critical_section;
        std::cout << "\n";

        std::cout << "EMPTY SOCKET TIMED OUT:" << "\n";
        std::cout << "\tEXPECTED WAITING FOR " << empty_socket_timeout_ms
                  << " ms." << "\n";
        std::cout << "\tACTUAL WAITED FOR " << waiting_time << " ms."
                  << "\n";
      }
    }

    cycle_counter_++;

    if (cycle_counter_ == empty_socket_iterations_count) {
      std::cout << "\t~EMPTY SOCKET EXITING.~" << "\n";
      thread_.Finished();
    }

    paraos::Thread::DelayMs(empty_sock_thread_delay_ms);
  }

 private:
  paraos::Thread thread_;

  paraos::UDPSocket* socket_ptr_;

  std::array<uint8_t, array_size> receiver_array_{0};

  size_t cycle_counter_{0};
};

/// @brief Структура потока, использующего сокет с заданным временем ожидания
/// входных данных.
struct BlockingSocketThread {
  explicit BlockingSocketThread(
      paraos::UDPSocket* socket_ptr, const paraos::ThreadAttr& attr)
      : thread_{attr}, socket_ptr_{socket_ptr} {
    thread_.RegisterDelegate(
        paraos::thread_delegate_type::create<
            BlockingSocketThread, &BlockingSocketThread::Run>(*this));
  }

  void Run() {
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
        std::cout << "\n";

        std::cout << "BLOCKING SOCKET GOT DATA FROM THE SERVER: "
                  << receiver_array_.data() << "\n";
        std::cout << "\tEXPECTED WAITING FOR " << blocking_socket_timeout_ms
                  << " ms." << "\n";
        std::cout << "\tACTUAL WAITED FOR " << waiting_time << " ms."
                  << "\n";
      }

      data_counter_++;
      if (data_counter_ == blocking_socket_iterations_count) {
        socket_ptr_->Transmit(
            static_cast<void*>(disconnect_data.data()), disconnect_data.size());

        std::cout << "\t~BLOCKING THREAD EXITING~" << "\n";
        thread_.Finished();
        blocking_thread_exit_flag = true;
      }
    } else {
      {
        const paraos::CriticalSection critical_section;
        std::cout << "\n";

        std::cout << "BLOCKING SOCKET TIMED OUT:" << "\n";
        std::cout << "\tEXPECTED WAITING FOR " << blocking_socket_timeout_ms
                  << " ms." << "\n";
        std::cout << "\tACTUAL WAITED FOR " << waiting_time << " ms."
                  << "\n";
      }
    }

    paraos::Thread::DelayMs(blocking_sock_thread_delay_ms);
  }

 private:
  paraos::Thread thread_;

  paraos::UDPSocket* socket_ptr_;

  std::array<uint8_t, array_size> receiver_array_{0};

  size_t data_counter_{0};
};

/// @brief Структура потока, использующего сокет с неограниченным временем
/// ожидания входных данных.
struct ForeverBlockingSocketThread {
  explicit ForeverBlockingSocketThread(
      paraos::UDPSocket* socket_ptr, const paraos::ThreadAttr& attr)
      : thread_{attr}, socket_ptr_{socket_ptr} {
    thread_.RegisterDelegate(
        paraos::thread_delegate_type::create<
            ForeverBlockingSocketThread, &ForeverBlockingSocketThread::Run>(
            *this));
  }

  void Run() {
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
        std::cout << "\n";

        std::cout << "FOREVER BLOCKING SOCKET GOT DATA FROM THE SERVER: "
                  << receiver_array_.data() << "\n";
        std::cout << "\tEXPECTED WAITING FOREVER" << "\n";
        std::cout << "\tACTUAL WAITED FOR " << waiting_time << " ms."
                  << "\n";
      }

    } else {
      // Данный участок кода никогда не должен быть вызван, поскольку сокет
      // имеет неограниченное время ожидания входных данных.
      {
        const paraos::CriticalSection critical_section;
        std::cout << "\n";

        std::cout << "FOREVER BLOCKING SOCKET TIMED OUT:" << "\n";
        std::cout << "\tEXPECTED WAITING FOREVER" << "\n";
        std::cout << "\tACTUAL WAITED FOR " << waiting_time << " ms."
                  << "\n";
      }
    }

    // Если потоки с ограниченным временем ожидания завершили свою работу,
    // данный поток также завершается.
    if ((nonblocking_thread_exit_flag) && (blocking_thread_exit_flag)) {
      socket_ptr_->Transmit(
          static_cast<void*>(disconnect_data.data()), disconnect_data.size());

      std::cout
          << "\t~Other threads finished, FOREVER BLOCKING THREAD EXITING.~"
          << "\n";
      thread_.Finished();
      paraos::Thread::Exit();
    }
  }

 private:
  paraos::Thread thread_;

  paraos::UDPSocket* socket_ptr_;

  std::array<uint8_t, array_size> receiver_array_{0};
};

auto main() -> int {
  // Аттрибуты для инициализации неблокирующего сокета
  paraos::UDPSocketAttrs nonblocking_attrs{};

  nonblocking_attrs.port = server_port;
  // Необходимо указать значение времени ожидания входных данных равное нулю.
  nonblocking_attrs.recv_timeout_ms = nonblocking_socket_timeout_ms;

  // Инициализация неблокирующего сокета.
  paraos::UDPSocket nonblocking_socket{nonblocking_attrs};

  // Инициализация "пустого" неблокирующего сокета, который, в рамках данного
  // примера, не должен получать никаких данных. В таком случае можно будет
  // проверить работу сокета в неблокирующем режиме.
  paraos::UDPSocketAttrs empty_socket_attrs{};

  // Порт "пустого" сервера, который не будет отправлять никакие данные.
  empty_socket_attrs.port = empty_server_port;
  // Необходимо указать значение времени ожидания входных данных равное нулю.
  empty_socket_attrs.recv_timeout_ms = empty_socket_timeout_ms;

  // Инициализация неблокирующего сокета.
  paraos::UDPSocket empty_socket{empty_socket_attrs};

  // Аттрибуты для инициализации сокета с заданным временем ожидания в мс.
  paraos::UDPSocketAttrs blocking_attrs{};

  blocking_attrs.port = server_port;
  // Время ожидания для каждой итерации получения входных данных - полсекунды.
  blocking_attrs.recv_timeout_ms = blocking_socket_timeout_ms;

  // Инициализация сокета с заданным временем ожидания входных данных.
  paraos::UDPSocket blocking_socket{blocking_attrs};

  // Аттрибуты для инициализации сокета с неограниченным временем ожидания
  // входных данных.
  paraos::UDPSocketAttrs forever_blocking_attrs{};

  forever_blocking_attrs.port = server_port;

  // Для инициализации сокета неограниченным временем ожидания
  // входных данных необходимо оставить значение по умолчанию для поля
  // "recv_timeout_ms" в аттрибутах.
  paraos::UDPSocket forever_blocking_socket{forever_blocking_attrs};

  // Аналогично можно явным образом указать в вышеупомянутом поле значение,
  // равное максимальному времени ожидания для используемой платформы:
  forever_blocking_attrs.recv_timeout_ms = paraos::max_delay;

  // Инициализация потоков, работающих с созданными сокетами.
  {
    paraos::ThreadAttr attr{};
    attr.thread_name = "Nonblocking thread";
    const static NonBlockingSocketThread non_blocking_thread{
        &nonblocking_socket, attr};
  }

  {
    paraos::ThreadAttr attr{};
    attr.thread_name = "Empty socket thread";
    const static EmptySocketThread empty_socket_thread{&empty_socket, attr};
  }

  {
    paraos::ThreadAttr attr{};
    attr.thread_name = "Blocking socket thread";
    const static BlockingSocketThread blocking_thread{&blocking_socket, attr};
  }

  {
    paraos::ThreadAttr attr{};
    attr.thread_name = "Forever blocking socket thread";
    const static ForeverBlockingSocketThread forever_blocking_thread{
        &forever_blocking_socket, attr};
  }

  paraos::Thread::StartScheduler();
  paraos::Thread::DeleteAll();

  exit(0);
}
// NOLINTEND(misc-include-cleaner, readability-magic-numbers)
