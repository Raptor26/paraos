#include <gtest/gtest.h>

#include <array>
#include <memory>
#include <set>
#include <string>

#include "paraos_message_buffer.hpp"

using namespace paraos;

#if 1
using QueueRealization = paraos::QueueMessageBuffWrapper;

class MessageBufferCreate : public ::testing::Test {
 public:
  std::unique_ptr<MessageBuffer<QueueRealization>> buffer_;

 protected:
  virtual void SetUp() {
    buffer_ = std::make_unique<MessageBuffer<QueueRealization>>();
  }

  virtual void TearDown() {}
};

#if 1
TEST(MessageBuff, Create) { MessageBuffer buff; }

TEST_F(MessageBufferCreate, AllocZeroMemory) {
  auto message = buffer_->Alloc(0);
  EXPECT_FALSE(message);
}

TEST_F(MessageBufferCreate, PopFromEmptyBuff) {
  auto message = buffer_->Pop();
  EXPECT_FALSE(message);
}
#endif

TEST_F(MessageBufferCreate, PushThenPop) {
  constexpr float val{0.1234};

  {
    auto message_area = buffer_->Alloc(sizeof(val));
    EXPECT_TRUE(message_area);
    EXPECT_EQ(sizeof(val), message_area.GetSize());

    auto float_ptr = static_cast<float *>(message_area.GetAddr());
    *float_ptr = val;

    // деструктор "message_area' автоматически отправит сообщение в буфер.
  }

  {
    auto read = buffer_->Pop();
    EXPECT_TRUE(read);

    auto float_ptr = static_cast<float *>(read.GetAddr());

    EXPECT_NEAR(val, *float_ptr, 0.001);
    EXPECT_EQ(sizeof(val), read.GetSize());
    // После выхода read из области видимости, деструктор автоматически удалит
    // занимаемые ресурсы.
  }
}

TEST_F(MessageBufferCreate, PushManyMessagesThenPop) {
  constexpr double start_value{10};
  constexpr size_t messages_numb{10};

  // Операции записи данных
  size_t written_elem_numb{0};
  for (size_t i = 0; i < messages_numb; ++i) {
    auto writable = buffer_->Alloc(sizeof(start_value));
    if (writable) {
      auto float_ptr = static_cast<double *>(writable.GetAddr());
      *float_ptr = start_value + static_cast<double>(i);
      ++written_elem_numb;
    }

    // Сообщение writable будет отправлено в деструкторе автоматически
  }

  size_t i{0};
  EXPECT_EQ(false, buffer_->IsEmpty());
  while (!buffer_->IsEmpty()) {
    auto readable = buffer_->Pop();
    auto float_ptr = static_cast<double *>(readable.GetAddr());

    EXPECT_NEAR(start_value + static_cast<double>(i), *float_ptr, 0.001);
    ++i;

    EXPECT_EQ(written_elem_numb - i, buffer_->Size());
  }

  EXPECT_EQ(messages_numb, i);
}

TEST_F(MessageBufferCreate, EraseEmptyBuff) {
  EXPECT_EQ(true, buffer_->IsEmpty());
  buffer_->Erase();
  EXPECT_EQ(true, buffer_->IsEmpty());
}

TEST_F(MessageBufferCreate, EraseFillBuff) {
  EXPECT_EQ(true, buffer_->IsEmpty());

  constexpr double start_value{10};
  constexpr size_t messages_numb{5};

  // Операции записи данных
  for (size_t i = 0; i < messages_numb; ++i) {
    auto writable = buffer_->Alloc(sizeof(start_value));
    if (writable) {
      auto float_ptr = static_cast<double *>(writable.GetAddr());
      *float_ptr = start_value + static_cast<double>(i);
    }
  }
  EXPECT_EQ(false, buffer_->IsEmpty());

  buffer_->Erase();
  EXPECT_EQ(true, buffer_->IsEmpty());
}

TEST_F(MessageBufferCreate, WriteStrings) {
  constexpr size_t str_array_size{5};

  // Количество строк в массиве без учета пустых строк
  constexpr size_t usefull_str_array_size{str_array_size - 1};

  std::array<std::string, str_array_size> set_str = {
      "Hello", "World", "Update me please", "Now!!!"};

  size_t write_cnt{0};
  // Запись строк
  for (const auto &str : set_str) {
    // Фигурные скобки ниже необходимы в рамках теста для дополнительного
    // ограничения области видимости переменной 'write' что позволит в цикле
    // 'for (const auto &str : set_str)' проверять количество записанных
    // сообщений в 'buffer_' (при вызове деструктора переменной `write`).
    {
      auto write = buffer_->Alloc(str.length());
      if (write) {
        memcpy(write.GetAddr(), static_cast<const void *>(str.c_str()),
               write.GetSize());
        ++write_cnt;
      }

      // Запись сообщения в буфер будет выполнена в деструкторе 'write'
    }

    EXPECT_EQ(buffer_->Size(), write_cnt);
  }

  size_t read_cnt{0};
  // Чтение строк
  while (!buffer_->IsEmpty()) {
    auto read = buffer_->Pop();

    if (read) {
      std::string_view str_view{static_cast<char *>(read.GetAddr()),
                                read.GetSize()};

      EXPECT_EQ(set_str[read_cnt], str_view);
      ++read_cnt;
    }
  }

  EXPECT_EQ(usefull_str_array_size, read_cnt);
}

TEST_F(MessageBufferCreate, AllocThenUserPop) {
  std::string str = {"Hello World"};
  {
    auto write = buffer_->Alloc(str.length());

    if (write) {
      memcpy(write.GetAddr(), static_cast<const void *>(str.c_str()),
             write.GetSize());

      // Пользователь передумал записывать сообщение.
      write.Pop();
    }

    // В деструкторе переменной 'write' сообщение не будет записано в 'buffer_'
    // т.к. пользователь вызвал Pop()
  }

  EXPECT_TRUE(buffer_->IsEmpty());
}

TEST_F(MessageBufferCreate, AllocThenUserPopIfEmptyMessage) {
  auto write = buffer_->Alloc(0);

  // Пользователь передумал записывать сообщение, под которое не выделена
  // память.
  write.Pop();

  EXPECT_TRUE(buffer_->IsEmpty());
}

#endif