// AuthFilter.cpp
#include "AuthFilter.h"
#include <drogon/HttpResponse.h>
#include <drogon/utils/Utilities.h>
#include <iostream>

void AuthFilter::doFilter(const drogon::HttpRequestPtr& req,
                          drogon::FilterCallback&& fcb,
                          drogon::FilterChainCallback&& fccb) {
    
    auto session = req->session();
    std::string path = req->path();
    std::string method = req->methodString();

    // Debug logging (optional)
    // std::cout << "[" << method << "] " << path << " - Session: " 
    //           << (session ? "Yes" : "No") << std::endl;

    // ========== HOME PAGE REDIRECT ==========
    if (path == "/") {
        if (session && session->find("user_id")) {
            // Redirect logged-in users to dashboard
            auto resp = drogon::HttpResponse::newRedirectionResponse("/dashboard");
            fcb(resp);
            return;
        }
    }
    
    // ========== PUBLIC ROUTES ==========
    std::vector<std::string> publicRoutes = {
        "/api/login",
        "/api/register",
        "/api/logout",  // Allow logout even if not authenticated
        "/",
        "/login",
        "/register",
        "/logout",
        "/css/",
        "/js/",
        "/fonts/",
        "/assets/",
        "/health",
        "/api/health",
        "/api/hello",
        "/greet/",
        "/api/test",
        "/favicon.ico",
        "/favicon.png",
        "/robots.txt",
        "/sitemap.xml"
    };
    
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
    
    // ========== CHECK AUTHENTICATION ==========
    if (!session || !session->find("user_id")) {
        // Session expired or not logged in
        
        // Debug log
        std::cout << "🔒 Access denied - Session expired for: " << path 
                  << " (Method: " << method << ")" << std::endl;
        
        auto resp = drogon::HttpResponse::newHttpResponse();
        
        // For API requests, return JSON with 401
        if (path.find("/api/") == 0) {
            Json::Value json;
            json["error"] = "Session expired";
            json["message"] = "Please log in again";
            json["redirect"] = "/login";
            json["success"] = false;
            resp = drogon::HttpResponse::newHttpJsonResponse(json);
            resp->setStatusCode(drogon::k401Unauthorized);
        } 
        // For web pages, redirect to login with return URL
        else {
            resp->setStatusCode(drogon::k302Found);
            
            // Encode the current path as return URL (except for login/register pages)
            std::string returnUrl = "";
            if (path != "/login" && path != "/register") {
                returnUrl = "?return=" + drogon::utils::urlEncode(path);
            }
            
            resp->addHeader("Location", "/login" + returnUrl);
        }
        
        fcb(resp);
        return;
    }
    
    // ========== USER IS AUTHENTICATED ==========
    
    // Optional: Log user access (for debugging)
    // if (session->find("user_id")) {
    //     std::cout << "✅ User " << session->get<int>("user_id") 
    //               << " accessing: " << path << std::endl;
    // }
    
    fccb(); // User is authenticated, continue
}