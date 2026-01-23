#include "AuthController.h"
#include <drogon/orm/DbClient.h>
#include <drogon/utils/Utilities.h>
#include "DatabaseConfig.h"
#include <iostream>
#include <random>
#include <sstream>
#include <iomanip>

using namespace drogon;
using namespace drogon::orm;

// Helper function to generate random hex token
std::string generateRandomToken() {
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
    std::cout << "✅ AuthController instantiated" << std::endl;
}

drogon::orm::DbClientPtr AuthController::getDbClient() {
    try {
        return DatabaseConfig::getInstance().getClient();
    } catch (const std::exception& e) {
        std::cerr << "❌ Database client error: " << e.what() << std::endl;
        return nullptr;
    }
}

Json::Value AuthController::parseJsonBody(const HttpRequestPtr& req) {
    auto json = req->getJsonObject();
    if (!json) {
        throw std::runtime_error("No JSON body");
    }
    return *json;
}

HttpResponsePtr AuthController::createJsonResponse(const Json::Value& json, HttpStatusCode code) {
    auto resp = HttpResponse::newHttpJsonResponse(json);
    resp->setStatusCode(code);
    return resp;
}

HttpResponsePtr AuthController::createErrorResponse(const std::string& message, HttpStatusCode code) {
    Json::Value error;
    error["error"] = true;
    error["message"] = message;
    error["code"] = static_cast<int>(code);
    return createJsonResponse(error, code);
}

std::string AuthController::generateToken() {
    return generateRandomToken();
}

// REGISTER ENDPOINT
void AuthController::registerUser(const HttpRequestPtr& req,
                                 std::function<void(const HttpResponsePtr&)>&& callback) {
    std::cout << "🔵 AuthController::registerUser called" << std::endl;
    std::cout << "Path: " << req->path() << std::endl;
    
    auto dbClient = getDbClient();
    
    if (!dbClient) {
        callback(createErrorResponse("Database not available", k503ServiceUnavailable));
        return;
    }
    
    try {
        Json::Value json = parseJsonBody(req);
        
        if (!json.isMember("username") || !json.isMember("email") || !json.isMember("password")) {
            callback(createErrorResponse("Missing fields: username, email, password", k400BadRequest));
            return;
        }
        
        std::string username = json["username"].asString();
        std::string email = json["email"].asString();
        std::string password = json["password"].asString();
        std::string passwordHash = drogon::utils::getSha256(password);
        
        dbClient->execSqlAsync(
            "INSERT INTO users_drogon (username, email, password_hash) VALUES ($1, $2, $3) RETURNING id",
            [callback, this](const Result& r) {  // Capture 'this'
                if (!r.empty()) {
                    Json::Value respJson;
                    respJson["success"] = true;
                    respJson["message"] = "User created successfully";
                    respJson["user_id"] = r[0]["id"].as<int>();
                    
                    auto resp = createJsonResponse(respJson);  // Now 'this' is captured
                    std::cout << "✅ User created successfully" << std::endl;
                    std::cout << "📋 New User ID: " << r[0]["id"].as<int>() << std::endl;
                    
                    callback(resp);
                }
            },
            [callback, this](const DrogonDbException& e) {  // Capture 'this'
                std::cerr << "❌ Registration error: " << e.base().what() << std::endl;
                
                Json::Value respJson;
                if (std::string(e.base().what()).find("duplicate") != std::string::npos) {
                    respJson["error"] = "Username or email already exists";
                    auto resp = createJsonResponse(respJson, k409Conflict);
                    callback(resp);
                } else {
                    respJson["error"] = "Database error";
                    auto resp = createJsonResponse(respJson, k500InternalServerError);
                    callback(resp);
                }
            },
            username, email, passwordHash
        );
    } catch (const std::exception& e) {
        callback(createErrorResponse(std::string("Invalid request: ") + e.what(), k400BadRequest));
    }
}

// LOGIN ENDPOINT
void AuthController::login(const HttpRequestPtr& req,
                          std::function<void(const HttpResponsePtr&)>&& callback) {
    std::cout << "🔵 AuthController::login called" << std::endl;
    
    auto dbClient = getDbClient();
    
    if (!dbClient) {
        callback(createErrorResponse("Database not available", k503ServiceUnavailable));
        return;
    }
    
    try {
        Json::Value json = parseJsonBody(req);
        
        if (!json.isMember("username") || !json.isMember("password")) {
            callback(createErrorResponse("Missing username or password", k400BadRequest));
            return;
        }
        
        std::string username = json["username"].asString();
        std::string password = json["password"].asString();
        
        dbClient->execSqlAsync(
            "SELECT id, username, email, password_hash FROM users_drogon WHERE username = $1 OR email = $1",
            [password, callback, req, this](const Result& r) {  // Capture 'this'
                if (r.empty()) {
                    callback(createErrorResponse("Invalid credentials", k401Unauthorized));
                    return;
                }
                
                std::string storedHash = r[0]["password_hash"].as<std::string>();
                std::string inputHash = drogon::utils::getSha256(password);
                
                if (inputHash == storedHash) {
                    auto session = req->session();
                    session->insert("user_id", r[0]["id"].as<int>());
                    session->insert("username", r[0]["username"].as<std::string>());
                    session->insert("email", r[0]["email"].as<std::string>());
                    
                    Json::Value respJson;
                    respJson["success"] = true;
                    respJson["message"] = "Login successful";
                    respJson["token"] = generateRandomToken();
                    
                    Json::Value userJson;
                    userJson["id"] = r[0]["id"].as<int>();
                    userJson["username"] = r[0]["username"].as<std::string>();
                    userJson["email"] = r[0]["email"].as<std::string>();
                    respJson["user"] = userJson;
                    
                    auto resp = createJsonResponse(respJson);
                    
                    // FIXED: Correct addCookie signature
                    resp->addCookie("session_id", session->sessionId());
                    
                    std::cout << "✅ Login successful for user: " << r[0]["username"].as<std::string>() << std::endl;
                    callback(resp);
                } else {
                    callback(createErrorResponse("Invalid credentials", k401Unauthorized));
                }
            },
            [callback, this](const DrogonDbException& e) {  // Capture 'this'
                std::cerr << "❌ Login database error: " << e.base().what() << std::endl;
                callback(createErrorResponse("Database error", k500InternalServerError));
            },
            username
        );
    } catch (const std::exception& e) {
        callback(createErrorResponse(std::string("Invalid request: ") + e.what(), k400BadRequest));
    }
}

