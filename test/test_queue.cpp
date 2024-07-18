#include <gtest/gtest.h>

#include "paraos_queue.hpp"

using namespace paraos;

TEST(Queue, Create) { Queue<int> queue{10}; }