#pragma once

#include <string>
#include <fstream>
#include <sstream>
#include <filesystem>

namespace fs = std::filesystem;

class ViewLoader {
public:
    // Load HTML file and return as string
    static std::string loadView(const std::string& viewName) {
        // Try multiple paths for views directory
        std::vector<fs::path> possiblePaths = {
            fs::current_path() / "views" / (viewName + ".html"),
            fs::current_path() / ".." / "views" / (viewName + ".html"),
            fs::current_path() / ".." / ".." / "views" / (viewName + ".html"),
            fs::path("views") / (viewName + ".html"),
            fs::path("..") / "views" / (viewName + ".html"),
            fs::path("..") / ".." / "views" / (viewName + ".html")
        };
        
        for (const auto& viewPath : possiblePaths) {
            if (fs::exists(viewPath)) {
                std::ifstream file(viewPath);
                if (file.is_open()) {
                    std::stringstream buffer;
                    buffer << file.rdbuf();
                    return buffer.str();
                }
            }
        }
        
        throw std::runtime_error("View file not found: " + viewName + ".html");
    }
    
    // Load view and replace placeholders
    static std::string loadViewWithData(const std::string& viewName, 
                                       const std::string& placeholder,
                                       const std::string& value) {
        std::string html = loadView(viewName);
        
        // Replace placeholder {{PLACEHOLDER}} with value
        std::string search = "{{" + placeholder + "}}";
        size_t pos = 0;
        while ((pos = html.find(search, pos)) != std::string::npos) {
            html.replace(pos, search.length(), value);
            pos += value.length();
        }
        
        return html;
    }
};
