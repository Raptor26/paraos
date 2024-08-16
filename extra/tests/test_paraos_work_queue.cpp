#include <gtest/gtest.h>

#include "paraos_work_queue.hpp"

using namespace paraos;

TEST(WorkQueue, Create) {
  WorkQueue work_queue("Def", 1024, ThreadPriority::kIdle, 2, false);
}