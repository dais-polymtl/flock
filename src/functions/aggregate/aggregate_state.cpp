#include "flock/functions/aggregate/aggregate.hpp"

namespace flock {

void AggregateFunctionState::Initialize() {
    value = new nlohmann::json(nlohmann::json::array());
    model_details = nlohmann::json::object();
    user_query = "";
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
                // For metadata, only set if not already set
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

    // Copy model_details and user_query from source if not already set
    if (model_details.empty() && !source.model_details.empty()) {
        model_details = source.model_details;
        user_query = source.user_query;
    }

    if (source.value) {
        auto idx = 0u;
        for (const auto& column: *source.value) {
            // Ensure the target value array has enough elements
            if (value->size() <= idx) {
                value->push_back(nlohmann::json::object());
            }

            // Initialize data array if it doesn't exist
            if (!(*value)[idx].contains("data")) {
                (*value)[idx]["data"] = nlohmann::json::array();
            }

            // Merge column metadata - preserve existing, add new
            for (const auto& item: column.items()) {
                if (item.key() == "data") {
                    // Append data items
                    if (item.value().is_array()) {
                        for (const auto& item_value: item.value()) {
                            (*value)[idx]["data"].push_back(item_value);
                        }
                    }
                } else {
                    // For metadata (name, type, etc), only set if not already set
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
    model_details = nlohmann::json::object();
    user_query.clear();
    user_query.shrink_to_fit();
}

}// namespace flock
