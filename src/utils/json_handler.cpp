#include "utils.h"

#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

namespace _json {
    void write_cache(const std::string& json_name, const json& json_data) {
        try {
            std::ofstream file(json_name);
            if (!file.is_open()) {
                throw std::runtime_error("Could not open file: " + json_name);
            }

            file << json_data.dump(4);

        } catch (std::exception& e) {
            std::cerr << "Error writing JSON: " << e.what() << std::endl;
            throw;
        }
    }
    json read_cache(const std::string& json_name) {
        std::ifstream file(json_name);
        json data = json::parse(file);
        file.close();
        return data;
    }
    json jsonify(std::string data) { return json::parse(data); }
    json test_json() {
        json data;

        data["name"] = "John Doe";
        data["age"] = 30;
        data["active"] = true;

        data["address"]["street"] = "123 Main St";
        data["address"]["city"] = "New York";
        data["address"]["zip"] = "10001";
        return data;
    }
} // namespace _json