/// @file test_queue.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
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

#include "paraos_queue.hpp"

using namespace paraos;

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

using user_data_type = DataForHeapAlloc;

class QueueCreator : public ::testing::Test {
 public:
  std::unique_ptr<paraos::Queue<user_data_type>> queue_;

 protected:
  virtual void SetUp() {
    queue_ = std::make_unique<paraos::Queue<user_data_type>>(10);
    ASSERT_TRUE(*queue_);
  }

  virtual void TearDown() {}
};

TEST(Queue, Create) {
  Queue<user_data_type> queue{10};

  ASSERT_TRUE(queue);
}

TEST(Queue, CreateEmptyQueue) {
  Queue<user_data_type> queue{0};

  ASSERT_FALSE(queue);
}

TEST_F(QueueCreator, PushThenPop) {
#ifdef paraosTRACE_ENABLE
  std::cout << "-- Push()" << std::endl;
#endif
  ASSERT_TRUE(queue_->Push(user_data_type{10}));

#ifdef paraosTRACE_ENABLE
  std::cout << "-- Pop()" << std::endl;
#endif
  auto read = queue_->Pop();

  ASSERT_TRUE(read);
}

TEST_F(QueueCreator, PushOnly) {
  ASSERT_TRUE(queue_->IsEmpty());
  ASSERT_TRUE(queue_->Push(user_data_type{10}));
  ASSERT_FALSE(queue_->IsEmpty());
}

TEST_F(QueueCreator, PushCopy) {
  Queue<user_data_type> queue{2};
  ASSERT_TRUE(queue_->IsEmpty());
  user_data_type data{10};
  ASSERT_TRUE(queue_->Push(data));
  ASSERT_FALSE(queue_->IsEmpty());
}

TEST(Queue, PushMaxThenPopWhileNotEmpty) {
  constexpr int queue_size{3};
  Queue<int> queue{queue_size};
  constexpr std::array<int, queue_size> arr{1, 2, 3};
  ASSERT_TRUE(queue.IsEmpty());

  ASSERT_TRUE(queue.Push(arr.at(0)));
  ASSERT_FALSE(queue.IsFull());

  ASSERT_TRUE(queue.Push(arr.at(1)));
  ASSERT_FALSE(queue.IsFull());

  ASSERT_TRUE(queue.Push(arr.at(2)));
  ASSERT_TRUE(queue.IsFull());

  ASSERT_FALSE(queue.Push(arr.at(2)));
  ASSERT_TRUE(queue.IsFull());

  {
    auto val = queue.Pop();
    ASSERT_EQ(arr.at(0), val);
    ASSERT_FALSE(queue.IsEmpty());
  }

  {
    auto val = queue.Pop();
    ASSERT_EQ(arr.at(1), val);
    ASSERT_FALSE(queue.IsEmpty());
  }

  {
    auto val = queue.Pop();
    ASSERT_EQ(arr.at(2), val);
    ASSERT_TRUE(queue.IsEmpty());
  }

  {
// Код ниже приведет к неопределенному поведению т.к. данных в очереди уже
// нет.
#if 0
    auto val = queue.Pop();
    ASSERT_EQ(0, val);
#endif
  }
}

TEST_F(QueueCreator, IsFullEmptyBuff) { ASSERT_FALSE(queue_->IsFull()); }

TEST(Queue, FullThenCheck) {
  Queue<user_data_type> queue{2};
  ASSERT_TRUE(queue.EmplaceBack(10));
  ASSERT_TRUE(queue.Push(user_data_type{20}));
  ASSERT_TRUE(queue.IsFull());
  ASSERT_FALSE(queue.Push(user_data_type{30}));
}

TEST(Queue, Erase) {
  Queue<user_data_type> queue{2};
  ASSERT_TRUE(queue.EmplaceBack(10));
  ASSERT_EQ(1, queue.Size());
  ASSERT_TRUE(queue.Push(user_data_type{20}));
  ASSERT_EQ(2, queue.Size());
  ASSERT_TRUE(queue.IsFull());
  ASSERT_FALSE(queue.Push(user_data_type{30}));
  ASSERT_EQ(2, queue.Size());
  queue.Erase();
  ASSERT_FALSE(queue.IsFull());
  ASSERT_TRUE(queue.IsEmpty());
  ASSERT_EQ(0, queue.Size());
}
