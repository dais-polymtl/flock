#include "flock/functions/aggregate/aggregate.hpp"

namespace flock {

void AggregateFunctionState::Initialize() {
    if (!value) {
        value = new nlohmann::json(nlohmann::json::array());
    }
}

void AggregateFunctionState::Update(const nlohmann::json& input) {
    if (!value) {
        Initialize();
    }

    auto idx = 0u;
    for (auto& column: input) {
        if (value->size() <= idx) {
            value->push_back(nlohmann::json::object());
            (*value)[idx]["data"] = nlohmann::json::array();
        }
        for (const auto& item: column.items()) {
            if (item.key() == "data") {
                for (const auto& item_value: item.value()) {
                    (*value)[idx]["data"].push_back(item_value);
                }
            } else {
                (*value)[idx][item.key()] = item.value();
            }
        }
        idx++;
    }
}

void AggregateFunctionState::Combine(const AggregateFunctionState& source) {
    if (!value) {
        Initialize();
    }

    if (source.value) {
        auto idx = 0u;
        for (auto& column: *source.value) {
            for (const auto& item: column.items()) {
                if (item.key() == "data") {
                    for (const auto& item_value: item.value()) {
                        (*value)[idx]["data"].push_back(item_value);
                    }
                } else {
                    (*value)[idx][item.key()] = item.value();
                }
            }
            idx++;
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

}// namespace flock
