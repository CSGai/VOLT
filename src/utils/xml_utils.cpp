#include <iostream>
#include <pugixml.hpp>

namespace xml_utils {

    pugi::xml_document parse(const std::string& text) {
        pugi::xml_document doc;
        pugi::xml_parse_result result = doc.load_string(text.c_str());

        if (!result) {
            std::cerr << result.description();
        }
        
        return doc;
    }

} // namespace xml_utils
