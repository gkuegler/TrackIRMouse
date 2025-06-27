/*
 * Ensure I add this source file to my visual studio project, so that it gets
 * compiled.
 */

#include "example-simple.h"

#include <exception>
namespace ExampleSimple {
void
runTestException()
{
  throw std::exception("Test Failed.");
}
int
runTestValue()
{
  return 42;
}
}