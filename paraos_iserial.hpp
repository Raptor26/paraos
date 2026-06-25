/// @file paraos_iserial.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
#ifndef PARAOS_ISERIAL_HPP
#define PARAOS_ISERIAL_HPP

#include <cstdint>
#include <cstdlib>

#include "paraos_config.hpp"

namespace paraos {

class serial_tx_base {
 public:
  virtual ~serial_tx_base() = default;

  /// @brief Повторная инициализация порта ввода/вывода с заданной скоростью
  /// работы.
  ///
  /// @param[in] baudrate: Требуемая скорость работы порта ввода/вывода.
  ///
  /// @return true в случае успешной инициализации, false в противном
  /// случае.
  virtual auto reinit() -> bool { return false; }

  /// @brief Метод выполняет передачу заданного количества байтов из указанной
  /// области памяти.
  ///
  /// @param[in] src: Указатель на область памяти, данные из которой необходимо
  /// отправить.
  /// @param[in] msg_size: Количество байтов, которое необходимо передать.
  ///
  /// @return Возвращает количество переданных байтов.
  virtual auto transmit(const void *src, std::size_t msg_size)
      -> std::size_t = 0;

  /// @brief Five rule.
  serial_tx_base(serial_tx_base &&other) = delete;
  auto operator=(serial_tx_base &&other) -> serial_tx_base & = delete;
  auto operator=(const serial_tx_base &other) -> serial_tx_base & = delete;
  serial_tx_base(const serial_tx_base &other) = delete;

 protected:
  /// @brief Disable direct creation of interface class by declaring ctor as
  /// protected.
  serial_tx_base() = default;
};

using ISerialTx PARAOS_DEPRECATED("use paraos::serial_tx_base") = serial_tx_base;

class serial_rx_base {
 public:
  virtual ~serial_rx_base() = default;

  /// @brief Five rule.
  serial_rx_base(serial_rx_base &&other) = delete;
  auto operator=(serial_rx_base &&other) -> serial_rx_base & = delete;
  auto operator=(const serial_rx_base &other) -> serial_rx_base & = delete;
  serial_rx_base(const serial_rx_base &other) = delete;

 protected:
  /// @brief Disable direct creation of interface class by declaring ctor as
  /// protected.
  serial_rx_base() = default;
};

using ISerialRx PARAOS_DEPRECATED("use paraos::serial_rx_base") = serial_rx_base;

/// @brief Интерфейс, описывающий механизм получения и
/// передачи данных.
class serial_base : public serial_tx_base {
 public:
  ~serial_base() override = default;

  /// @brief Метод выполняет запись заданного количества полученных байтов в
  /// указанную область памяти.
  ///
  /// @param[in] dst: Указатель на область памяти, в которую необходимо записать
  /// полученные данные.
  /// @param[in] dst_size: Количество байтов, которое необходимо считать и
  /// записать.
  ///
  /// @return Возвращает количество полученных байтов.
  virtual auto receive(void *dst, std::size_t dst_size) -> std::size_t = 0;

  /// @brief Five rule.
  serial_base(serial_base &&other) = delete;
  auto operator=(serial_base &&other) -> serial_base & = delete;
  auto operator=(const serial_base &other) -> serial_base & = delete;
  serial_base(const serial_base &other) = delete;

 protected:
  /// @brief Disable direct creation of interface class by declaring ctor as
  /// protected.
  serial_base() = default;
};

using ISerial PARAOS_DEPRECATED("use paraos::serial_base") = serial_base;

}  // namespace paraos

#endif /* PARAOS_ISERIAL_HPP */
