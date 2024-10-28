/// @file bench_queue_blocking.cpp
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

#include <benchmark/benchmark.h>

#include <cstring>
#include <string>

#include "paraos_message_buffer.hpp"
#include "paraos_queue_blocking.hpp"

namespace bm = benchmark;
using namespace paraos;

BENCHMARK_MAIN();

static void QueueBlockingPushThenPop(bm::State &state) {
  std::string str{"Hello world"};
  constexpr std::size_t max_elem{10};
  QueueBlocking<std::string, max_elem> queue;
  assert(queue);
  for (auto _ : state) {
    benchmark::DoNotOptimize(queue.Push(str, 0));
    benchmark::DoNotOptimize(queue.Pop(0));
  }
}
BENCHMARK(QueueBlockingPushThenPop);

static void MessageBufferPushThenPop(bm::State &state) {
  std::string str{"Hello world"};
  paraos::MessageBuffer<10> buff;
  assert(buff);
  for (auto _ : state) {
    auto write = buff.Alloc(str.size());
    assert(write);
    memcpy(write.Addr(), static_cast<const void *>(str.data()), str.size());
    write.Push();

    auto read = buff.Pop(0);
    assert(read);
  }
}
BENCHMARK(MessageBufferPushThenPop);