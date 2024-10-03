# Paraos containers

Данная директория содержит реализации различных контейнеров данных, к примеру, кольцевого буфера, очереди и т.п.

## Использование

### Paraos ring buffer

Код из примера расположен в `./tests/test_lwrb.cpp`.

#### Запись и чтение данных

```c++
#include "paraos_ringbuff.hpp"

constexpr std::size_t ringbuff_size_in_bytes{100};
// Инициализация ring buffer.
paraos::RingBuff<char, ringbuff_size_in_bytes> ring_buff;

// После инициализации количество свободного места 
// на 1 байт меньше размера буфера. Метод Free() возвращает 
// количество свободного для записи места.
ASSERT_EQ(ringbuff_size_in_bytes - 1, ring_buff.Free());

std::string src{"Hello World!"};
// Запись данных в буфер.
auto written_bytes_numb =
    ring_buff.Write(static_cast<const void *>(src.c_str()), src.size());
ASSERT_EQ(src.size(), written_bytes_numb);
// Метод ring_buff.Size() возвращает количество байтов, 
// находящихся в буфере.
ASSERT_EQ(src.size(), ring_buff.Size());

std::array<char, 100> dst_;
// Чтение данных из буфера.
auto read_bytes_numb = ring_buff.Read(dst_.data(), dst_.size());
ASSERT_EQ(written_bytes_numb, read_bytes_numb);

ASSERT_EQ(
    0, memcmp(
            static_cast<const void *>(src.data()),
            static_cast<const void *>(dst_.data()), src.size()));

// После считывания данных из буфера количество свободного места 
// вновь становится на 1 байт меньше размера буфера.
ASSERT_EQ(ringbuff_size_in_bytes - 1, ring_buff.Free());
```
_Полный код можно посмотреть в тесте `TEST(RingBuff, WriteThenRead)`._

Также Ring buffer поддерживает чтение/запись данных с использованием `итераторов` и `span`.  
_Примеры можно посмотреть в тестах `TEST(RingBuff, WriteThenReadIterator)` и `TEST(RingBuff, WriteSpanThenReadSpan)` соответственно._

#### Пропуск данных в буфере 

Метод `Skip` используется для перемещения указателя чтения данных на указанное количество байтов вперёд. Пройденные байты отмечаются как прочитанные.  
Пример использования метода представлен ниже:
```c++
std::string src{"Hello World!"};
auto written_bytes_numb =
      ring_buff.Write(static_cast<const void *>(src.data()), src.size());
ASSERT_EQ(written_bytes_numb, ring_buff.Size());

constexpr std::size_t skip_need{10};
auto skip_bytes = ring_buff.Skip(skip_need);

ASSERT_EQ(skip_need, skip_bytes);
ASSERT_EQ(written_bytes_numb - skip_need, ring_buff.Size());
```
_Полный код можно посмотреть в тесте `TEST(RingBuff, Skip)`._

#### Очистка кольцевого буфера

Для очистки буфера применяется метод `Clear()`:
```c++
std::string src{"Hello World!"};
auto written_bytes_numb =
    ring_buff.Write(static_cast<const void *>(src.data()), src.size());
ASSERT_EQ(written_bytes_numb, ring_buff.Size());

ring_buff.Clear();
ASSERT_EQ(0u, ring_buff.Size());
```
_Полный код можно посмотреть в тесте `TEST(RingBuff, Clear)`._
<hr>

### Paraos multiple ring buffer

`Multiple ring buffer` представляет собой структуру, которая позволяет помещать данные в несколько независимых кольцевых буферов. Для определения в какой буфер были помещены данные используется управляющая очередь.

Код из примера расположен в `./tests/test_multi_ringbuff.cpp`.

#### Запись и чтение данных

```c++
#include "paraos_multi_ringbuff.hpp"

constexpr std::size_t queue_size{10};
constexpr std::size_t max_ring_buff_size{128};
constexpr std::size_t max_ring_buff_numb{5};
// Инициализация multi ring buffer.
paraos::MultiRingBuff<queue_size, max_ring_buff_size, max_ring_buff_numb>
    multi_ring_buff{};

std::string str{"Hello world!"};
constexpr std::size_t cbuff_id{0};
// Запись данных в multi ring buffer.
auto written_bytes_numb = multi_ring_buff.Write(
    cbuff_id, static_cast<const void *>(str.data()), str.size(), 0u);
ASSERT_EQ(str.size(), written_bytes_numb);

std::array<std::uint8_t, 128> dst_arr{};
std::size_t buff_id;
// Чтение данных из Multi ring buffer.
auto read_bytes_numb = multi_ring_buff.Read(
    buff_id, static_cast<void *>(dst_arr.data()), dst_arr.size(), 0u);
ASSERT_EQ(written_bytes_numb, read_bytes_numb);
ASSERT_EQ(cbuff_id, buff_id);
```

Также Multi ring buffer поддерживает запись/чтение данных с использованием `span`. _Пример можно посмотреть в тесте `TEST(MultiRingBuff, WriteSpanToRingBufferThenRead)`._
<hr>
