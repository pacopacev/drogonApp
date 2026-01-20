#include "AuthController.h"
#include <drogon/orm/DbClient.h>
#include <drogon/utils/Utilities.h>
#include "DatabaseConfig.h"
// #include <bcrypt/BCrypt.hpp>
#include <iostream>


using namespace drogon;
using namespace drogon::orm;


#include <random>
#include <sstream>
#include <iomanip>

// Helper function to generate random hex token
std::string generateToken() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (int i = 0; i < 32; ++i) {
        ss << std::setw(2) << dis(gen);
    }
    return ss.str();
}

AuthController::AuthController() {
    std::cout << "🚀 AuthController constructor called!" << std::endl;
};
void AuthController::asyncHandleHttpRequest(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback) {
    
    // ADD THIS DEBUG OUTPUT
    std::cout << "========================================" << std::endl;
    std::cout << "🔵 AuthController::asyncHandleHttpRequest CALLED!" << std::endl;
    std::cout << "Path: " << req->path() << std::endl;
    std::cout << "Method: " << req->methodString() << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    // Use both cout and cerr
    std::cerr << "[AuthController] Handling: " << req->path() << std::endl;
    
    auto json = req->getJsonObject();
    auto dbClient = DatabaseConfig::getInstance().getClient();
    
    auto path = req->path();
    auto session = req->getSession();

    if (!dbClient) {
        Json::Value respJson;
        respJson["error"] = "Database not available";
        auto resp = HttpResponse::newHttpJsonResponse(respJson);
        resp->setStatusCode(k503ServiceUnavailable);
        callback(resp);
        return;
    }
    if (path == "/api/dashboard") {
        // Handle dashboard API endpoint
        // You can add authentication check here
        Json::Value json;
        json["message"] = "Dashboard API endpoint";
        json["status"] = "success";
        json["timestamp"] = std::to_string(time(nullptr));
        
        auto resp = HttpResponse::newHttpJsonResponse(json);
        callback(resp);
    }
    // REGISTER
    else if (path == "/api/register") {
        // std::cout << "Register endpoint hit\n" << std::endl;
        // std::cout << "JSON: " << json->toStyledString() << std::endl;
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
        std::string salt = drogon::utils::getUuid();
        // std::cout << "Generated salt: " << salt << std::endl;
        
        // SIMPLIFY: Use SHA256 for now
        std::string passwordHash = drogon::utils::getSha256(password);
        // std::string saltedPassword = password + salt;
        // // Use BCrypt.hpp C++ wrapper
        // std::string passwordHash;
        // try {
        //     passwordHash = BCrypt::generateHash(password);
        //     std::cout << "Hash: " << passwordHash << std::endl;
            
        //     // Test the hash
        //     bool valid = BCrypt::validatePassword(password, passwordHash);
        //     std::cout << "\"" << password << "\" : " << (valid ? "valid" : "invalid") << std::endl;
            
        //     bool wrong = BCrypt::validatePassword("wrong", passwordHash);
        //     std::cout << "\"wrong\" : " << (wrong ? "valid" : "invalid") << std::endl;
            
        // } catch (const std::exception& e) {
        //     Json::Value respJson;
        //     respJson["error"] = std::string("Password hashing failed: ") + e.what();
        //     auto resp = HttpResponse::newHttpJsonResponse(respJson);
        //     resp->setStatusCode(k500InternalServerError);
        //     callback(resp);
        //     return;
        // }
        
        
        dbClient->execSqlAsync(
            "INSERT INTO users_drogon (username, email, password_hash) VALUES ($1, $2, $3) RETURNING id",
            [callback](const Result& r) {
                if (!r.empty()) {
                    Json::Value respJson;
                    respJson["success"] = true;
                    respJson["message"] = "User created successfully";
                    auto resp = HttpResponse::newHttpJsonResponse(respJson);
                    std::cout << "User created successfully" << std::endl;
                    std::cout << "New User ID: " << r[0]["id"].as<int>() << std::endl;
                    std::cout << respJson.toStyledString() << std::endl; 
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
    else if (path == "/api/login") {
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
            "SELECT id, username, email, password_hash FROM users_drogon WHERE username = $1 OR email = $1",
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
                // std::cout << "Stored hash: " << drogon::utils::getSha256(password) << std::endl;
                bool isValid = (drogon::utils::getSha256(password) == storedHash);
                
                if (isValid) {
                    auto session = req->session();
                    session->insert("user_id", r[0]["id"].as<int>());
                    session->insert("username", r[0]["username"].as<std::string>());

                    std::string token = generateToken();
                    
                    Json::Value respJson;
                    respJson["success"] = true;
                    respJson["token"] = token;
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
    else if (path == "/api/logout") {

        if(session->find("user_id")){
            session->erase("user_id");    
            session->erase("username");
            session->erase("email");
        }
        
        
        Json::Value respJson;
        respJson["success"] = true;
        respJson["message"] = "Logged out";
        auto resp = HttpResponse::newHttpJsonResponse(respJson);
        callback(resp);
    }
    
    // GET CURRENT USER
    else if (path == "/api/me") {
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
        "SELECT id, username, email FROM users_drogon WHERE id = $1",  // users_drogon NOT users
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
            respJson["error"] = "Database error: " + std::string(e.base().what());
            auto resp = HttpResponse::newHttpJsonResponse(respJson);
            resp->setStatusCode(k500InternalServerError);
            callback(resp);
        },
        session->get<int>("user_id")  // This might throw!
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