#pragma once
#include "large_flock/common.hpp"

namespace large_flock {

namespace core {

struct CoreLlmResponseParsers {
	// lf_map response parser that return a vector of strings
	static std::vector<std::string> LfMapResponseParser(std::string &response, int num_responses);
};

} // namespace core

} // namespace large_flock
