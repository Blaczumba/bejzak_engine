#include <glm/glm.hpp>
#include <gtest/gtest.h>

// Demonstrate some basic assertions.
TEST(HelloTest, GLMinclude) {
  glm::ivec2 first = {1, 2};
  glm::ivec2 second = {1, 2};
  EXPECT_EQ(first, second);
}
