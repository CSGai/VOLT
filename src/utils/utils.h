#pragma once
#include <ctime>
#include <libxml/HTMLparser.h>
#include <libxml/tree.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace utils::datetime {
    /// prase rfc822 and rfc3339 to a unix timestamp
    time_t parse_rfc(const std::string& dateStr);
} // namespace utils::datetime

namespace utils::markup {
    class XmlNode {
        public:
            XmlNode(xmlNode* n = nullptr);

            XmlNode child(const char* name) const;
            std::vector<XmlNode> children(const char* name) const;

            std::string value() const;
            std::string child_value(const char* name) const;

            bool exists() const;

        private:
            xmlNode* node;
    };

    class XmlDoc {
        public:
            XmlDoc(const std::string& xml);
            ~XmlDoc();

            XmlNode child(const char* name) const;

        private:
            xmlDocPtr doc;
    };

    class HtmlNode {
        public:
            HtmlNode(xmlNode* n = nullptr);

            HtmlNode child(const char* name) const;
            std::vector<HtmlNode> children(const char* name) const;

            std::string value() const;

            bool exists() const;

        private:
            xmlNode* node;
    };

    class HtmlDoc {
        public:
            HtmlDoc(const std::string& html);
            ~HtmlDoc();

            HtmlNode child(const char* name) const;

        private:
            htmlDocPtr doc;
    };

} // namespace utils::markup

namespace utils::misc {
    /// join string on delimiter
    std::string str_join(const std::vector<std::string>& symbols, const char& delimiter);
    /// remove substring from string
    std::string str_remove(std::string str, const std::string& sub);
} // namespace utils::misc