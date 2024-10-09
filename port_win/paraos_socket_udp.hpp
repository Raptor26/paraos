/// @file paraos_socket_udp.hpp
/// @author Vyhodcev Egor (vyhodcev@internet.ru)
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

#ifndef PARAOS_SOCKET_UDP_HPP
#define PARAOS_SOCKET_UDP_HPP

// clang-format off
// winsock2.h must include before windows.h
#include <winsock2.h>
#include <windows.h>
// clang-format on

#include <cstdint>
#include <iostream>
#include <string>

#include "paraos_iserial.hpp"
#include "paraos_thread.hpp"

namespace paraos {

/// @brief Порт по умолчанию, который прослушивают НСУ (QGroundControl,
/// MissionPlanner)
inline constexpr uint16_t default_gcs_port = 14550;
/// @brief Тайм-аут на приём данных по умолчанию, мс. (Значение 0 означает, что
/// время ожидания будет бесконечно).
inline constexpr int default_recv_timeout_ms = 0;

/// @brief Атрибуты класса UDP сокета, передаваемые ему при инициализации.
struct UDPSocketAttrs {
  /// @brief IP адрес UDP соединения (сервера).
  std::string ip_address = "127.0.0.1";
  /// @brief Порт, который прослушивает сервер.
  uint16_t port = default_gcs_port;
  /// @brief Тайм-аут на приём данных, мс.
  int recv_timeout_ms = default_recv_timeout_ms;
};

/// @brief Класс UDP сокета, реализующего интерфейс, описывающий методы
/// коммуникации.
class UDPSocket : public paraos::ISerial {
 public:
  /// @brief Конструктор UDPSocket.
  /// @param[in] attrs: Атрибуты UDP сокета.
  UDPSocket(UDPSocketAttrs &attrs) {
    if (WSAStartup(MAKEWORD(2, 2), &wsa_) != 0) {
      is_init_succeeded_ = false;
    } else {
      client_socket_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
      if (client_socket_ == INVALID_SOCKET) {
        is_init_succeeded_ = false;
      } else {
        // Установка тайм-аута на приём данных из сокета.
        auto result = setsockopt(
            client_socket_, SOL_SOCKET, SO_RCVTIMEO,
            (const char *)&attrs.recv_timeout_ms,
            sizeof(attrs.recv_timeout_ms));

        if (result != SOCKET_ERROR) {
          server_.sin_family = AF_INET;
          server_.sin_addr.s_addr = inet_addr(attrs.ip_address.c_str());
          server_.sin_port = htons(attrs.port);
        } else {
          is_init_succeeded_ = false;
        }
      }
    }
  }

  /// @brief Метод выполняет запись заданного количества полученных из сокета
  /// байтов в указанную область памяти.
  /// @param[in] dst: Указатель на область памяти, в которую необходимо записать
  /// полученные данные.
  /// @param[in] dst_size: Количество байтов, которое необходимо считать и
  /// записать.
  /// @return Возвращает количество полученных байтов.
  auto Receive(void *dst, size_t dst_size) -> size_t override {
    size_t read_bytes_num = 0;
    int slen = sizeof(sockaddr_in);

    // When socket not connected, recvfrom (see below) return control
    // immediately. We want wait some time before check connection again.
    if (!is_connected_) {
      paraos::Thread::SleepMs(500u);
    }

    // If no incoming data is available at the socket, the recvfrom function
    // blocks and waits for data to arrive according to the blocking rules
    // defined for WSARecv with the MSG_PARTIAL flag not set unless the socket
    // is nonblocking.
    read_bytes_num = recvfrom(
        client_socket_, static_cast<char *>(dst), static_cast<int>(dst_size), 0,
        (sockaddr *)&server_, &slen);

    if (read_bytes_num == static_cast<size_t>(SOCKET_ERROR)) {
      std::cout << "recvfrom() failed with error code: " << WSAGetLastError()
                << "\n";
      read_bytes_num = 0;
      is_connected_ = false;
    } else {
      is_connected_ = true;
    }

    return read_bytes_num;
  }

  /// @brief Метод выполняет отправку заданного количества байтов из указанной
  /// области памяти на сервер.
  /// @param[in] src: Указатель на область памяти, данные из которой необходимо
  /// отправить.
  /// @param[in] msg_size: Количество байтов, которое необходимо передать.
  /// @return Возвращает количество переданных байтов.
  auto Transmit(void *src, size_t msg_size) -> size_t override {
    size_t transmitted_bytes_num = 0;
    transmitted_bytes_num = sendto(
        client_socket_, reinterpret_cast<const char *>(src),
        static_cast<int>(msg_size), 0, (sockaddr *)&server_,
        sizeof(sockaddr_in));

    return transmitted_bytes_num;
  }

  ~UDPSocket() override {
    closesocket(client_socket_);
    WSACleanup();
  }

 private:
  WSADATA wsa_{};
  SOCKET client_socket_ = 0;
  sockaddr_in server_{};

  bool is_init_succeeded_ = true;

  bool is_connected_{false};
};

}  // namespace paraos

#endif /* PARAOS_SOCKET_UDP_HPP */
