#include "AuthFilter.h"

void AuthFilter::doFilter(const drogon::HttpRequestPtr& req,
                          drogon::FilterCallback&& fcb,
                          drogon::FilterChainCallback&& fccb) {
    
    auto session = req->session();
    
    // Public routes (no auth required)
    std::vector<std::string> publicRoutes = {
        "/api/login",
        "/api/register",
        "/",
        "/login.html",
        "/register.html",
        "/css/",
        "/js/",
        "/fonts/"
    };
    
    std::string path = req->getPath();
    
    // Check if route is public
    bool isPublic = false;
    for (const auto& route : publicRoutes) {
        if (path.find(route) == 0) {
            isPublic = true;
            break;
        }
    }
    
    if (isPublic) {
        fccb(); // Continue to next filter/controller
        return;
    }
    
    // Check if user is authenticated
    if (!session || !session->find("user_id")) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        
        // For API requests, return JSON
        if (path.find("/api/") == 0) {
            Json::Value json;
            json["error"] = "Not authenticated";
            resp = drogon::HttpResponse::newHttpJsonResponse(json);
            resp->setStatusCode(drogon::k401Unauthorized);
        } 
        // For web pages, redirect to login
        else {
            resp->setStatusCode(drogon::k302Found);
            resp->addHeader("Location", "/login.html");
        }
        
        fcb(resp);
        return;
    }
    
    fccb(); // User is authenticated, continue
}