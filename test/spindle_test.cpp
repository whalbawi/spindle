#include "spindle.h"

#include "gtest/gtest.h"

class SpindleTest : public ::testing::Test {
  protected:
    spindle::Spindle spindle{};
};

TEST_F(SpindleTest, Version) {
    ASSERT_NE(spindle::Spindle::get_version(), "");
}
