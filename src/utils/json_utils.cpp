#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace json_utils {

    void write_cache(const fs::path& path, const json& data) {
        std::cout << "writing to cache: " << path << std::endl;
        const auto tmp = fs::path(path).concat(".tmp");

        fs::create_directories(path.parent_path());

        if (std::ofstream f(tmp, std::ios::binary); f) f << data.dump(4);
        else
            std::cerr << "Cannot write: " << tmp.string();

        fs::rename(tmp, path);
    }
    json read_json(const std::filesystem::path& path) {
        try {
            std::ifstream file(path);
            if (!file) {
                std::cerr << "Could not open file for reading: " << path.string();
            }

            json data;
            file >> data;

            return data;
        } catch (const json::parse_error& e) {
            std::cerr << "JSON parse error in '" << path << "': " << e.what() << '\n';
        } catch (const std::exception& e) {
            std::cerr << "Error reading cache '" << path << "': " << e.what() << '\n';
        }

        return {};
    }
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