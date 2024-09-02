#pragma once
#include <map>
#include <vector>

#include "ccl/bmrs.h"

namespace CclExpectedOuputs {
extern const std::map<const char*, std::vector<int>> TestCases;
std::string GetImage(const char* testname);
}  // namespace CclExpectedOuputs
