#include "meta.hpp"
#include <gtest/gtest.h>

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  meta::Logger::log()->set_level(spdlog::level::warn);
  return RUN_ALL_TESTS();
}
