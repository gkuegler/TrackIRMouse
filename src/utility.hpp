#pragma once

#include <string>

#define STR_IMPL_(x) #x     // stringify arg
#define STR(x) STR_IMPL_(x) // indirection to expand

#define STR_IMPL_VA(...) #__VA_ARGS__       // stringify arg
#define STRVA(...) STR_IMPL_VA(__VA_ARGS__) // indirection to expand arg macros

namespace utility {

std::string
GetAbsolutePathRelativeToExeFolder(std::string relative_path);

} // namespace utility
