#include <gtest/gtest.h>

// The fixture for testing class Foo.
class FixtureTest : public testing::Test
{
protected:
  // You can remove any or all of the following functions if their bodies would
  // be empty.
  FixtureTest() = default;
  ~FixtureTest() override = default;

  // Prefer to not use the constructor and destructor for setup because
  // exceptions can't be caught in constructors or destructors.
  void SetUp() override
  {
    // Code here will be called immediately after the constructor (right
    // before each test).
  }

  void TearDown() override
  {
    // Code here will be called immediately after each test (right
    // before the destructor).
  }

  // Class members & subroutines declared here can be used by all tests with the
  // test suite name 'FixtureTest'.
  // Use fixtures also as a namespace.
  int testmethod() { return 5; }
};

TEST_F(FixtureTest, FixtureInteger)
{
  EXPECT_EQ(5, testmethod());
}