#include <gtest/gtest.h>

#include "settings.hpp"

// The fixture for testing class Foo.
class SettingsTest : public testing::Test
{
protected:
  // You can remove any or all of the following functions if their bodies would
  // be empty.
  SettingsTest() = default;
  ~SettingsTest() override = default;

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
  // test suite name 'SettingsTest'.
  // Use fixtures also as a namespace.
};

TEST_F(SettingsTest, LoadFromFile)
{
  EXPECT_NO_THROW(Settings::LoadFromFile());
}
