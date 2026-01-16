#include <drogon/drogon.h>
#include <string>
#include <iostream>
#include "ViewLoader.h"
#include "DatabaseConfig.h"
#include "controllers/AuthController.h"
#include "filters/AuthFilter.h"

using namespace drogon;

int main() {
    // Debug: Check if PostgreSQL is defined
    #ifdef USE_POSTGRESQL
        std::cout << "✓ USE_POSTGRESQL IS DEFINED!" << std::endl;
    #else
        std::cout << "✗ USE_POSTGRESQL IS NOT DEFINED!" << std::endl;
    #endif

    std::cout << "==========================================" << std::endl;
    std::cout << "Starting Drogon Application" << std::endl;
    std::cout << "==========================================" << std::endl;

    // ========== INITIALIZE DATABASE FROM CONFIG.JSON ==========
    std::cout << "\nStep 1: Initializing database..." << std::endl;
    
    // Explicitly call initialize() first
    if (!DatabaseConfig::getInstance().initialize()) {
        std::cout << "⚠ Database initialization failed or no database configured" << std::endl;
        std::cout << "Server will start without database support" << std::endl;
    } else {
        std::cout << "✓ Database configuration loaded" << std::endl;
    }

    // ========== TEST DATABASE CONNECTION ==========
    std::cout << "\nStep 2: Testing database connection..." << std::endl;
    auto dbClient = DatabaseConfig::getInstance().getClient();
    
    if (dbClient) {
        try {
            auto result = dbClient->execSqlSync("SELECT version()");
            std::cout << "✓ Database connected: PostgreSQL " 
                      << result[0]["version"].as<std::string>().substr(0, 60) << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "✗ Database connection test failed: " << e.what() << std::endl;
            dbClient = nullptr;
        }
    } else {
        std::cout << "⚠ No database client available" << std::endl;
    }

    // Store for use in handlers
    auto sharedDbClient = dbClient;

    // ========== LOAD DROGON CONFIGURATION ==========
    std::cout << "\nStep 3: Loading server configuration..." << std::endl;
    try {
        std::string configPath = DatabaseConfig::getInstance().getConfigPath();
        if (!configPath.empty()) {
            std::cout << "Loading from: " << configPath << std::endl;
            app().loadConfigFile(configPath);
            std::cout << "✓ Server configuration loaded" << std::endl;
        } else {
            // Fallback: Set up basic configuration
            app().addListener("0.0.0.0", 8080);
            std::cout << "✓ Using default configuration (port 8080)" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "⚠ Error loading server config: " << e.what() << std::endl;
        app().addListener("0.0.0.0", 8080); // Fallback
        std::cout << "✓ Using fallback configuration (port 8080)" << std::endl;
    }

    // ========== SETUP ROUTES ==========
    std::cout << "\nStep 4: Setting up routes..." << std::endl;
    
    // Home page
    app().registerHandler("/",
        [](const HttpRequestPtr& req,
           std::function<void(const HttpResponsePtr&)>&& callback) {
            try {
                std::string html = ViewLoader::loadView("home");
                auto resp = HttpResponse::newHttpResponse();
                resp->setContentTypeCode(CT_TEXT_HTML);
                resp->setBody(html);
                callback(resp);
            } catch (const std::exception& e) {
                auto resp = HttpResponse::newHttpResponse();
                resp->setStatusCode(k404NotFound);
                resp->setBody("Error: " + std::string(e.what()));
                callback(resp);
            }
        },
        {Get});

    // Login page
    app().registerHandler("/login",
        [](const HttpRequestPtr& req,
           std::function<void(const HttpResponsePtr&)>&& callback) {
            try {
                std::string html = ViewLoader::loadView("login");
                auto resp = HttpResponse::newHttpResponse();
                resp->setContentTypeCode(CT_TEXT_HTML);
                resp->setBody(html);
                callback(resp);
            } catch (const std::exception& e) {
                auto resp = HttpResponse::newHttpResponse();
                resp->setStatusCode(k404NotFound);
                resp->setBody("Error: " + std::string(e.what()));
                callback(resp);
            }
        },
        {Get});

    // Health check
    app().registerHandler("/health",
        [sharedDbClient](const HttpRequestPtr& req,
           std::function<void(const HttpResponsePtr&)>&& callback) {
            Json::Value json;
            json["status"] = "ok";
            json["service"] = "Drogon Web Server";
            
            if (sharedDbClient) {
                json["database"] = "configured";
                try {
                    auto result = sharedDbClient->execSqlSync("SELECT 1 as test");
                    json["database_test"] = "passed";
                } catch (const std::exception& e) {
                    json["database_test"] = "failed";
                    json["database_error"] = e.what();
                }
            } else {
                json["database"] = "not_configured";
            }
            
            auto resp = HttpResponse::newHttpJsonResponse(json);
            callback(resp);
        },
        {Get});

    std::cout << "✓ Routes configured" << std::endl;

    // ========== START SERVER ==========
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "      DROGON WEB SERVER v1.9.11" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    std::cout << "Server running on http://localhost:8080" << std::endl;
    std::cout << "Database: " << (dbClient ? "Connected ✓" : "Not available") << std::endl;
    std::cout << "Health check: http://localhost:8080/health" << std::endl;
    std::cout << "Press Ctrl+C to stop" << std::endl;
    std::cout << std::string(60, '=') << "\n" << std::endl;
    
    // Run the application
    app().run();
    
    return 0;
}