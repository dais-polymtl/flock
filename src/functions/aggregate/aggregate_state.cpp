#include "flockmtl/functions/aggregate/aggregate.hpp"

namespace flockmtl {

void AggregateFunctionState::Initialize() {
    if (!value) {
        value = new std::vector<nlohmann::json>();
    }
}

void AggregateFunctionState::Update(const nlohmann::json& input) {
    if (!value) {
        Initialize();
    }
    value->push_back(input);
}

void AggregateFunctionState::Combine(const AggregateFunctionState& source) {
    if (!value) {
        Initialize();
    }

    if (source.value) {
        for (const auto& input: *source.value) {
            value->push_back(input);
        }
    }
}

void AggregateFunctionState::Destroy() {
    if (value) {
        delete value;
        value = nullptr;
    }
    initialized = false;
}

}// namespace flockmtl
