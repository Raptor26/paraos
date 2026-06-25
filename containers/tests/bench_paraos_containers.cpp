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
static void QueueBlockingPushThenPop(bm::State& state) {
  std::string str{"Hello world"};
  constexpr std::size_t max_elem{10};
  paraos::queue_blocking<std::string, max_elem> queue;
  assert(queue);
  for (auto unused : state) {
    benchmark::DoNotOptimize(queue.try_push(str));
    benchmark::DoNotOptimize(queue.pop(0));
  }
}
BENCHMARK(QueueBlockingPushThenPop);

static void MessageBufferPushThenPop(bm::State& state) {
  std::string str{"Hello world"};
  paraos::message_buffer<10> buff;
  assert(buff);
  for (auto unused : state) {
    auto write = buff.alloc(str.size());
    assert(write);
    memcpy(write.data(), static_cast<const void*>(str.data()), str.size());
    write.try_push();

    auto read = buff.pop(0);
    assert(read);
  }
}
BENCHMARK(MessageBufferPushThenPop);

}  // namespace

// NOLINTEND(*-magic-numbers, google-build-using-namespace,
// readability-function-cognitive-,
// cppcoreguidelines-avoid-non-const-global-variables, clang-analyzer-deadcode*)