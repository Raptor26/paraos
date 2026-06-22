/// @file bench_paraos_containers.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
#include <benchmark/benchmark.h>

#include <cstring>
#include <string>

#include "paraos_message_buffer.hpp"
#include "paraos_queue_blocking.hpp"

// NOLINTBEGIN(*-magic-numbers, google-build-using-namespace,
// readability-function-cognitive-,
// cppcoreguidelines-avoid-non-const-global-variables, clang-analyzer-deadcode*)

namespace bm = benchmark;

BENCHMARK_MAIN();

namespace {
static void QueueBlockingPushThenPop(bm::State &state) {
  std::string str{"Hello world"};
  constexpr std::size_t max_elem{10};
  paraos::QueueBlocking<std::string, max_elem> queue;
  assert(queue);
  for (auto unused : state) {
    benchmark::DoNotOptimize(queue.TryPush(str));
    benchmark::DoNotOptimize(queue.Pop(0));
  }
}
BENCHMARK(QueueBlockingPushThenPop);

static void MessageBufferPushThenPop(bm::State &state) {
  std::string str{"Hello world"};
  paraos::MessageBuffer<10> buff;
  assert(buff);
  for (auto unused : state) {
    auto write = buff.Alloc(str.size());
    assert(write);
    memcpy(write.Data(), static_cast<const void *>(str.data()), str.size());
    write.TryPush();

    auto read = buff.Pop(0);
    assert(read);
  }
}
BENCHMARK(MessageBufferPushThenPop);

}  // namespace

// NOLINTEND(*-magic-numbers, google-build-using-namespace,
// readability-function-cognitive-,
// cppcoreguidelines-avoid-non-const-global-variables, clang-analyzer-deadcode*)