// LOGOUT ENDPOINT
void AuthController::logout(const HttpRequestPtr& req,
                           std::function<void(const HttpResponsePtr&)>&& callback) {
    std::cout << "🔵 AuthController::logout called" << std::endl;
    
    auto session = req->session();
    
    if (session && session->find("user_id")) {
        session->erase("user_id");
        session->erase("username");
        session->erase("email");
        session->erase("token");
        
        std::cout << "✅ User logged out successfully" << std::endl;
    }
    
    Json::Value respJson;
    respJson["success"] = true;
    respJson["message"] = "Logged out successfully";
    
    auto resp = createJsonResponse(respJson);
    
    // FIXED: Correct way to expire cookie
    auto cookie = Cookie("session_id", "");
    cookie.setExpiresDate(trantor::Date::date());
    resp->addCookie(cookie);
    
    callback(resp);
}

// GET CURRENT USER ENDPOINT
void AuthController::getCurrentUser(const HttpRequestPtr& req,
                                   std::function<void(const HttpResponsePtr&)>&& callback) {
    std::cout << "🔵 AuthController::getCurrentUser called" << std::endl;
    
    auto session = req->session();
    auto dbClient = getDbClient();
    
    if (!dbClient) {
        callback(createErrorResponse("Database not available", k503ServiceUnavailable));
        return;
    }
    
    if (!session || !session->find("user_id")) {
        callback(createErrorResponse("Not authenticated", k401Unauthorized));
        return;
    }
    
    int userId = session->get<int>("user_id");
    
    dbClient->execSqlAsync(
        "SELECT id, username, email FROM users_drogon WHERE id = $1",
        [callback, this](const Result& r) {  // Capture 'this'
            if (r.empty()) {
                callback(createErrorResponse("User not found", k404NotFound));
                return;
            }
            
            Json::Value respJson;
            respJson["success"] = true;
            
            Json::Value userJson;
            userJson["id"] = r[0]["id"].as<int>();
            userJson["username"] = r[0]["username"].as<std::string>();
            userJson["email"] = r[0]["email"].as<std::string>();
            respJson["user"] = userJson;
            
            auto resp = createJsonResponse(respJson);
            callback(resp);
        },
        [callback, this](const DrogonDbException& e) {  // Capture 'this'
            std::cerr << "❌ Get user database error: " << e.base().what() << std::endl;
            callback(createErrorResponse("Database error", k500InternalServerError));
        },
        userId
    );
}

// DASHBOARD ENDPOINT
void AuthController::getDashboard(const HttpRequestPtr& req,
                                 std::function<void(const HttpResponsePtr&)>&& callback) {
    std::cout << "🔵 AuthController::getDashboard called" << std::endl;
    
    auto session = req->session();
    
    if (!session || !session->find("user_id")) {
        callback(createErrorResponse("Not authenticated", k401Unauthorized));
        return;
    }
    
    // Create dashboard data
    Json::Value dashboard;
    dashboard["welcome"] = "Welcome to the Packaging System Dashboard";
    dashboard["user_id"] = session->get<int>("user_id");
    dashboard["username"] = session->get<std::string>("username");
    dashboard["packaging_orders"] = 12;
    dashboard["pending_shipments"] = 3;
    dashboard["completed_today"] = 8;
    dashboard["system_status"] = "Operational";
    dashboard["timestamp"] = std::to_string(time(nullptr));
    
    // Recent activities
    Json::Value activities(Json::arrayValue);
    
    Json::Value activity1;
    activity1["id"] = 1;
    activity1["action"] = "Package #1234 processed";
    activity1["time"] = "Just now";
    activity1["status"] = "completed";
    activities.append(activity1);
    
    Json::Value activity2;
    activity2["id"] = 2;
    activity2["action"] = "New packaging order received";
    activity2["time"] = "5 minutes ago";
    activity2["status"] = "pending";
    activities.append(activity2);
    
    Json::Value activity3;
    activity3["id"] = 3;
    activity3["action"] = "Shipment #5678 dispatched";
    activity3["time"] = "1 hour ago";
    activity3["status"] = "shipped";
    activities.append(activity3);
    
    dashboard["recent_activities"] = activities;
    
    Json::Value respJson;
    respJson["success"] = true;
    respJson["dashboard"] = dashboard;
    
    auto resp = createJsonResponse(respJson);
    callback(resp);
}