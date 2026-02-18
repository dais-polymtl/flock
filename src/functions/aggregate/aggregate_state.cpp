#include "flock/functions/aggregate/aggregate.hpp"

namespace flock {

void AggregateFunctionState::Initialize() {
    value = new nlohmann::json(nlohmann::json::array());
    initialized = true;
}

void AggregateFunctionState::Update(const nlohmann::json& input) {
    if (!value) {
        Initialize();
    }

    auto idx = 0u;
    for (const auto& column: input) {
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
                if (!(*value)[idx].contains(item.key())) {
                    (*value)[idx][item.key()] = item.value();
                }
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
        for (const auto& column: *source.value) {
            if (value->size() <= idx) {
                value->push_back(nlohmann::json::object());
            }

            if (!(*value)[idx].contains("data")) {
                (*value)[idx]["data"] = nlohmann::json::array();
            }

            for (const auto& item: column.items()) {
                if (item.key() == "data") {
                    if (item.value().is_array()) {
                        for (const auto& item_value: item.value()) {
                            (*value)[idx]["data"].push_back(item_value);
                        }
                    }
                } else {
                    if (!(*value)[idx].contains(item.key())) {
                        (*value)[idx][item.key()] = item.value();
                    }
                }
            }
            idx++;
        }
    }
}

void AggregateFunctionState::Destroy() {
    initialized = false;
    if (value) {
        delete value;
        value = nullptr;
    }
}

}// namespace flock
