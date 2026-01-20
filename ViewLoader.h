#pragma once

#include <string>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <vector>
#include <map>

namespace fs = std::filesystem;

class ViewLoader {
public:
    // Load view with simple values
    static std::string loadView(const std::string& viewName, 
                               const std::map<std::string, std::string>& values = {}) {
        std::string html = loadViewFile(viewName);
        
        for (const auto& [key, value] : values) {
            html = replaceAll(html, "{{" + key + "}}", value);
        }
        
        return html;
    }
    
    // Load view with ARRAY of MAPS (each item has multiple fields)
    static std::string loadViewWithData(const std::string& viewName,
                                       const std::map<std::string, std::string>& values) {
        // 1. Load the HTML file
        std::string html = loadView(viewName);
        
        // 2. Replace ALL placeholders
        for (const auto& [key, value] : values) {
            std::string placeholder = "{{" + key + "}}";
            size_t pos = 0;
            while ((pos = html.find(placeholder, pos)) != std::string::npos) {
                html.replace(pos, placeholder.length(), value);
                pos += value.length();
            }
        }
        
        return html;
    }

private:
    // Load HTML file from disk
    static std::string loadViewFile(const std::string& viewName) {
        std::string filePath = "views/" + viewName + ".html";
        std::ifstream file(filePath);
        
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open view: " + filePath);
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }
    
    // Replace all occurrences of a string
    static std::string replaceAll(std::string str, 
                                 const std::string& from, 
                                 const std::string& to) {
        size_t pos = 0;
        while ((pos = str.find(from, pos)) != std::string::npos) {
            str.replace(pos, from.length(), to);
            pos += to.length();
        }
        return str;
    }
};