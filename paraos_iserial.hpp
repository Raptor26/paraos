#ifndef PARAOS_ISERIAL_HPP
#define PARAOS_ISERIAL_HPP

#include <cstdint>
#include <cstdlib>

namespace paraos {

class ISerialTx {
 public:
  /// @brief Повторная инициализация порта ввода/вывода с заданной скоростью
  /// работы.
  ///
  /// @param[in] baudrate: Требуемая скорость работы порта ввода/вывода.
  ///
  /// @return true в случае успешной инициализации, false в противном случае.
  virtual auto Reinit() -> bool { return false; }

  /// @brief Метод выполняет передачу заданного количества байтов из указанной
  /// области памяти.
  ///
  /// @param[in] src: Указатель на область памяти, данные из которой необходимо
  /// отправить.
  /// @param[in] msg_size: Количество байтов, которое необходимо передать.
  ///
  /// @return Возвращает количество переданных байтов.
  virtual auto Transmit(const void *src, std::size_t msg_size)
      -> std::size_t = 0;

  /// @brief Five rule.
  ISerialTx(ISerialTx &&other) = delete;
  auto operator=(ISerialTx &&other) -> ISerialTx & = delete;
  auto operator=(const ISerialTx &other) -> ISerialTx & = delete;
  ISerialTx(const ISerialTx &other) = delete;

 protected:
  /// @brief Disable direct creation of interface class by declaring ctor as
  /// protected.
  ISerialTx() = default;
};

class ISerialRx {
 public:
  /// @brief Five rule.
  ISerialRx(ISerialRx &&other) = delete;
  auto operator=(ISerialRx &&other) -> ISerialRx & = delete;
  auto operator=(const ISerialRx &other) -> ISerialRx & = delete;
  ISerialRx(const ISerialRx &other) = delete;

 protected:
  /// @brief Disable direct creation of interface class by declaring ctor as
  /// protected.
  ISerialRx() = default;
};

/// @brief Интерфейс, описывающий механизм получения и
/// передачи данных.
class ISerial : public ISerialTx {
 public:
  /// @brief Метод выполняет запись заданного количества полученных байтов в
  /// указанную область памяти.
  ///
  /// @param[in] dst: Указатель на область памяти, в которую необходимо записать
  /// полученные данные.
  /// @param[in] dst_size: Количество байтов, которое необходимо считать и
  /// записать.
  ///
  /// @return Возвращает количество полученных байтов.
  virtual auto Receive(void *dst, std::size_t dst_size) -> std::size_t = 0;

  virtual ~ISerial() = default;

  /// @brief Five rule.
  ISerial(ISerial &&other) = delete;
  auto operator=(ISerial &&other) -> ISerial & = delete;
  auto operator=(const ISerial &other) -> ISerial & = delete;
  ISerial(const ISerial &other) = delete;

 protected:
  /// @brief Disable direct creation of interface class by declaring ctor as
  /// protected.
  ISerial() = default;
};

}  // namespace paraos

#endif /* PARAOS_ISERIAL_HPP */
