Table of context:
<!-- TOC -->

- [Descriptions](#descriptions)
- [Examples](#examples)
    - [Default std::allocator memory allocator *for more users*](#default-stdallocator-memory-allocator-for-more-users)
    - [Custom memory allocator](#custom-memory-allocator)

<!-- /TOC -->

# Descriptions

Message buffer for multithread communications. Support:
- single producer sing consumer (spsc)
- single producer multi consumer (spmc)
- multi producer single consumer (mpsc) 
- multi producer multi consumer (mpmp) 

Read operations may be blocking, but write operations always non blocking.

By default, buffer use default `std::allocator` for request memory from heap. But user can use custom allocator. For example usage customs allocators see `test_message_buff_with_user_allocator.cpp`

# Examples

## Default `std::allocator` memory allocator (*for more users*)

```cpp
@test_message_buff.cpp

#include <gtest/gtest.h>
#include <string>
#include "paraos_message_buffer.hpp"

TEST(Message, Example) {
  // Max message numb for contained in buffer in same time.
  constexpr std::size_t buffer_size{2};

  // Create buffer
  paraos::MessageBuffer<buffer_size> buff;

  // For example, we want write next strings:
  std::string str1{"Hello"};
  std::string str2{" world!"};

  {
    // Alloc memory with null terminate symbol.
    auto message1 = buff.Alloc(str1.length() + 1u);

    // Before any action check container validation.
    if (message1) {
      // Copy data in container.
      memcpy(message1.Data(), str1.data(), message1.Size());
    }

    // when message1 leave scope, message1 calls dtor and data automatically will
    // push in buffer.
    // Be careful: If other thread full queue buffer between buff.Alloc() and
    // message leave scope, message1 will not pushed in buffer. message1
    // will miss, but always resources will correctly free (no leak memory).
  }

  {
    // Alloc memory with null terminate symbol.
    auto message2 = buff.Alloc(str2.length() + 1u);

    // Before any action check container validation.
    if (message2) {
      // Copy data in container.
      memcpy(message2.Data(), str2.data(), message2.Size());

      // Force push message in buffer.
      // Be careful: If other thread full queue buffer between buff.Alloc() and
      // TryPush(), message1 will not pushed in buffer. message2
      // will miss, but always resources will correctly free (no leak memory).
      auto is_message_pushed = message2.TryPush();

      if (!is_message_pushed) {
        // No space in queue. Message don't pushed in buffer.
      }
    }

    // when message1 leave scope, message1 calls dtor. Because user call
    // TryPush() above, message2 already in buffer and dtor don't push this
    // message again.
  }

  // Let's try read data form buffer.

  constexpr std::size_t timeout_ms{0};

  // Read first string.
  {
    // For example, set zero timeout above, but if set non zero value, thread
    // will wait time with timeout_ms respect when data in buffer will
    // available. Useful in multithread programming.
    auto read = buff.Pop(timeout_ms);

    // Always check if message was read.
    if (read) {
      // Print first string.
      std::cout << static_cast<char *>(read->Data());
    }

    // read automatically free resources when exit from scope visible.
  }

  // Read second string.
  {
    auto read = buff.Pop(timeout_ms);

    // Always check if message was read.
    if (read) {
      // Print first string.
      std::cout << static_cast<char *>(read->Data()) << std::endl;
    }

    // read automatically free resources when exit from scope visible.
  }
}

```

## Custom memory allocator

```cpp
@ test_message_buff_with_user_allocator.cpp

#include <gtest/gtest.h>
#include <iostream>
#include "paraos_attr.h"
#include "paraos_message_buffer.hpp"

/// @brief Define custom allocator.
template <class T>
class MyAllocBuffer {
 public:
  // type definitions
  typedef T value_type;
  typedef T *pointer;
  typedef const T *const_pointer;
  typedef T &reference;
  typedef const T &const_reference;
  typedef std::size_t size_type;
  typedef std::ptrdiff_t difference_type;

  // rebind allocator to type U
  template <class U>
  struct rebind {
    typedef MyAllocBuffer<U> other;
  };

  // return Dataess of values
  pointer Dataess(reference value) const { return &value; }
  const_pointer Dataess(const_reference value) const { return &value; }

  /* constructors and destructor
   * - nothing to do because the allocator has no state
   */
  MyAllocBuffer() throw() {}
  MyAllocBuffer(const MyAllocBuffer &) throw() {}
  template <class U>
  MyAllocBuffer(const MyAllocBuffer<U> &) throw() {}
  ~MyAllocBuffer() throw() {}

  // return maximum number of elements that can be allocated
  size_type max_size() const throw() {
    return std::numeric_limits<std::size_t>::max() / sizeof(T);
  }

  // allocate but don't initialize num elements of type T
  pointer allocate(size_type num, const void * = 0) {
    // print message and allocate memory with global new
    paraosTRACE_MESSAGE(
        "Buffer allocator: allocate " << num << " element(s)"
                                      << " of size " << sizeof(T));

    pointer ret = (pointer)(::operator new(num * sizeof(T)));
    paraosTRACE_MESSAGE(" Buffer allocator: allocated at: " << (void *)ret);

    return ret;
  }

  // initialize elements of allocated storage p with value value
  void construct(pointer p, const T &value) noexcept {
    // initialize memory with placement new
    paraosTRACE_MESSAGE("Buffer allocator: Construct in place");
    new (reinterpret_cast<void *>(p)) T(value);
  }

  template <typename U, typename... Args>
  void construct(U *ptr, Args &&...args) {
    paraosTRACE_MESSAGE(
        "Buffer allocator: Construct in place (perfect forward)");
    new (reinterpret_cast<void *>(ptr)) U{std::forward<Args>(args)...};
  }

  // destroy elements of initialized storage p
  template <typename U>
  void destroy(U *ptr) {
    paraosTRACE_MESSAGE(
        "Buffer allocator: Destroy objects by calling their destructor");
    ptr->~U();
  }

  // deallocate storage p of deleted elements
  void deallocate(pointer p, size_type num) noexcept {
    PARAOS_ATTR_UNUSED_VAR(num);
    // print message and deallocate memory with global delete
    paraosTRACE_MESSAGE(
        "Buffer allocator: deallocate " << num << " element(s)"
                                        << " of size " << sizeof(T)
                                        << " at: " << (void *)p);
    ::operator delete((void *)p);
  }
};

using buffer_allocator = MyAllocBuffer<std::uint8_t>;

TEST(Buffer, UserAllocCreate) {
  // Create buffer with custom allocator.
  paraos::MessageBuffer<10, buffer_allocator> buff;

  // For example, we want write next strings:
  std::string str1{"Hello"};

  {
    // Alloc memory with null terminate symbol.
    auto message1 = buff.Alloc(str1.length() + 1u);

    // Before any action check container validation.
    if (message1) {
      // Copy data in container.
      memcpy(message1.Data(), str1.data(), message1.Size());
    }

    // when message1 leave scope, message1 calls dtor and data automatically will
    // push in buffer.
  }

  constexpr std::size_t timeout_ms{0};

  // Read first string.
  {
    // For example, set zero timeout above, but if set non zero value, thread
    // will wait time with timeout_ms respect when data in buffer will
    // available. Useful in multithread programming.
    auto read = buff.Pop(timeout_ms);

    // Always check if message was read.
    if (read) {
      // Processing 'read' message.
    }

    // read automatically free resources when exit from scope visible.
  }
}

```