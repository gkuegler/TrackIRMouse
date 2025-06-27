/*
 * Include this header file in the main 'test.cpp' file to run the tests.
 * Tests are automatically registered by the gtest framework.
 * This file cannot be included in more than one translation unit.
 * That is why it is seperate from 'test-settings.h' so it doesn't get included
 * in the .
 */
#include <gtest/gtest.h>

#include "example-simple.h"

namespace ExampleSimple {

TEST(ExampleSimple, Exception)
{
  EXPECT_NO_THROW(runTestException());
}
TEST(ExampleSimple, Value)
{
  EXPECT_EQ(42, runTestValue());
}
}
