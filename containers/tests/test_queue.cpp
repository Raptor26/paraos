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
