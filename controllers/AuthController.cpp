#include "AuthController.h"
#include <drogon/orm/DbClient.h>
#include <drogon/utils/Utilities.h>
#include "DatabaseConfig.h"

using namespace drogon;
using namespace drogon::orm;

void AuthController::asyncHandleHttpRequest(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback) {
    
    auto json = req->getJsonObject();
    auto dbClient = DatabaseConfig::getInstance().getClient();
    
    if (!dbClient) {
        Json::Value respJson;
        respJson["error"] = "Database not available";
        auto resp = HttpResponse::newHttpJsonResponse(respJson);
        resp->setStatusCode(k503ServiceUnavailable);
        callback(resp);
        return;
    }
    
    // REGISTER
    if (req->getPath() == "/api/register") {
        if (!json || !json->isMember("username") || 
            !json->isMember("email") || !json->isMember("password")) {
            Json::Value respJson;
            respJson["error"] = "Missing fields";
            auto resp = HttpResponse::newHttpJsonResponse(respJson);
            resp->setStatusCode(k400BadRequest);
            callback(resp);
            return;
        }
        
        std::string username = (*json)["username"].asString();
        std::string email = (*json)["email"].asString();
        std::string password = (*json)["password"].asString();
        
        // SIMPLIFY: Use SHA256 for now
        std::string passwordHash = drogon::utils::getSha256(password);
        
        dbClient->execSqlAsync(
            "INSERT INTO users (username, email, password_hash) VALUES ($1, $2, $3) RETURNING id",
            [callback](const Result& r) {
                if (!r.empty()) {
                    Json::Value respJson;
                    respJson["success"] = true;
                    respJson["message"] = "User created successfully";
                    auto resp = HttpResponse::newHttpJsonResponse(respJson);
                    callback(resp);
                }
            },
            [callback](const DrogonDbException& e) {
                Json::Value respJson;
                respJson["error"] = "Username or email already exists";
                auto resp = HttpResponse::newHttpJsonResponse(respJson);
                resp->setStatusCode(k400BadRequest);
                callback(resp);
            },
            username, email, passwordHash
        );
    }
    
    // LOGIN
    else if (req->getPath() == "/api/login") {
        if (!json || !json->isMember("username") || !json->isMember("password")) {
            Json::Value respJson;
            respJson["error"] = "Missing username or password";
            auto resp = HttpResponse::newHttpJsonResponse(respJson);
            resp->setStatusCode(k400BadRequest);
            callback(resp);
            return;
        }
        
        std::string username = (*json)["username"].asString();
        std::string password = (*json)["password"].asString();
        
        dbClient->execSqlAsync(
            "SELECT id, username, email, password_hash FROM users WHERE username = $1 OR email = $1",
            [password, callback, req](const Result& r) {
                if (r.empty()) {
                    Json::Value respJson;
                    respJson["error"] = "Invalid credentials";
                    auto resp = HttpResponse::newHttpJsonResponse(respJson);
                    resp->setStatusCode(k401Unauthorized);
                    callback(resp);
                    return;
                }
                
                std::string storedHash = r[0]["password_hash"].as<std::string>();
                
                // SIMPLIFY: Use SHA256 for now
                bool isValid = (drogon::utils::getSha256(password) == storedHash);
                
                if (isValid) {
                    auto session = req->session();
                    session->insert("user_id", r[0]["id"].as<int>());
                    session->insert("username", r[0]["username"].as<std::string>());
                    
                    Json::Value respJson;
                    respJson["success"] = true;
                    Json::Value userJson;
                    userJson["id"] = r[0]["id"].as<int>();
                    userJson["username"] = r[0]["username"].as<std::string>();
                    userJson["email"] = r[0]["email"].as<std::string>();
                    respJson["user"] = userJson;
                    
                    auto resp = HttpResponse::newHttpJsonResponse(respJson);
                    callback(resp);
                } else {
                    Json::Value respJson;
                    respJson["error"] = "Invalid credentials";
                    auto resp = HttpResponse::newHttpJsonResponse(respJson);
                    resp->setStatusCode(k401Unauthorized);
                    callback(resp);
                }
            },
            [callback](const DrogonDbException& e) {
                Json::Value respJson;
                respJson["error"] = "Database error: " + std::string(e.base().what());
                auto resp = HttpResponse::newHttpJsonResponse(respJson);
                resp->setStatusCode(k500InternalServerError);
                callback(resp);
            },
            username
        );
    }
    
    // LOGOUT
    else if (req->getPath() == "/api/logout") {
        req->session()->erase("user_id");
        req->session()->erase("username");
        
        Json::Value respJson;
        respJson["success"] = true;
        respJson["message"] = "Logged out";
        auto resp = HttpResponse::newHttpJsonResponse(respJson);
        callback(resp);
    }
    
    // GET CURRENT USER
    else if (req->getPath() == "/api/me") {
        auto session = req->session();
        if (!session || !session->find("user_id")) {
            Json::Value respJson;
            respJson["error"] = "Not authenticated";
            auto resp = HttpResponse::newHttpJsonResponse(respJson);
            resp->setStatusCode(k401Unauthorized);
            callback(resp);
            return;
        }
        
        int userId = session->get<int>("user_id");
        
        dbClient->execSqlAsync(
            "SELECT id, username, email FROM users WHERE id = $1",
            [callback](const Result& r) {
                if (r.empty()) {
                    Json::Value respJson;
                    respJson["error"] = "User not found";
                    auto resp = HttpResponse::newHttpJsonResponse(respJson);
                    resp->setStatusCode(k404NotFound);
                    callback(resp);
                    return;
                }
                
                Json::Value respJson;
                Json::Value userJson;
                userJson["id"] = r[0]["id"].as<int>();
                userJson["username"] = r[0]["username"].as<std::string>();
                userJson["email"] = r[0]["email"].as<std::string>();
                respJson["user"] = userJson;
                
                auto resp = HttpResponse::newHttpJsonResponse(respJson);
                callback(resp);
            },
            [callback](const DrogonDbException& e) {
                Json::Value respJson;
                respJson["error"] = "Database error";
                auto resp = HttpResponse::newHttpJsonResponse(respJson);
                resp->setStatusCode(k500InternalServerError);
                callback(resp);
            },
            userId
        );
    }
    
    // Unknown endpoint
    else {
        Json::Value respJson;
        respJson["error"] = "Not found";
        auto resp = HttpResponse::newHttpJsonResponse(respJson);
        resp->setStatusCode(k404NotFound);
        callback(resp);
    }
}
// END OF FILE - NO EXTRA TEXT HERE