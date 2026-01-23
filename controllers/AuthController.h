// AuthController.h
#pragma once
#include <drogon/HttpController.h>
#include <drogon/HttpResponse.h>
#include <drogon/HttpRequest.h>
#include <json/json.h>
#include <memory>

using namespace drogon;

class AuthController : public drogon::HttpController<AuthController, false> {
public:
    AuthController();
    
    // Method handlers
    void registerUser(const HttpRequestPtr& req,
                     std::function<void(const HttpResponsePtr&)>&& callback);
    
    void login(const HttpRequestPtr& req,
              std::function<void(const HttpResponsePtr&)>&& callback);
    
    void logout(const HttpRequestPtr& req,
               std::function<void(const HttpResponsePtr&)>&& callback);
    
    void getCurrentUser(const HttpRequestPtr& req,
                       std::function<void(const HttpResponsePtr&)>&& callback);
    
    void getDashboard(const HttpRequestPtr& req,
                     std::function<void(const HttpResponsePtr&)>&& callback);
    
    // Route definitions
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(AuthController::registerUser, "/api/register", Post);
    ADD_METHOD_TO(AuthController::login, "/api/login", Post);
    ADD_METHOD_TO(AuthController::logout, "/api/logout", Post);
    ADD_METHOD_TO(AuthController::getCurrentUser, "/api/me", Get);
    ADD_METHOD_TO(AuthController::getDashboard, "/api/dashboard", Get);
    METHOD_LIST_END

private:
    // Helper methods
    Json::Value parseJsonBody(const HttpRequestPtr& req);
    HttpResponsePtr createJsonResponse(const Json::Value& json, HttpStatusCode code = k200OK);
    HttpResponsePtr createErrorResponse(const std::string& message, HttpStatusCode code = k400BadRequest);
    
    // Database connection
    drogon::orm::DbClientPtr getDbClient();
    
    // Token generation
    std::string generateToken();
};