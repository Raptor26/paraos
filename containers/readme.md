# Paraos containers

Данная директория содержит реализации различных контейнеров данных, к примеру, кольцевого буфера, очереди и т.п.

## Использование

### Paraos ring buffer

Код из примера расположен в `./tests/test_lwrb.cpp`.

Перед работой с кольцевым буфером необходимо выполнить include:

```c++
#include "paraos_ringbuff.hpp"
```

#### Инициализация буфера

_Полный код можно посмотреть в тесте `TEST(RingBuff, WriteThenRead)`._

Согласно шаблону `template <typename T, std::size_t SIZE>` при создании объекта буфера необходимо в качестве шаблонных параметров передать ему `тип данных для хранения` и `размер буфера`.

Инициализация буфера может выглядеть следующим образом:
```c++
constexpr std::size_t ringbuff_size_in_bytes{100};
paraos::RingBuff<char, ringbuff_size_in_bytes> ring_buff;
```

#### Помещение данных в кольцевой буфер

Для записи данных в буфер используется метод `Write`. Ниже представлены доступные перегрузки данного метода и примеры их использования.

- Запись через указатель на буфер-источник:  
`Write(const void* src, lwrb_sz_t size_in_bytes)`
    ```c++
    std::string src{"Hello World!"};
    auto written_bytes_numb =
        ring_buff.Write(static_cast<const void *>(src.c_str()), src.size());
    ```
    _Полный код можно посмотреть в тесте `TEST(RingBuff, WriteThenRead)`._

- Запись с использованием `span`:  
`Write(const gsl::span<T> src)`
    ```c++
    std::string src{"Hello World!"};
    auto written_bytes_numb = ring_buff.Write(src);
    ```
    _Полный код можно посмотреть в тесте `TEST(RingBuff, WriteSpanThenReadSpan)`._

- Запись с помощью итераторов:  
    ```c++
    template <class TIterator>
    auto Write(TIterator begin, TIterator end)
    ```

    ```c++
    constexpr std::array<char, ringbuff_size_in_bytes> src{"Hello World!"};
    const auto str_len_without_null = strlen(src.data());
    auto written_bytes_numb =
        ring_buff.Write(src.begin(), src.begin() + str_len_without_null);
    ```
    _Полный код можно посмотреть в тесте `TEST(RingBuff, WriteThenReadIterator)`._

#### Чтение данных из кольцевого буфера

Для получения данных из буфера используется метод `Read`. Ниже представлены доступные перегрузки данного метода и примеры их использования.

- Чтение через указатель на буфер:  
`Read(void* dst, lwrb_sz_t dst_size_in_bytes)`
    ```c++
    std::array<char, 100> dst_;
    auto read_bytes_numb = ring_buff.Read(dst_.data(), dst_.size());
    ```
    _Полный код можно посмотреть в тесте `TEST(RingBuff, WriteThenRead)`._

- Чтение через `span`:  
`Read(gsl::span<T> dst)`
    ```c++
    std::array<char, 100> dst_;
    auto read_bytes_numb = ring_buff.Read(dst_);
    ```
    _Полный код можно посмотреть в тесте `TEST(RingBuff, WriteSpanThenReadSpan)`._

- Чтение с помощью итераторов:  
`Read(iterator begin, iterator end)`
    ```c++
    std::array<char, 100> dst_;
    auto read_bytes_numb = ring_buff.Read(dst_.begin(), dst_.end());
    ```
    _Полный код можно посмотреть в тесте `TEST(RingBuff, WriteThenReadIterator)`._

#### Пропуск данных в буфере 

Метод `Skip` используется для перемещения указателя чтения данных на указанное количество байтов вперёд. Пройденные байты отмечаются как прочитанные.  
Метод имеет следующее определение: `auto Skip(lwrb_sz_t size_in_bytes)`.
Пример использования метода представлен ниже:
```c++
std::string src{"Hello World!"};
auto written_bytes_numb =
      ring_buff.Write(static_cast<const void *>(src.data()), src.size());
constexpr std::size_t skip_need{10};
auto skip_bytes = ring_buff.Skip(skip_need);
```
_Полный код можно посмотреть в тесте `TEST(RingBuff, Skip)`._

#### Получение свободного места в буфере

Для получения количества байтов, свободных для записи используется метод `Free()`:
```c++
ASSERT_EQ(ringbuff_size_in_bytes - 1, ring_buff.Free());
```

#### Получение количества байт, находящихся в буфере

Метод `Size()`:
```c++
auto written_bytes_numb =
    ring_buff.Write(static_cast<const void *>(src.c_str()), src.size());
ASSERT_EQ(src.size(), ring_buff.Size());
```

#### Очистка кольцевого буфера

