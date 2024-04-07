#ifndef CRITICAL_HPP
#define CRITICAL_HPP

#include <Windows.h>
#include <assert.h>

#include <iostream>

namespace paraos {

class CriticalSectionFactory final {
 public:
  CriticalSectionFactory() noexcept {
    std::cout << "CriticalSectionFactory Ctor" << std::endl;
    if (!InitializeCriticalSectionAndSpinCount(&critical_section_, 0x00000400))
      assert(true == false);
  }

  ~CriticalSectionFactory() {
    std::cout << "CriticalSectionFactory Dtor" << std::endl;
    DeleteCriticalSection(&critical_section_);
  }

  LPCRITICAL_SECTION GiveHandle() { return &critical_section_; }

 private:
  CRITICAL_SECTION critical_section_;
};

class CriticalSection final {
 public:
  /// @brief Конструктор обеспечивает автоматический вход в критическую секцию.
  /// @param is_isr
  CriticalSection(bool is_isr = false) : is_isr_{is_isr} {
    EnterCriticalSection(critical_section_factory.GiveHandle());
  }

  /// @brief Деструктор обеспечивает автоматический выход из критической секции.
  ~CriticalSection() {
    LeaveCriticalSection(critical_section_factory.GiveHandle());
  }

 private:
  const bool is_isr_;
  static inline CriticalSectionFactory critical_section_factory;
};

}  // namespace paraos

#endif /* CRITICAL_HPP */
