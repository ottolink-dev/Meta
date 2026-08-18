/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#include "meta.hpp"
#include <gtest/gtest.h>

TEST(EventTest, SubscribeAndNotify)
{
  meta::Event<int> event;
  int              count = 0;

  auto conn = event.subscribe([&count](int val) { count += val; });

  event.notify(5);
  EXPECT_EQ(count, 5);

  event.notify(10);
  EXPECT_EQ(count, 15);
}

TEST(EventTest, Unsubscribe)
{
  meta::Event<std::string> event;
  std::string              last_msg;

  auto conn = event.subscribe([&last_msg](const std::string &msg)
                              { last_msg = msg; });

  event.notify("first");
  EXPECT_EQ(last_msg, "first");

  conn.disconnect();
  event.notify("second");
  EXPECT_EQ(last_msg, "first");
}

TEST(EventTest, TeardownSafety)
{
  meta::EventConnection conn;
  {
    meta::Event<int> event;
    conn = event.subscribe([](int) {});
  }
  EXPECT_NO_THROW(conn.disconnect());
}
