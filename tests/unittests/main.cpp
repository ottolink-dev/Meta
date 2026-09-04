#include "meta.hpp"
#include <gtest/gtest.h>

#ifdef META_ENABLE_QT_UI
#include <QApplication>
#endif

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  meta::Logger::log()->set_level(spdlog::level::warn);

#ifdef META_ENABLE_QT_UI
  int          qt_argc = 0;
  char       **qt_argv = nullptr;
  QApplication app(qt_argc, qt_argv);
#endif

  return RUN_ALL_TESTS();
}
