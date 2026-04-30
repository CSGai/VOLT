#include "libxml/tree.h"
#include "utils.h"
#include <libxml/HTMLparser.h>
#include <libxml/xmlreader.h>
#include <string>
#include <vector>

namespace utils::markup {

    //------------- HTML PARSING -------------

    /// constructor
    HtmlNode::HtmlNode(xmlNode* n) : node(n) {}

    /// get first matching child node
    HtmlNode HtmlNode::child(const char* name) const {
        if (!node) return HtmlNode(nullptr);

        for (xmlNode* cur = node->children; cur; cur = cur->next) {
            if (cur->type == XML_ELEMENT_NODE && xmlStrcasecmp(cur->name, BAD_CAST name) == 0) {
                return HtmlNode(cur);
            }
        }
        return HtmlNode(nullptr);
    }

    /// get all children with matching tag name
    std::vector<HtmlNode> HtmlNode::children(const char* name) const {
        std::vector<HtmlNode> nodes;
        if (!node) return nodes;

        for (xmlNode* cur = node->children; cur; cur = cur->next) {
            if (cur->type == XML_ELEMENT_NODE && xmlStrcasecmp(cur->name, BAD_CAST name) == 0) {
                nodes.emplace_back(cur);
            }
        }
        return nodes;
    }

    /// get text content of node
    std::string HtmlNode::value() const {
        if (!node) return "";

        xmlChar* content = xmlNodeGetContent(node);
        if (!content) return "";

        std::string result = reinterpret_cast<const char*>(content);
        xmlFree(content);
        return result;
    }

    /// check if node exists
    bool HtmlNode::exists() const { return node != nullptr; }

    /// parse html from memory
    HtmlDoc::HtmlDoc(const std::string& html) {
        doc = htmlReadMemory(html.c_str(), static_cast<int>(html.size()), nullptr, nullptr,
                             HTML_PARSE_RECOVER | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING);
    }

    /// cleanup document
    HtmlDoc::~HtmlDoc() {
        if (doc) xmlFreeDoc(doc);
    }

    /// get first matching child from root
    HtmlNode HtmlDoc::child(const char* name) const {
        if (!doc) return HtmlNode(nullptr);

        xmlNode* root = xmlDocGetRootElement(doc);
        if (!root) return HtmlNode(nullptr);

        // direct match on root
        if (xmlStrcasecmp(root->name, BAD_CAST name) == 0) {
            return HtmlNode(root);
        }

        for (xmlNode* cur = root->children; cur; cur = cur->next) {
            if (cur->type == XML_ELEMENT_NODE && xmlStrcasecmp(cur->name, BAD_CAST name) == 0) {
                return HtmlNode(cur);
            }
        }

        return HtmlNode(nullptr);
    }

    //------------- XML PARSING -------------

    /// constructor
    XmlNode::XmlNode(xmlNode* n) : node(n) {}

    /// get first matching child node
    XmlNode XmlNode::child(const char* name) const {
        if (!node) return XmlNode(nullptr);

        for (xmlNode* cur = node->children; cur; cur = cur->next) {
            if (cur->type == XML_ELEMENT_NODE && xmlStrEqual(cur->name, BAD_CAST name)) {
                return XmlNode(cur);
            }
        }
        return XmlNode(nullptr);
    }

    /// get all matching children
    std::vector<XmlNode> XmlNode::children(const char* name) const {
        std::vector<XmlNode> nodes;
        if (!node) return nodes;

        for (xmlNode* cur = node->children; cur; cur = cur->next) {
            if (cur->type == XML_ELEMENT_NODE && xmlStrEqual(cur->name, BAD_CAST name)) {
                nodes.emplace_back(cur);
            }
        }
        return nodes;
    }

    /// get text content of node
    std::string XmlNode::value() const {
        if (!node) return "";

        xmlChar* content = xmlNodeGetContent(node);
        if (!content) return "";

        std::string result = reinterpret_cast<const char*>(content);
        xmlFree(content);
        return result;
    }

    /// shortcut for child(name).value()
    std::string XmlNode::child_value(const char* name) const { return child(name).value(); }

    /// check if node exists
    bool XmlNode::exists() const { return node != nullptr; }

    /// parse xml from memory
    XmlDoc::XmlDoc(const std::string& xml) {
        doc = xmlReadMemory(xml.c_str(), static_cast<int>(xml.size()), nullptr, nullptr,
                            XML_PARSE_RECOVER | XML_PARSE_NOERROR | XML_PARSE_NOWARNING);
    }

    /// free document
    XmlDoc::~XmlDoc() {
        if (doc) xmlFreeDoc(doc);
    }

    /// get first matching child from root
    XmlNode XmlDoc::child(const char* name) const {
        if (!doc) return XmlNode(nullptr);

        xmlNode* root = xmlDocGetRootElement(doc);
        if (!root) return XmlNode(nullptr);

        // allow direct root match
        if (xmlStrEqual(root->name, BAD_CAST name)) {
            return XmlNode(root);
        }

        for (xmlNode* cur = root->children; cur; cur = cur->next) {
            if (cur->type == XML_ELEMENT_NODE && xmlStrEqual(cur->name, BAD_CAST name)) {
                return XmlNode(cur);
            }
        }

        return XmlNode(nullptr);
    }

} // namespace utils::markup