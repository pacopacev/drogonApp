#include <cstdio>
#include <chrono>
#include <thread>


#include "GlobalModel.h"
#include <drogon/drogon.h>
#include "drogon/HttpResponse.h"
#include <string>
#include <iostream>
#include "ViewLoader.h"
#include "DatabaseConfig.h"
#include "controllers/AuthController.h"
#include "filters/AuthFilter.h"
#include "models/User.h"
#include <fstream>

using namespace drogon;

int main() {

    

    app().enableSession(30);
    // DISABLE OUTPUT BUFFERING - ADD THIS
    setvbuf(stdout, nullptr, _IONBF, 0);
    setvbuf(stderr, nullptr, _IONBF, 0);

    std::cout << "🚀 STARTING SERVER - BUFFERING DISABLED" << std::endl;

    drogon::app().setLogLevel(trantor::Logger::kTrace);

    // Add debug logging middleware
    app().registerPreHandlingAdvice([](const HttpRequestPtr& req,
                                       AdviceCallback&& acb,
                                       AdviceChainCallback&& accb) {
        // Use both cout and cerr
        std::cout << "➡️ [" << std::time(nullptr) << "] " 
                  << req->methodString() << " " << req->path() << std::endl;
        std::cerr << "➡️ [" << std::time(nullptr) << "] " 
                  << req->methodString() << " " << req->path() << std::endl;
        accb();
    });



    // Debug: Print loaded configuration
    std::cout << "\n=== Configuration Summary ===" << std::endl;

  

// Debug: Print loaded configuration
std::cout << "\n=== Configuration Summary ===" << std::endl;
std::cout << "Document root: " << app().getDocumentRoot() << std::endl;
std::cout << "Number of IO threads: " << app().getThreadNum() << std::endl;
std::cout << "=============================" << std::endl;

// Test if static files directory exists
std::string docRoot = app().getDocumentRoot();
if (docRoot.empty()) {
    std::cout << "⚠ Warning: Document root is not set!" << std::endl;
} else {
    std::cout << "Document root: " << docRoot << std::endl; 
}   
    
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

            std::cout << "🟢 ROOT PATH '/' CALLED!" << std::endl;
            std::cout.flush();  // <-- ADD THIS

            
         
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

    // Register page
    app().registerHandler("/register",
        [](const HttpRequestPtr& req,
           std::function<void(const HttpResponsePtr&)>&& callback) {
            try {
                std::string html = ViewLoader::loadView("register");
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

    // In main(), modify the dashboard route:
    // In main.cpp, fix the dashboard route:
    app().registerHandler("/dashboard",
        [](const HttpRequestPtr& req,
        std::function<void(const HttpResponsePtr&)>&& callback) {

            auto session = req->session();
            if (!session || !session->find("user_id")) {
                auto resp = HttpResponse::newRedirectionResponse("/login?return=/dashboard");
                callback(resp);
                return;
            }

            try
            {
                std::string username = session->get<std::string>("username");
                std::string firstLetter_of_username = username.front() ? std::string(1, username.front()) : "";
                char firstLetter = GlobalModel::toUpper(firstLetter_of_username);
                // std::cout << firstLetter << std::endl;
                std::map<std::string, std::string> values;
                values["username"] = username;
                values["firstLetter"] = firstLetter;
                
                std::string html = ViewLoader::loadViewWithData("dashboard", values);
                
                auto resp = HttpResponse::newHttpResponse();
                resp->setContentTypeCode(CT_TEXT_HTML);
                resp->setBody(html);
                callback(resp);
            }
            catch(const std::exception& e)
            {
                auto resp = HttpResponse::newHttpResponse();
                resp->setStatusCode(k500InternalServerError);
                resp->setBody("Error loading dashboard: " + std::string(e.what()));
                callback(resp);
            }
            
            
        },
        {Get}, {AuthFilter::classTypeName()}); // Protect with AuthFilter

        app().registerHandler("/logout",
        [](const HttpRequestPtr& req,
        std::function<void(const HttpResponsePtr&)>&& callback) {
            // Clear session and redirect to home
            auto session = req->session();
            if (session) {
                session->erase("user_id");
                session->erase("username");
                session->erase("email");
            }
            auto resp = HttpResponse::newRedirectionResponse("/");
            callback(resp);    
        },
        {Get});

        // Add after other routes in main.cpp
app().registerHandler("/test-auth",
    [](const HttpRequestPtr& req,
       std::function<void(const HttpResponsePtr&)>&& callback) {
        
        std::cout << "🔍 Testing AuthController..." << std::endl;
        Json::Value json;
        json["message"] = "AuthController test";
        json["expected_endpoints"] = Json::arrayValue;
        json["expected_endpoints"].append("POST /api/register");
        json["expected_endpoints"].append("POST /api/login");
        json["expected_endpoints"].append("POST /api/logout");
        json["expected_endpoints"].append("GET /api/me");
        json["expected_endpoints"].append("GET /api/dashboard");
        
        auto resp = HttpResponse::newHttpJsonResponse(json);
        callback(resp);
    },
    {Get});

        



    std::cout << "✓ Routes configured" << std::endl;

    
    // ========== END ROUTES ==========
    // ========== SETUP CONTROLLERS ==========
    std::cout << "\nStep 5: Setting up controllers..." << std::endl;
    
    // Create and register AuthController
    auto authController = std::make_shared<AuthController>();
    app().registerController(authController);
    
    std::cout << "✓ AuthController registered" << std::endl;
    std::cout << "  Available endpoints:" << std::endl;
    std::cout << "  - POST /api/register" << std::endl;
    std::cout << "  - POST /api/login" << std::endl;
    std::cout << "  - POST /api/logout" << std::endl;
    std::cout << "  - GET  /api/me" << std::endl;
    std::cout << "  - GET  /api/dashboard (API)" << std::endl;
    

    // ========== START SERVER ==========
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "      DROGON WEB SERVER v1.9.11" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    std::cout << "Server running on http://localhost:8080" << std::endl;
    std::cout << "Database: " << (dbClient ? "Connected ✓" : "Not available") << std::endl;
    std::cout << "Health check: http://localhost:8080/health" << std::endl;
    std::cout << "Press Ctrl+C to stop" << std::endl;
    std::cout << std::string(60, '=') << "\n" << std::endl;
    
    
    
    

    

    app().run();
    
    return 0;
}