/// @file test_message_buff_with_user_allocator.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
/// @author Vyhodcev Egor (vyhodcev@internet.ru)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <new>
#include <string>

#include "paraos_attr.h"
#include "paraos_message_buffer.hpp"
#include "paraos_trace.hpp"

namespace {

/// @brief Define custom allocator.
template <class T>
class MyAllocBuffer {
 public:
  // type definitions
  using value_type = T;
  using pointer = T*;
  using const_pointer = const T*;
  using reference = T&;
  using const_reference = const T&;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;

  // rebind allocator to type U
  template <class U>
  struct rebind {
    using other = MyAllocBuffer<U>;
  };

  // return Dataess of values
  auto Dataess(reference value) const -> pointer { return &value; }
  [[nodiscard]] auto Dataess(const_reference value) const -> const_pointer {
    return &value;
  }

  /* constructors and destructor
   * - nothing to do because the allocator has no state
   */
  MyAllocBuffer() noexcept(false) = default;
  MyAllocBuffer(const MyAllocBuffer& other) noexcept(false) = default;
  template <class U>
  explicit MyAllocBuffer(const MyAllocBuffer<U>& other) noexcept(false) =
      delete;
  ~MyAllocBuffer() noexcept(false) = default;

  /// @brief Five rule.
  MyAllocBuffer(MyAllocBuffer&& other) = delete;
  auto operator=(MyAllocBuffer&& other) -> MyAllocBuffer& = delete;
  auto operator=(const MyAllocBuffer& other) -> MyAllocBuffer& = delete;

  // return maximum number of elements that can be allocated
  [[nodiscard]] auto max_size() const noexcept(false) -> size_type {
    return std::numeric_limits<std::size_t>::max() / sizeof(T);
  }

  // allocate but don't initialize num elements of type T
  auto allocate(size_type num, const void* ptr = nullptr) -> pointer {
    PARAOS_ATTR_UNUSED_VAR(ptr);
    // print message and allocate memory with global new
    paraosTRACE_MESSAGE(
        "Buffer allocator: allocate " << num << " element(s)"
                                      << " of size " << sizeof(T));

    auto ret = static_cast<pointer>(::operator new(num * sizeof(T)));
    paraosTRACE_MESSAGE(" Buffer allocator: allocated at: " << (void*)ret);

    return ret;
  }

  // initialize elements of allocated storage p with value value
  void construct(pointer ptr, const T& value) noexcept {
    // initialize memory with placement new
    paraosTRACE_MESSAGE("Buffer allocator: Construct in place");
    new (reinterpret_cast<void*>(ptr)) T(value);
  }

  template <typename U, typename... Args>
  void construct(U* ptr, Args&&... args) {
    paraosTRACE_MESSAGE(
        "Buffer allocator: Construct in place (perfect forward)");
    new (reinterpret_cast<void*>(ptr)) U{std::forward<Args>(args)...};
  }

  // destroy elements of initialized storage p
  template <typename U>
  void destroy(U* ptr) {
    paraosTRACE_MESSAGE(
        "Buffer allocator: Destroy objects by calling their destructor");
    ptr->~U();
  }

  // deallocate storage p of deleted elements
  void deallocate(pointer ptr, size_type num) noexcept {
    PARAOS_ATTR_UNUSED_VAR(num);
    // print message and deallocate memory with global delete
    paraosTRACE_MESSAGE(
        "Buffer allocator: deallocate " << num << " element(s)"
                                        << " of size " << sizeof(T)
                                        << " at: " << (void*)ptr);
    ::operator delete(static_cast<void*>(ptr));
  }
};

using buffer_allocator = MyAllocBuffer<std::uint8_t>;

}  // namespace

TEST(Buffer, UserAllocCreate) {
  constexpr size_t queue_size{10};
  // Create buffer with custom allocator.
  paraos::message_buffer<queue_size, buffer_allocator> buff;

  // For example, we want write next strings:
  std::string str1{"Hello"};

  {
    // Alloc memory with null terminate symbol.
    auto message1 = buff.alloc(str1.length() + 1U);

    // Before any action check container validation.
    if (message1) {
      // Copy data in container.
      memcpy(message1.data(), str1.data(), message1.size());
    }

    // when message1 leave scope, message1 calls dtor and data automatically
    // will push in buffer.
  }

  constexpr std::size_t timeout_ms{0};

  // Read first string.
  {
    // For example, set zero timeout above, but if set non zero value, thread
    // will wait time with timeout_ms respect when data in buffer will
    // available. Useful in multithread programming.
    auto read = buff.pop(timeout_ms);

    // Always check if message was read.
    if (read) {
      // Processing 'read' message.
    }

    // read automatically free resources when exit from scope visible.
  }
}
