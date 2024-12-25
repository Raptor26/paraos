#ifndef PARAOS_SOCKET_UDP_HPP
#define PARAOS_SOCKET_UDP_HPP

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <string>

#include "paraos_iserial.hpp"
#include "paraos_thread.hpp"
#include "paraos_utils.hpp"

namespace paraos {

/// @brief Порт по умолчанию, который прослушивают НСУ (QGroundControl,
/// MissionPlanner)
inline constexpr uint16_t default_gcs_port = 14550;
/// @brief Тайм-аут на приём данных по умолчанию, мс. (Значение 0 означает, что
/// время ожидания будет бесконечно).
inline constexpr decltype(paraos::max_delay) default_recv_timeout_ms = 0;

/// @brief Атрибуты класса UDP сокета, передаваемые ему при инициализации.
struct UDPSocketAttrs {
  /// @brief IP адрес UDP соединения (сервера).
  std::string ip_address = "127.0.0.1";

  /// @brief Порт, который прослушивает сервер.
  uint16_t port = default_gcs_port;

  /// @brief Тайм-аут на приём данных, мс.
  std::remove_cv_t<decltype(paraos::max_delay)> recv_timeout_ms =
      paraos::default_recv_timeout_ms;
};

/// @brief Класс UDP сокета, реализующего интерфейс, описывающий методы
/// коммуникации.
class UDPSocket : public paraos::ISerial {
 public:
  /// @brief Конструктор UDPSocket.
  /// @param[in] attrs: Атрибуты UDP сокета.
  UDPSocket(UDPSocketAttrs &attrs) {
    client_socket_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (client_socket_ == -1) {
      is_init_succeeded_ = false;
    } else {
      // Установка тайм-аута на приём данных из сокета.
      struct timeval tv;
      tv.tv_sec = attrs.recv_timeout_ms / 1000;
      tv.tv_usec = 0;
      auto result = setsockopt(
          client_socket_, SOL_SOCKET, SO_RCVTIMEO, (const void *)&tv,
          sizeof(tv));

      if (result != -1) {
        server_.sin_family = AF_INET;
        server_.sin_addr.s_addr = inet_addr(attrs.ip_address.c_str());
        server_.sin_port = htons(attrs.port);
      } else {
        is_init_succeeded_ = false;
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
    int read_bytes_num = 0;
    unsigned int slen = sizeof(sockaddr_in);

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

    if (read_bytes_num == -1) {
      std::cout << "recvfrom() failed." << std::endl;
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

  ~UDPSocket() override { close(client_socket_); }

 private:
  int client_socket_ = 0;
  sockaddr_in server_{};

  bool is_init_succeeded_ = true;

  bool is_connected_{false};
};

}  // namespace paraos

#endif /* PARAOS_SOCKET_UDP_HPP */
