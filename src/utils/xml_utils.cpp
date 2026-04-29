#include <iostream>
#include <pugixml.hpp>
#include <string>

namespace xml_utils {

    pugi::xml_document parse(const std::string& text) {
        pugi::xml_document doc;
        pugi::xml_parse_result result = doc.load_string(text.c_str());

        if (!result) {
            std::cerr << result.description();
        }
        
        return doc;
    }
    
    std::string extract_link(const pugi::xml_document& doc) {
        return doc.child("link").text().as_string();
    }
} // namespace xml_utils
