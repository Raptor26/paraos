#include <gtest/gtest.h>

#include "paraos_utils.hpp"

using namespace paraos;

TEST(MsToTimerSpec, ZeroMs) {
  auto timespec = MilisecondsInTimerSpec(0);

  ASSERT_EQ(0, timespec.tv_sec);
  ASSERT_EQ(0, timespec.tv_nsec);
}

TEST(MsToTimerSpec, Ms500) {
  auto timespec = MilisecondsInTimerSpec(500);

  ASSERT_EQ(0, timespec.tv_sec);
  ASSERT_EQ(500 * 1000 * 1000, timespec.tv_nsec);
}

TEST(MsToTimerSpec, Ms1500) {
  auto timespec = MilisecondsInTimerSpec(1500);

  ASSERT_EQ(1, timespec.tv_sec);
  ASSERT_EQ(500 * 1000 * 1000, timespec.tv_nsec);
}

TEST(MsToTimerSpec, Ms1400) {
  auto timespec = MilisecondsInTimerSpec(1400);

  ASSERT_EQ(1, timespec.tv_sec);
  ASSERT_EQ(400 * 1000 * 1000, timespec.tv_nsec);
}

TEST(MsToTimerSpec, Ms1234) {
  auto timespec = MilisecondsInTimerSpec(1234);

  ASSERT_EQ(1, timespec.tv_sec);
  ASSERT_EQ(234 * 1000 * 1000, timespec.tv_nsec);
}

TEST(MsToTimerSpec, Ms10_987) {
  auto timespec = MilisecondsInTimerSpec(10987);

  ASSERT_EQ(10, timespec.tv_sec);
  ASSERT_EQ(987 * 1000 * 1000, timespec.tv_nsec);
}
