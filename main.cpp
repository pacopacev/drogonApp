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


    // Initialize database
    if (!DatabaseConfig::getInstance().initialize()) {
        LOG_ERROR << "Failed to initialize database. Exiting...";
        return 1;
    }

    

    #ifdef _WIN32
        // Windows - working directory is build folder, so go up one level
        std::string publicPath = "../public";
    #else
        std::string publicPath = "../public";
    #endif
    
    std::cout << "Using public path: " << publicPath << "\n";
    
    app().setThreadNum(4);
    app().setLogLevel(trantor::Logger::kInfo);
    app().addListener("0.0.0.0", 8080);
    app().setDocumentRoot(publicPath);

    
    
    // ========== ROUTE DEFINITIONS ==========
    
    // Route 1: Home page - load from views/home.html
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

    //Login page
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
    
    // Route 2: Simple JSON API
    app().registerHandler("/api/hello",
        [](const HttpRequestPtr& req,
           std::function<void(const HttpResponsePtr&)>&& callback) {
            Json::Value json;
            json["message"] = "Hello from Drogon Web Server!";
            json["status"] = "success";
            json["framework"] = "Drogon";
            json["language"] = "C++";
            json["version"] = "1.9.11";
            json["timestamp"] = trantor::Date::now().toFormattedString(false);
            
            auto resp = HttpResponse::newHttpJsonResponse(json);
            callback(resp);
        },
        {Get});
    
    // Route 3: Greeting - load from views/greet.html with template substitution
    app().registerHandler("/greet/{name}",
        [](const HttpRequestPtr& req,
           std::function<void(const HttpResponsePtr&)>&& callback,
           const std::string& name) {
            
            try {
                std::string html = ViewLoader::loadViewWithData("greet", "NAME", name);
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
    
    // Route 4: Health check endpoint
    app().registerHandler("/health",
        [](const HttpRequestPtr& req,
           std::function<void(const HttpResponsePtr&)>&& callback) {
            Json::Value json;
            json["status"] = "healthy";
            json["service"] = "Drogon Web Server";
            json["version"] = "1.9.11";
            json["timestamp"] = trantor::Date::now().toFormattedString(false);
            
            auto resp = HttpResponse::newHttpJsonResponse(json);
            callback(resp);
        },
        {Get});
    
    // Route 5: Simple HTML page (fallback)
    app().registerHandler("/simple",
        [](const HttpRequestPtr& req,
           std::function<void(const HttpResponsePtr&)>&& callback) {
            auto resp = HttpResponse::newHttpResponse();
            resp->setContentTypeCode(CT_TEXT_HTML);
            resp->setBody(R"(<!DOCTYPE html>
<html>
<head><title>Simple Page</title></head>
<body>
    <h1>Simple HTML Page</h1>
    <p>This page doesn't use CSP templates.</p>
    <p><a href="/">Back to Home</a></p>
</body>
</html>)");
            callback(resp);
        },
        {Get});
    
    // ========== START SERVER ==========
    
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "      DROGON WEB SERVER v1.9.11\n";
    std::cout << std::string(60, '=') << "\n";
    std::cout << "Server running on http://localhost:8080\n";
    std::cout << "Press Ctrl+C to stop.\n";
    std::cout << std::string(60, '=') << "\n\n";
    
    drogon::app()
        .setThreadNum(4)
        .setLogLevel(trantor::Logger::kInfo)
        .enableSession()
        .run();
    
    return 0;
}