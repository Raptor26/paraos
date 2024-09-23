/// @file test_message_buff_with_user_allocator.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
/// @author VyhodcevEgor <vyhodcev@internet.ru>
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

#include <gtest/gtest.h>

#include <iostream>

#include "paraos_attr.h"
#include "paraos_message_buffer.hpp"
#include "paraos_thread.hpp"

constexpr std::size_t thread_delay{0};

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

  // return address of values
  pointer address(reference value) const { return &value; }
  const_pointer address(const_reference value) const { return &value; }

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

template <class T>
class MyAllocQueue {
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
    typedef MyAllocQueue<U> other;
  };

  // return address of values
  pointer address(reference value) const { return &value; }
  const_pointer address(const_reference value) const { return &value; }

  /* constructors and destructor
   * - nothing to do because the allocator has no state
   */
  MyAllocQueue() throw() {}
  MyAllocQueue(const MyAllocQueue &) throw() {}
  template <class U>
  MyAllocQueue(const MyAllocQueue<U> &) throw() {}
  ~MyAllocQueue() throw() {}

  // return maximum number of elements that can be allocated
  size_type max_size() const throw() {
    return std::numeric_limits<std::size_t>::max() / sizeof(T);
  }

  // allocate but don't initialize num elements of type T
  pointer allocate(size_type num, const void * = 0) {
    // print message and allocate memory with global new
    paraosTRACE_MESSAGE(
        "Queue allocator: allocate " << num << " element(s)"
                                     << " of size " << sizeof(T));

    pointer ret = (pointer)(::operator new(num * sizeof(T)));
    paraosTRACE_MESSAGE(" Queue allocator: allocated at: " << (void *)ret);

    return ret;
  }

  // initialize elements of allocated storage p with value value
  void construct(pointer p, const T &value) noexcept {
    // initialize memory with placement new
    paraosTRACE_MESSAGE("Queue allocator: Construct in place");
    new (reinterpret_cast<void *>(p)) T(value);
  }

  template <typename U, typename... Args>
  void construct(U *ptr, Args &&...args) {
    paraosTRACE_MESSAGE(
        "Queue allocator: Construct in place (perfect forward)");
    new (reinterpret_cast<void *>(ptr)) U{std::forward<Args>(args)...};
  }

  // destroy elements of initialized storage p
  template <typename U>
  void destroy(U *ptr) {
    paraosTRACE_MESSAGE(
        "Queue allocator: Destroy objects by calling their destructor");
    ptr->~U();
  }

  // deallocate storage p of deleted elements
  void deallocate(pointer p, size_type num) noexcept {
    PARAOS_ATTR_UNUSED_VAR(num);
    // print message and deallocate memory with global delete
    paraosTRACE_MESSAGE(
        "Queue allocator: deallocate " << num << " element(s)"
                                       << " of size " << sizeof(T)
                                       << " at: " << (void *)p);
    ::operator delete((void *)p);
  }
};

// return that all specializations of this allocator are interchangeable
template <class T1, class T2>
bool operator==(const MyAllocBuffer<T1> &, const MyAllocBuffer<T2> &) throw() {
  return true;
}
template <class T1, class T2>
bool operator!=(const MyAllocBuffer<T1> &, const MyAllocBuffer<T2> &) throw() {
  return false;
}

// return that all specializations of this allocator are interchangeable
template <class T1, class T2>
bool operator==(const MyAllocQueue<T1> &, const MyAllocQueue<T2> &) throw() {
  return true;
}
template <class T1, class T2>
bool operator!=(const MyAllocQueue<T1> &, const MyAllocQueue<T2> &) throw() {
  return false;
}

using buffer_allocator = MyAllocBuffer<std::uint8_t>;

TEST(BufferUserAlloc, Create) {
  paraos::MessageBuffer<10, buffer_allocator> buffer;
  constexpr float val{123.456};

  {
    auto message_area = buffer.Alloc(sizeof(val), thread_delay);
    EXPECT_TRUE(message_area);
    EXPECT_EQ(sizeof(val), message_area.Size());

    auto float_ptr = static_cast<float *>(message_area.Addr());
    *float_ptr = val;

    // деструктор "message_area' автоматически отправит сообщение в буфер.
  }

  {
    auto read = buffer.Pop(thread_delay);
    EXPECT_TRUE(read);

    auto float_ptr = static_cast<float *>(read->Addr());

    EXPECT_NEAR(val, *float_ptr, 0.001);
    EXPECT_EQ(sizeof(val), read->Size());
    // После выхода read из области видимости, деструктор автоматически
    // удалит занимаемые ресурсы.
  }
}
