#include <gtest/gtest.h>

// Sample function to test
int add(int a, int b) { return a + b; }

int multiply(int a, int b) { return a * b; }

// Basic test case
TEST(SampleTest, BasicAssertion) {
  EXPECT_EQ(1 + 1, 2);
  EXPECT_TRUE(true);
  EXPECT_FALSE(false);
}

// Test fixture for more complex tests
class MathTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Setup code runs before each test
    value1 = 10;
    value2 = 5;
  }

  void TearDown() override {
    // Cleanup code runs after each test
  }

  int value1;
  int value2;
};

// Tests using the fixture
TEST_F(MathTest, Addition) {
  EXPECT_EQ(add(value1, value2), 15);
  EXPECT_EQ(add(0, 0), 0);
  EXPECT_EQ(add(-5, 5), 0);
}

TEST_F(MathTest, Multiplication) {
  EXPECT_EQ(multiply(value1, value2), 50);
  EXPECT_EQ(multiply(0, value1), 0);
  EXPECT_EQ(multiply(-1, value2), -5);
}

// Parameterized test example
class AdditionTest
    : public ::testing::TestWithParam<std::tuple<int, int, int>> {};

TEST_P(AdditionTest, ParameterizedAddition) {
  auto [a, b, expected] = GetParam();
  EXPECT_EQ(add(a, b), expected);
}

INSTANTIATE_TEST_SUITE_P(AdditionTests, AdditionTest,
                         ::testing::Values(std::make_tuple(1, 1, 2),
                                           std::make_tuple(5, 3, 8),
                                           std::make_tuple(-1, 1, 0),
                                           std::make_tuple(0, 0, 0),
                                           std::make_tuple(100, 200, 300)));