Для очистки буфера применяется метод `Clear()`:
```c++
std::string src{"Hello World!"};
auto written_bytes_numb =
    ring_buff.Write(static_cast<const void *>(src.data()), src.size());
ring_buff.Clear();
ASSERT_EQ(0u, ring_buff.Size());
```
_Полный код можно посмотреть в тесте `TEST(RingBuff, Clear)`._
<hr>

### Paraos multiple ring buffer

`Multiple ring buffer` представляет собой структуру, которая позволяет помещать данные в несколько независимых кольцевых буферов. Для определения в какой буфер были помещены данные используется управляющая очередь.

Код из примера расположен в `./tests/test_multi_ringbuff.cpp`.

Перед работой с multi ring buffer необходимо выполнить include:

```c++
#include "paraos_multi_ringbuff.hpp"
```

#### Инициализация multi ring buffer

Согласно шаблону:
```c++
template <std::size_t QUEUE_SIZE, std::size_t RING_BUFF_SIZE, std::size_t RING_BUFF_NUMB>
```

Для инициализации необходимо в качестве шаблонных параметров передать `размер управляющей очереди`, `размер кольцевых буферов` и `количество кольцевых буферов`.

Инициализация `Multi ring buffer` может выглядеть следующим образом:
```c++
constexpr std::size_t queue_size{5};
constexpr std::size_t max_ring_buff_numb{5};
constexpr std::size_t max_ring_buff_size{128};
paraos::MultiRingBuff<queue_size, max_ring_buff_size, max_ring_buff_numb> multi_ring_buff{};
```

#### Помещение данных в кольцевой буфер multi ring buffer

Для записи данных используется метод `Write`. Ниже представлены доступные перегрузки данного метода и примеры их использования.

- Запись через указатель на буфер-источник:  
    ```c++
    auto Write(
        std::size_t buff_id, 
        const void* src, std::size_t src_size, 
        std::size_t timeout_ms, bool is_isr = false) -> std::size_t
    ```  

    ```c++
    constexpr std::size_t write_timeout_ms = 0u;
    std::string str{"Hello world!"};
    constexpr std::size_t cbuff_id{0};
    auto written_bytes_numb = multi_ring_buff.Write(
        cbuff_id, static_cast<const void *>(str.data()), str.size(), write_timeout_ms);
    ```
    _Полный код можно посмотреть в тесте `TEST(MultiRingBuff, WriteDataToRingBufferThenRead)`._

- Запись с использованием `span`:  
    ```c++
    auto Write(
        std::size_t buff_id, 
        const gsl::span<std::uint8_t> src,
        std::size_t timeout_ms, bool is_isr = false) -> std::size_t
    ```

    ```c++
    constexpr std::size_t write_timeout_ms = 0u;
    std::string str{"Hello world!"};
    // Каст строки в массив uint8_t для последующего преобразования в span.
    std::vector<uint8_t> myVector(str.begin(), str.end());

    constexpr std::size_t cbuff_id{0};
    auto written_bytes_numb = multi_ring_buff.Write(cbuff_id, myVector, write_timeout_ms);
    ```
    _Полный код можно посмотреть в тесте `TEST(MultiRingBuff, WriteSpanToRingBufferThenRead)`._


#### Чтение данных из кольцевого буфера multi ring buffer

Для получения данных используется метод `Read`. Ниже представлены доступные перегрузки данного метода и примеры их использования.

- Чтение через указатель на буфер:  
    ```c++
    auto Read(
        std::size_t& buff_id, 
        void* dst, std::size_t dst_size,
        std::size_t timeout_ms, bool is_isr = false) -> std::size_t
    ```

    ```c++
    constexpr std::size_t read_timeout_ms = 0u;
    std::array<std::uint8_t, 128> dst_arr{};
    std::size_t buff_id;
    auto read_bytes_numb = multi_ring_buff.Read(
        buff_id, static_cast<void *>(dst_arr.data()), dst_arr.size(), read_timeout_ms);
    ```
    _Полный код можно посмотреть в тесте `TEST(MultiRingBuff, WriteDataToRingBufferThenRead)`._

- Чтение через `span`:  
    ```c++
    auto Read(
        std::size_t& buff_id, 
        gsl::span<std::uint8_t> dst, 
        std::size_t timeout_ms, bool is_isr = false) -> std::size_t
    ```

    ```c++
    constexpr std::size_t read_timeout_ms = 0u;
    std::array<std::uint8_t, 128> dst_arr{};
    std::size_t buff_id;
    auto read_bytes_numb = multi_ring_buff.Read(buff_id, dst_arr, read_timeout_ms);
    ```
    _Полный код можно посмотреть в тесте `TEST(MultiRingBuff, WriteSpanToRingBufferThenRead)`._

<hr>
