#include <gtest/gtest.h>

#include <iostream>

#include "paraos_queue.hpp"

template <class T> class MyAlloc {
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
  template <class U> struct rebind {
    typedef MyAlloc<U> other;
  };

  // return address of values
  pointer address(reference value) const { return &value; }
  const_pointer address(const_reference value) const { return &value; }

  /* constructors and destructor
   * - nothing to do because the allocator has no state
   */
  MyAlloc() throw() {}
  MyAlloc(const MyAlloc &) throw() {}
  template <class U> MyAlloc(const MyAlloc<U> &) throw() {}
  ~MyAlloc() throw() {}

  // return maximum number of elements that can be allocated
  size_type max_size() const throw() {
    return std::numeric_limits<std::size_t>::max() / sizeof(T);
  }

  // allocate but don't initialize num elements of type T
  pointer allocate(size_type num, const void * = 0) {
    // print message and allocate memory with global new
    paraosTRACE_MESSAGE("allocate " << num << " element(s)"
                                    << " of size " << sizeof(T));

    pointer ret = (pointer)(::operator new(num * sizeof(T)));
    paraosTRACE_MESSAGE(" allocated at: " << (void *)ret);

    return ret;
  }

  // initialize elements of allocated storage p with value value
  void construct(pointer p, const T &value) noexcept {
    // initialize memory with placement new
    paraosTRACE_MESSAGE("Construct in place");
    new (reinterpret_cast<void *>(p)) T(value);
  }

  template <typename U, typename... Args>
  void construct(U *ptr, Args &&...args) {
    paraosTRACE_MESSAGE("Construct in place (perfect forward)");
    new (reinterpret_cast<void *>(ptr)) U{std::forward<Args>(args)...};
  }

  // destroy elements of initialized storage p
  template <typename U> void destroy(U *ptr) {
    paraosTRACE_MESSAGE("Destroy objects by calling their destructor");
    ptr->~U();
  }

  // deallocate storage p of deleted elements
  void deallocate(pointer p, size_type num) noexcept {
    // print message and deallocate memory with global delete
    paraosTRACE_MESSAGE("deallocate " << num << " element(s)"
                                      << " of size " << sizeof(T)
                                      << " at: " << (void *)p);
    ::operator delete((void *)p);
  }
};

// return that all specializations of this allocator are interchangeable
template <class T1, class T2>
bool operator==(const MyAlloc<T1> &, const MyAlloc<T2> &) throw() {
  return true;
}
template <class T1, class T2>
bool operator!=(const MyAlloc<T1> &, const MyAlloc<T2> &) throw() {
  return false;
}

struct DataForHeapAlloc final {
  explicit DataForHeapAlloc(int val) {
    mem_ = new int;
    *mem_ = val;
  }

  DataForHeapAlloc() {
    mem_ = new int;
    *mem_ = 0;
  }

  DataForHeapAlloc(const DataForHeapAlloc &other) {
    mem_ = new int;
    *mem_ = *other.mem_;
  }

  DataForHeapAlloc(DataForHeapAlloc &&other) noexcept {
    mem_ = other.mem_;

    other.mem_ = nullptr;
  }

  DataForHeapAlloc &operator=(const DataForHeapAlloc &other) = delete;
  DataForHeapAlloc &operator=(DataForHeapAlloc &&other) {
    delete mem_;

    mem_ = other.mem_;

    return *this;
  }

  operator bool() const {
    bool is_valid{false};

    if (mem_) {
      is_valid = true;
    }

    return is_valid;
  }

  ~DataForHeapAlloc() { delete mem_; }

private:
  int *mem_;
};

TEST(QueueUserAlloc, Create) {
  paraos::Queue<DataForHeapAlloc, MyAlloc<DataForHeapAlloc>> queue{5};
  queue.EmplaceBack(10);
  queue.Push(DataForHeapAlloc{20});
}