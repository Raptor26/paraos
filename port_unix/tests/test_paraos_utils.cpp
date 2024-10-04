#include <gtest/gtest.h>

#include "paraos_utils.hpp"

using namespace paraos;

TEST(MsToTimeSpec, ZeroMs) {
  auto timespec = MilisecondsInTimespec(0);

  ASSERT_EQ(0, timespec.tv_sec);
  ASSERT_EQ(0, timespec.tv_nsec);
}

TEST(MsToTimeSpec, Ms500) {
  auto timespec = MilisecondsInTimespec(500);

  ASSERT_EQ(0, timespec.tv_sec);
  ASSERT_EQ(500 * 1000 * 1000, timespec.tv_nsec);
}

TEST(MsToTimeSpec, Ms1500) {
  auto timespec = MilisecondsInTimespec(1500);

  ASSERT_EQ(1, timespec.tv_sec);
  ASSERT_EQ(500 * 1000 * 1000, timespec.tv_nsec);
}

TEST(MsToTimeSpec, Ms1400) {
  auto timespec = MilisecondsInTimespec(1400);

  ASSERT_EQ(1, timespec.tv_sec);
  ASSERT_EQ(400 * 1000 * 1000, timespec.tv_nsec);
}

TEST(MsToTimeSpec, Ms1234) {
  auto timespec = MilisecondsInTimespec(1234);

  ASSERT_EQ(1, timespec.tv_sec);
  ASSERT_EQ(234 * 1000 * 1000, timespec.tv_nsec);
}

TEST(MsToTimeSpec, Ms10_987) {
  auto timespec = MilisecondsInTimespec(10987);

  ASSERT_EQ(10, timespec.tv_sec);
  ASSERT_EQ(987 * 1000 * 1000, timespec.tv_nsec);
}
