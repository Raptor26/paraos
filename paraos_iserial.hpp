#ifndef PARAOS_ISERIAL_HPP
#define PARAOS_ISERIAL_HPP

#include <cstdlib>
namespace paraos {

/// @brief Интерфейс, описывающий механизм получения и
/// передачи данных.
class ISerial {
 public:
  /// @brief Метод выполняет запись заданного количества полученных байтов в
  /// указанную область памяти.
  /// @param[in] dst: Указатель на область памяти, в которую необходимо записать
  /// полученные данные.
  /// @param[in] dst_size: Количество байтов, которое необходимо считать и
  /// записать.
  /// @return Возвращает количество полученных байтов.
  virtual auto Receive(void *dst, size_t dst_size) -> size_t = 0;

  /// @brief Метод выполняет передачу заданного количества байтов из указанной
  /// области памяти.
  /// @param[in] src: Указатель на область памяти, данные из которой необходимо
  /// отправить.
  /// @param[in] msg_size: Количество байтов, которое необходимо передать.
  /// @return Возвращает количество переданных байтов.
  virtual auto Transmit(void *src, size_t msg_size) -> size_t = 0;

  virtual ~ISerial() = default;

 protected:
  /// @brief Disable direct creation of interface class by declaring ctor as
  /// protected.
  ISerial() = default;
};

}  // namespace paraos

#endif /* PARAOS_ISERIAL_HPP */
